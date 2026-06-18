#pragma once

#include <stdio.h>
#include <assert.h>

// TODO: use size_t and better types everywhere

// use same circular buffer for search and lookahead
#define BUFFER_BITS 16
#define BUFFER_SIZE (1 << BUFFER_BITS)
#define WINDOW_LENGTH 32767    // fit into 15 bits
#define LOOKAHEAD_LENGTH 255 // fit into 1 byte
#define REF_MAX_SIZE 3  // max size of offset-length ref

// cool compile-time check
static_assert(BUFFER_SIZE >= WINDOW_LENGTH + LOOKAHEAD_LENGTH,
              "Buffer too small!");

// Dictionary related constants
#define KEY_LENGTH 3  // bytes to hash
#define DICT_BITS 15
#define DICT_SIZE (1 << DICT_BITS)
#define MAX_CHAIN_LENGTH 64
#define NULL_POS -1

#define DEBUG 0 // turn into #ifdef?

#define debug_print(...) \
            do { if (DEBUG) fprintf(stderr, __VA_ARGS__); } while (0)

// dict functions
unsigned int knuth_hash(unsigned int key);
void dict_reset(void);
void dict_insert(unsigned int hash, int pos);
int dict_search(unsigned int hash, int pos, int max_pos, int* best_length);

// Main functions

void compress_stream(FILE* input, FILE* output);

int decompress(FILE* input, FILE* output);

