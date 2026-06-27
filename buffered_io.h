// My own buffered versions of fgetc, fputc
// After done writing, output has to be flushed manually

#pragma once
#include <stdio.h>
#include <stdint.h>

#define INBUF_SIZE 8192
#define OUTBUF_SIZE 8192

static uint8_t inbuf[INBUF_SIZE];
static uint8_t outbuf[OUTBUF_SIZE];

static size_t in_pos = 0;
static size_t in_len = 0;
static size_t out_pos = 0;

static inline int get_byte(FILE* input)
{
    if (in_pos >= in_len)
    {
        in_len = fread(inbuf, 1, INBUF_SIZE, input);
        in_pos = 0;
        if (in_len == 0)
            return EOF;
    }
    return inbuf[in_pos++];
}

static inline void put_byte(uint8_t b, FILE* output)
{
    if (out_pos >= OUTBUF_SIZE)
    {
        fwrite(outbuf, 1, OUTBUF_SIZE, output);
        out_pos = 0;
    }
    outbuf[out_pos++] = b;
}

static inline void flush_output(FILE* output)
{
    if (out_pos > 0)
    {
        fwrite(outbuf, 1, out_pos, output);
        out_pos = 0;
    }
}