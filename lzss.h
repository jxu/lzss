#pragma once

#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <sys/types.h>

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

typedef enum
{
    STATUS_SUCCESS,
    STATUS_FAIL,
} lzss_status;

// Compressor (state)
typedef struct
{
    // Main circular buffer, storing search window and lookahead window
    // all positions are indexed mod buffer size
    uint8_t buffer[BUFFER_SIZE];

    // Search hash table "dictionary" data structure, storing positions
    off_t   search_dict[DICT_SIZE];

    // stores previous pos in chain, indexes by current pos
    off_t   prev_pos[BUFFER_SIZE];

    // current input stream position, not mod buffer
    off_t   pos;

    // tracking known end position (byte after last known byte)
    off_t   end_pos;

} compressor;

// Decompressor (less state than Compressor)
typedef struct
{
    uint8_t buffer[BUFFER_SIZE];
    off_t pos;
} decompressor;

// helper functions
uint32_t knuth_hash(uint32_t key);


// dict functions
uint32_t pack3(const compressor* C);
void dict_reset(compressor* C);
void dict_insert(compressor* C, uint32_t hash);
size_t dict_search(compressor* C, uint32_t hash, size_t* best_length);

// Main functions

void compress(compressor* C, FILE* input, FILE* output);
lzss_status decompress(decompressor* D, FILE* input, FILE* output);

