#include "lzss.h"

// global state with static storage duration and internal linkage
static compressor global_compressor;

int main()
{
    compress(&global_compressor, stdin, stdout);
}