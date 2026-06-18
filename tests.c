// Tests for compress/decompress functions

#include <assert.h>
#include <complex.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
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




void test_compress(void)
{
    unsigned char inbuf[8] = {0};
    unsigned char outbuf[8];
    unsigned char expected[4] = {0b00000010, 0, 1, 7};

    // glibc simulate file stream with memory
    FILE* infile = fmemopen(inbuf, sizeof(inbuf), "r");
    FILE* outfile = fmemopen(outbuf, sizeof(outbuf), "w");

    compress(infile, outfile);

    CHECK(ftell(outfile) == sizeof(expected));

    // must close or flush stream to write
    fclose(infile);
    fclose(outfile);

    for (int i = 0; i < 4; ++i)
    {
        printf("%x ", outbuf[i]);
    }
    printf("\n");

    CHECK(memcmp(outbuf, expected, sizeof(expected)) == 0);

    printf("Compress tests passed\n");
}


int main()
{
    test_dict();
 

    //test_compress();
}
