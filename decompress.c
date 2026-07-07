#include "lzss.h"

// use same state as compression
static lzss_state global_state;

int main()
{
    decompress(&global_state, stdin, stdout);
}