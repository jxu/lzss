#include <string.h>
#include <stdio.h>

// use buffer for search and lookahead
#define BUFFER_SIZE 64 // power of 2
#define WINDOW_LENGTH 16
#define LOOKAHEAD_LENGTH 16
#define REF_SIZE 3 // size of offset-length pair reference

// TODO: circular buffer
char buffer[BUFFER_SIZE]; // default init to zero

char example[] = "AAAAABCDEFG\0";

// brute-force search for best match through window
// by trying every offset and matching as much as possible
// return best offset and length
void search(int pos, int* offset, int* length)
{
    int best_length = 0;
    int best_offset = 0;

    // try offset of i
    for (int i = 1; i <= WINDOW_LENGTH; ++i)
    {

        int j = (pos - i + BUFFER_SIZE) % BUFFER_SIZE;
        int k = pos; // forward match pos
        int m = 0; // match length

        // in matching, length can be greater than offset
        // TODO: check if end of buffer?
        for (; m < LOOKAHEAD_LENGTH; ++m)
        {
            if (buffer[j] != buffer[k]) // combine with for loop
                break;


            j = (j + 1) % BUFFER_SIZE;
            k = (k + 1) % BUFFER_SIZE;
        }

        if (m > best_length)
        {
            best_length = m;
            best_offset = i;
        }


    }

    *offset = best_offset;
    *length = best_length;
}


int main()
{
    // for now, load entire file into buffer

    strcpy(buffer, example);

    int tokens = 0; // track tokens (literal or offset-length ref) outputted

    unsigned char bitflags = 0; // flags for 8 tokens at a time

    // stores up to 8 tokens 
    char output_buffer[8 * REF_SIZE];
    int op = 0; // output buffer pointer

    char c;
    // main loop: iterate through current positions 
    int pos = 0;
    while((c = buffer[pos]) != '\0')
    {
        int best_offset, best_length;

        search(pos, &best_offset, &best_length);
        
        //printf("search pos %d (%d,%d)\n", pos, best_offset, best_length);


        // good match, we want to save more than REF_SIZE bytes
        if (best_length > REF_SIZE) 
        {
            printf("push ref (%d,%d)\n", best_offset, best_length);
            pos += best_length;

            // write offset and length to output buffer
            output_buffer[op] = best_offset & 0xFF;
            output_buffer[op+1] = best_offset >> 8;
            output_buffer[op+2] = best_length;

            op += 3;

            // mark one flag
            bitflags |= 1 << (tokens % 8);

        }

        else // no match, output literal to buffer
        {
            printf("push literal '%c'\n", c);
            ++pos;

            // write literal to output buffer
            output_buffer[op++] = c;

            // mark zero flag by doing nothing
        }
    
        ++tokens;

        // if reached 8 tokens, output bitflags and 8 tokens
        // TODO: handle EOF
        if (tokens % 8 == 0)
        {
            printf("output bitflags %08b\n", bitflags);
            printf("output bytes ");

            for (int i = 0; i < op; ++i)
            {
                printf("%02x ", output_buffer[i]);
            }

            printf("\n");

            op = 0; // reset output buffer
            bitflags = 0; // reset bitflags
        }
    
    }

    // output possible leftover tokens
    if (tokens % 8 != 0)
    {

    }

    printf("tokens %d\n", tokens);

}