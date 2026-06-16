#pragma once

#include <string.h>
#include <stdio.h>

// TODO: use size_t and better types everywhere

// use same circular buffer for search and lookahead
// BUFFER_SIZE should be power of 2 >= WINDOW_LENGTH + LOOKAHEAD_LENGTH
#define BUFFER_SIZE 65536
#define WINDOW_LENGTH 32767    // fit into 15 bits
#define LOOKAHEAD_LENGTH 255 // fit into 1 byte
#define REF_MAX_SIZE 3  // max size of offset-length ref

#define DEBUG 0 // turn into #ifdef?

#define debug_print(...) \
            do { if (DEBUG) fprintf(stderr, __VA_ARGS__); } while (0)


// debugging buffer visualization: prints directly to stderr
inline void print_buffer(char* buf, size_t size)
{
    for (size_t i = 0; i < size; ++i)
    {
        debug_print("%c", buf[i]);
    }
    debug_print("\n");
}

// Main functions

int search(int pos, unsigned max_pos, int* best_length);
void compress_stream(FILE* input, FILE* output);

int decompress(FILE* input, FILE* output);

