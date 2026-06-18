#pragma once

#include <stdio.h>

// TODO: use size_t and better types everywhere

// use same circular buffer for search and lookahead
// BUFFER_SIZE should be power of 2, >= WINDOW_LENGTH + LOOKAHEAD_LENGTH
#define BUFFER_SIZE 65536
#define WINDOW_LENGTH 32767    // fit into 15 bits
#define LOOKAHEAD_LENGTH 255 // fit into 1 byte
#define REF_MAX_SIZE 3  // max size of offset-length ref

// Dictionary related constants
#define KEY_LENGTH 3  // bytes to hash
#define DICT_SIZE 32768
#define HASH_BITS 15 // 2 ** HASH_BITS == DICT_SIZE
#define MAX_CHAIN_LENGTH 64
#define NULL_POS -1

#define DEBUG 1 // turn into #ifdef?

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

