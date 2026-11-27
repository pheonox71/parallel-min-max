#!/usr/bin/env bash
# Check prog02 against a one-process minmax run and a direct scan of the file.
set -euo pipefail

root=$(cd "$(dirname "$0")/.." && pwd)
build="$root/build"
prog="$build/prog02"
minmax="$build/minmax"
gen="$build/gen_data"
work="$build/testdata"

mkdir -p "$work"

fail() {
    echo "FAIL: $*" >&2
    exit 1
}

# Print "min max" for a file of native-endian 32-bit ints.
scan_file() {
    python3 - "$1" <<'PY'
import struct
import sys

data = open(sys.argv[1], "rb").read()
count = len(data) // 4
if count == 0:
    sys.exit("file has no integers")
values = struct.unpack("=%di" % count, data[: count * 4])
print("%d %d" % (min(values), max(values)))
PY
}

single_process() {
    "$minmax" "$1" 1 0
}

run_prog() {
    local n=$1
    local file=$2
    local out min max lines

    out=$("$prog" "$n" "$file")
    lines=$(printf '%s\n' "$out" | grep -c .)
    if [[ "$lines" -ne 2 ]]; then
        echo "unexpected prog02 output:" >&2
        printf '%s\n' "$out" >&2
        exit 1
    fi
    min=$(printf '%s\n' "$out" | awk '/^Global Min: / { print $3 }')
    max=$(printf '%s\n' "$out" | awk '/^Global Max: / { print $3 }')
    if [[ -z "${min}" || -z "${max}" ]]; then
        echo "could not parse prog02 output:" >&2
        printf '%s\n' "$out" >&2
        exit 1
    fi
    printf '%s %s\n' "$min" "$max"
}

check_file() {
    local label=$1
    local file=$2
    shift 2

    local expected single got n
    expected=$(scan_file "$file")
    single=$(single_process "$file")
    if [[ "$single" != "$expected" ]]; then
        fail "$label: single-process minmax [$single] != scan [$expected]"
    fi
    echo "ok  $label  single-process  $single"

    for n in "$@"; do
        got=$(run_prog "$n" "$file")
        if [[ "$got" != "$expected" ]]; then
            fail "$label with $n processes: expected [$expected], got [$got]"
        fi
        echo "ok  $label  processes=$n  $got"
    done
}

"$gen" 1000 "$work/thousand.bin" 3100
# 3, 7, and 15 do not divide 1000, so the last slice is longer than the others.
check_file "1000 ints" "$work/thousand.bin" 1 2 3 4 7 8 15 16 64

"$gen" 10 "$work/ten.bin" 7
# 3 leaves a remainder. 11 and 17 are more processes than integers.
check_file "10 ints" "$work/ten.bin" 1 3 10 11 17

python3 - "$work/extremes.bin" <<'PY'
import struct
import sys

values = [-2147483648, -5, 0, 1, 2147483647, 42, -100]
with open(sys.argv[1], "wb") as out:
    out.write(struct.pack("=" + "i" * len(values), *values))
PY
# 6 does not divide 7. 20 is more processes than values, including INT_MIN/INT_MAX.
check_file "extremes" "$work/extremes.bin" 1 2 6 8 20

# prog02 should find minmax next to its own binary, not via the current directory.
abs_file=$(realpath "$work/thousand.bin")
from_root=$(cd / && "$prog" 4 "$abs_file")
parsed=$(printf '%s\n' "$from_root" | awk '/^Global Min: / { min = $3 } /^Global Max: / { max = $3 } END { print min, max }')
expected=$(scan_file "$abs_file")
if [[ "$parsed" != "$expected" ]]; then
    fail "run from / : expected [$expected], got [$parsed]"
fi
echo "ok  path lookup from /  $parsed"

echo "All tests passed."
