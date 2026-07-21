CC = clang
CFLAGS = -O3 -g -Wall -Wextra -march=haswell -Isrc

SRC = src
BUILD = build

# maybe change later
COMMON = $(BUILD)/lzss.o

PROGRAMS = compress decompress
TESTS = $(BUILD)/tests

all: $(PROGRAMS) $(TESTS)

debug: CFLAGS += -DDEBUG
debug: all

%: $(SRC)/%.c $(COMMON)
	$(CC) $(CFLAGS) $^ -o $@

$(TESTS): tests/tests.c $(COMMON) | $(BUILD)
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD)/%.o: $(SRC)/%.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD):
	mkdir -p $@

clean:
	rm -rf $(BUILD) $(PROGRAMS) $(TESTS)

.PHONY: all debug clean
