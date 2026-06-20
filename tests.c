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
// and round-trip (compress and decompress returns to original)
void test_compress_check(uint8_t orig_buf[], size_t orig_size, 
                         uint8_t expect_buf[], size_t expect_size)
{
    uint8_t actual_buf[1024];
    uint8_t decomp_buf[1024];

    // glibc simulate file streams with memory
    FILE* orig_file = fmemopen(orig_buf, orig_size, "r");
    FILE* actual_file = fmemopen(actual_buf, sizeof(actual_buf), "w");
    FILE* decomp_file = fmemopen(decomp_buf, orig_size, "w");

    compress(orig_file, actual_file);
    decompress(actual_file, decomp_file);

    // check outputs are correctly sized streams
    CHECK((size_t)ftell(actual_file) == expect_size);
    CHECK((size_t)ftell(decomp_file) == orig_size);

    // must close or flush stream to write
    fclose(orig_file);
    fclose(actual_file);
    fclose(decomp_file);

    // actual array check
    CHECK(memcmp(actual_buf, expect_buf, expect_size) == 0);
    CHECK(memcmp(decomp_buf, orig_buf, orig_size) == 0);
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
