// Tests for compress/decompress functions

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "lzss.h"

#define CHECK(cond) if (!(cond)) \
    fprintf(stderr, "%s:%d: FAIL %s\n", __FILE__, __LINE__, #cond);


int main()
{
    unsigned char inbuf[8] = {0};
    unsigned char outbuf[8];
    unsigned char expected[3] = {1, 1, 8};

    // glibc simulate file stream with memory
    FILE* infile = fmemopen(inbuf, sizeof(inbuf), "r");
    FILE* outfile = fmemopen(outbuf, sizeof(outbuf), "w");

    compress_stream(infile, outfile);

    CHECK(ftell(outfile) == 3);

    // must close or flush stream to write
    fclose(infile);
    fclose(outfile);

    for (int i = 0; i < 3; ++i)
    {
        printf("%x ", outbuf[i]);
    }
    printf("\n");

    CHECK(memcmp(outbuf, expected, sizeof(expected)) == 0);
}
