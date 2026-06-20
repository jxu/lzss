// Tests for compress/decompress functions

#include <assert.h>
#include <complex.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "lzss.h"

#define CHECK(cond) if (!(cond)) { \
    fprintf(stderr, "%s:%d: FAIL %s\n", __FILE__, __LINE__, #cond); \
    exit(1); \
}


void test_dict(void)
{
    dict_reset();

    // try to search empty dict, should not return anything
    //int best_length;

    //CHECK(dict_search(0, 8, &best_length) == 0);

    // insert entry 5:0
    //dict_insert(5, 0);




    printf("Dict tests passed\n");
}

// C arrays decay into pointers when passed to functions, so pass in sizes
// Function tests if inbuf data gets compressed to expect data
void test_compress_check(uint8_t inbuf[], size_t insize, uint8_t expect[], size_t expect_size)
{
    uint8_t outbuf[1024];

    // glibc simulate file stream with memory
    FILE* infile = fmemopen(inbuf, insize, "r");
    FILE* outfile = fmemopen(outbuf, sizeof(outbuf), "w");

    compress(infile, outfile);

    CHECK((size_t)ftell(outfile) == expect_size);

    // must close or flush stream to write
    fclose(infile);
    fclose(outfile);

    // actual array check
    CHECK(memcmp(outbuf, expect, expect_size) == 0);
}


void test_compress(void)
{
    uint8_t inbuf[8] = {0};
    uint8_t expected[] = {0b00000010, 0, 1, 7};

    test_compress_check(inbuf, sizeof(inbuf), expected, sizeof(expected));

    printf("Compress tests passed\n");
}


int main()
{
    test_dict();
 
    //test_compress();
}
