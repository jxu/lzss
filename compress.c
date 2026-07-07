#include "lzss.h"

// global state with static storage duration and internal linkage
static lzss_state global_state;

int main()
{
    compress(&global_state, stdin, stdout);
}