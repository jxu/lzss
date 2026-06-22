#pragma once

#include <stdio.h>
#include <assert.h>
#include <stdint.h>

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
#define NULL_POS (size_t)(-1)

// Define -DDEBUG in compilation to turn on debug printing
// We do it this way so the print statements are always visible to compiler
// https://stackoverflow.com/a/1644898
#ifdef DEBUG 
    #define DEBUG_PRINT 1
#else
    #define DEBUG_PRINT 0
#endif

#define debug_print(...) \
            do { if (DEBUG_PRINT) fprintf(stderr, __VA_ARGS__); } while (0)

// dict functions
uint32_t knuth_hash(uint32_t key);
void dict_reset(void);
void dict_insert(uint32_t hash, size_t pos);
size_t dict_search(uint32_t hash, size_t pos, size_t end_pos, size_t* best_length);

// Main functions

size_t compress(FILE* input, FILE* output);

int decompress(FILE* input, FILE* output);

