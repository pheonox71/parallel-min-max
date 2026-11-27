CC     = gcc
CFLAGS = -Wall -Wextra -std=c11

SRC   = src
TOOLS = tools
BUILD = build

.PHONY: all test clean

all: $(BUILD)/prog02 $(BUILD)/minmax $(BUILD)/gen_data

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/prog02: $(SRC)/prog02.c | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $<

$(BUILD)/minmax: $(SRC)/minmax.c | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $<

$(BUILD)/gen_data: $(TOOLS)/gen_data.c | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $<

test: all
	bash tests/test_minmax.sh

clean:
	rm -rf $(BUILD)
