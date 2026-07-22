CC = clang
CFLAGS = -O3 -g -Wall -Wextra -march=haswell -Isrc -DUNITY_OUTPUT_COLOR

SRC = src
BUILD = build

# maybe change later
COMMON = $(BUILD)/lzss.o

PROGRAMS = $(BUILD)/compress $(BUILD)/decompress
TESTS = $(BUILD)/tests

all: $(PROGRAMS) $(TESTS)

debug: CFLAGS += -DDEBUG
debug: all

$(BUILD)/compress: src/compress.c $(COMMON) | $(BUILD)
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD)/decompress: src/decompress.c $(COMMON) | $(BUILD)
	$(CC) $(CFLAGS) $^ -o $@

$(TESTS): tests/tests.c tests/unity.c $(COMMON) | $(BUILD)
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD)/%.o: $(SRC)/%.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD):
	mkdir -p $@

clean:
	rm -rf $(BUILD) $(PROGRAMS) $(TESTS)

.PHONY: all debug clean
