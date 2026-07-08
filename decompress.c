#include "lzss.h"

static decompressor global_decompressor;

int main()
{
    decompress(&global_decompressor, stdin, stdout);
}