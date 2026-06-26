// Tests for compress/decompress functions
// Uses asserts for checking, so don't define NDEBUG

#include <assert.h>
#include <complex.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "lzss.h"

#define TMPBUF_SIZE 4096


void test_dict(void)
{
    // clear buffer to all zeros and reset dict
    memset(buffer, 0, BUFFER_SIZE);
    dict_reset();

    // put in some data
    uint8_t data[] = "aaaaaa";
    memcpy(buffer, data, sizeof data);

    size_t length, offset;

    // hash of "aaa"
    uint32_t aaa_hash = knuth_hash(pack3(0));

    
    // try to search empty dict, should not return anything (offset = 0)
    // dict_search API is kinda awkward because it references global buffer
    offset = dict_search(aaa_hash, 1, 8, &length);
    assert(offset == 0);
    assert(length == 0);

    // insert "aaa" at pos 0
    dict_insert(aaa_hash, 0);

    // search for hash starting at pos 1
    // with end_pos = 5, the best match would be "aaaa" with offset 1, length 4
    offset = dict_search(aaa_hash, 1, 5, &length);
    assert(offset == 1);
    assert(length == 4);

    // with end_pos = 4, best match is offset 1, length 3
    offset = dict_search(aaa_hash, 1, 4, &length);
    assert(offset == 1);
    assert(length == 3);

    // insert "aaa" at pos 1, same hash
    dict_insert(aaa_hash, 1);

    // search for hash starting at pos 2, up to end pos 6
    // both pos 0 and pos 1 match "aaaa", pos 1 is nearer
    offset = dict_search(aaa_hash, 2, 6, &length);
    assert(offset == 1);
    assert(length == 4);


    printf("Dict tests passed\n");
}

// Test if orig data gets compressed to expect data
// and round-trip (compressed gets decompressed to original)
// Arrays decay into pointers when passed to functions, so pass in sizes
void test_exact_helper(uint8_t orig_data[], size_t orig_size, 
                       uint8_t expect_data[], size_t expect_size)
{
    uint8_t actual_buf[TMPBUF_SIZE];
    uint8_t decomp_buf[TMPBUF_SIZE];

    // glibc fmemopen simulates file streams with memory
    // read buffer size must be exact for EOF getc to work correctly
    FILE* orig_file = fmemopen(orig_data, orig_size, "r");
    FILE* actual_file = fmemopen(actual_buf, TMPBUF_SIZE, "w");

    compress(orig_file, actual_file);
    fflush(actual_file); // force write

    // check compressed data is correctly sized and matches
    assert((size_t)ftell(actual_file) == expect_size);
    assert(memcmp(actual_buf, expect_data, expect_size) == 0);

    fclose(orig_file);
    fclose(actual_file);

    // reopen "actual file" for reading with with exact size
    actual_file = fmemopen(actual_buf, expect_size, "r");
    FILE* decomp_file = fmemopen(decomp_buf, TMPBUF_SIZE, "w");

    decompress(actual_file, decomp_file);
    fflush(decomp_file);

    // checks decompressed is correctly sized and matches
    assert((size_t)ftell(decomp_file) == orig_size);
    assert(memcmp(decomp_buf, orig_data, orig_size) == 0);

    fclose(actual_file);
    fclose(decomp_file);
}


void test_exact_nul8(void)
{
    uint8_t orig[8] = {0};
    uint8_t expect[] = {0b00000010, 0, 1, 7};
    test_exact_helper(orig, sizeof(orig), expect, sizeof(expect));
}

void test_exact_nul1(void)
{
    uint8_t orig[1] = {0};
    uint8_t expect[] = {0, 0};
    test_exact_helper(orig, sizeof(orig), expect, sizeof(expect));
}

void test_exact_literals(void)
{
    uint8_t orig[] = {'a','b','c','d','e','f','g','h'};
    uint8_t expect[] = {0, 'a','b','c','d','e','f','g','h'};
    test_exact_helper(orig, sizeof(orig), expect, sizeof(expect));
}

void test_exact_sam(void)
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

    test_exact_helper(orig, sizeof(orig), expect, sizeof(expect));
}

void test_exact(void)
{
    test_exact_nul8();
    test_exact_nul1();
    test_exact_literals();
    test_exact_sam();
    printf("Compress tests passed\n");
}

// idiomatic compare streams: read chunks into buffer and compare
bool streams_equal(FILE* s1, FILE* s2)
{
    uint8_t buf1[TMPBUF_SIZE];
    uint8_t buf2[TMPBUF_SIZE];

    while (1)
    {
        size_t n1 = fread(buf1, 1, sizeof(buf1), s1);
        size_t n2 = fread(buf2, 1, sizeof(buf2), s2);

        if (n1 != n2) // different sizes (or read error)
            return false; 

        if (n1 == 0) // both are 0 and hit EOF, done comparing
            return true;

        if (memcmp(buf1, buf2, n1) != 0) // data not equal
            return false;
    }
}

// Test compress then decompress, checking decompress matches the original
void test_roundtrip_helper(FILE* infile)
{
    FILE* compressed = tmpfile();
    FILE* decompressed = tmpfile();

    // compress/decompress both move file pos, need to be rewound
    compress(infile, compressed);
    rewind(infile);
    rewind(compressed);

    decompress(compressed, decompressed);
    rewind(decompressed);

    assert(streams_equal(infile, decompressed));
}

void test_roundtrip_file(const char* filename)
{
    FILE* sample = fopen(filename, "r");

    test_roundtrip_helper(sample);

    fclose(sample);
}

void test_roundtrip(void)
{
    test_roundtrip_file("samples/sam.txt");
    test_roundtrip_file("samples/kjv.txt"); // several MB

    printf("Roundtrip tests passed\n");
}


int main()
{
    test_dict();
    test_exact();
    test_roundtrip();
}
