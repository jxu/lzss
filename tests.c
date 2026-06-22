// Tests for compress/decompress functions

#include <assert.h>
#include <complex.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "lzss.h"

#define TMPBUF_SIZE 1024

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

// Test if orig data gets compressed to expect data
// and round-trip (compressed gets decompressed to original)
// Arrays decay into pointers when passed to functions, so pass in sizes
void test_endtoend_data(uint8_t orig_data[], size_t orig_size, 
                        uint8_t expect_data[], size_t expect_size)
{
    uint8_t actual_buf[TMPBUF_SIZE];
    uint8_t decomp_buf[TMPBUF_SIZE];

    // glibc fmemopen simulates file streams with memory
    // read buffer size must be exact for EOF getc to work correctly
    FILE* orig_file = fmemopen(orig_data, orig_size, "r");
    FILE* actual_file = fmemopen(actual_buf, TMPBUF_SIZE, "w");

    size_t actual_written = compress(orig_file, actual_file);
    fflush(actual_file); // force write

    // check compressed data is correctly sized and matches
    CHECK(actual_written == expect_size);
    CHECK(memcmp(actual_buf, expect_data, expect_size) == 0);

    fclose(orig_file);
    fclose(actual_file);

    // reopen "actual file" for reading with with exact size
    actual_file = fmemopen(actual_buf, expect_size, "r");
    FILE* decomp_file = fmemopen(decomp_buf, TMPBUF_SIZE, "w");

    decompress(actual_file, decomp_file);
    fflush(decomp_file);

    // checks decompressed is correctly sized and matches
    CHECK((size_t)ftell(decomp_file) == orig_size);
    CHECK(memcmp(decomp_buf, orig_data, orig_size) == 0);

    fclose(actual_file);
    fclose(decomp_file);
}


void test_endtoend_nul8(void)
{
    uint8_t orig[8] = {0};
    uint8_t expect[] = {0b00000010, 0, 1, 7};
    test_endtoend_data(orig, sizeof(orig), expect, sizeof(expect));
}

void test_endtoend_nul1(void)
{
    uint8_t orig[1] = {0};
    uint8_t expect[] = {0, 0};
    test_endtoend_data(orig, sizeof(orig), expect, sizeof(expect));
}

void test_endtoend_literals(void)
{
    uint8_t orig[] = {'a','b','c','d','e','f','g','h'};
    uint8_t expect[] = {0, 'a','b','c','d','e','f','g','h'};
    test_endtoend_data(orig, sizeof(orig), expect, sizeof(expect));
}

void test_endtoend_sam(void)
{
    // null terminated string
    uint8_t orig[] = "I am Sam. Sam I am. That Sam-I-am! That Sam-I-am! I do not like that Sam-I-am!";
    // can't believe I did this by hand
    uint8_t expect[] = 
    {
        0x00, 'I', ' ', 'a', 'm', ' ', 'S', 'a', 'm',
        0x0a, '.',   5,   4, ' ',  14,   4, '.', ' ', 'T', 'h',
        0x04, 'a', 't',  15,   4, '-', 'I', '-', 'a', 'm',
        0x02, '!',  15,  16, 'I', ' ', 'd', 'o', ' ', 'n',
        0x00, 'o', 't', ' ', 'l', 'i', 'k', 'e', ' ',
        0x02, 't',  29,  13, '\0',
    };

    test_endtoend_data(orig, sizeof(orig), expect, sizeof(expect));
}

void test_endtoend(void)
{
    test_endtoend_nul8();
    test_endtoend_nul1();
    test_endtoend_literals();
    test_endtoend_sam();
    printf("Compress tests passed\n");
}


int main()
{
    //test_dict();
 
    test_endtoend();
}
