#pragma once

#include <stdio.h>

// TODO: use size_t and better types everywhere

// use same circular buffer for search and lookahead
// BUFFER_SIZE should be power of 2 >= WINDOW_LENGTH + LOOKAHEAD_LENGTH
#define BUFFER_SIZE 32
#define WINDOW_LENGTH 16    // fit into 15 bits
#define LOOKAHEAD_LENGTH 16 // fit into 1 byte
#define REF_MAX_SIZE 3  // max size of offset-length ref

// Dictionary related constants
#define KEY_LENGTH 3  // bytes to hash
#define DICT_SIZE 16
#define MAX_CHAIN_LENGTH 4
#define NULL_POS -1

#define DEBUG 1 // turn into #ifdef?

#define debug_print(...) \
            do { if (DEBUG) fprintf(stderr, __VA_ARGS__); } while (0)

// Main functions

int search(int pos, unsigned max_pos, int* best_length);
void compress_stream(FILE* input, FILE* output);

int decompress(FILE* input, FILE* output);

