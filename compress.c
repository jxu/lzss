#include <string.h>
#include <stdio.h>

// use buffer for search and lookahead
#define BUFFER_SIZE 64 // power of 2
#define WINDOW_LENGTH 16
#define LOOKAHEAD_LENGTH 16
#define REF_SIZE 3 // size of offset-length pair reference

// TODO: circular buffer
char buffer[BUFFER_SIZE]; // default init to zero

char example[] = "I AM SAM I AM SAM SAM I AM\0";

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

    int tokens = 0; // track tokens (literal or offset-length pair) outputted
    int output_bytes = 0; // not necessary 
    int bitflags = 0; // flags for 8 tokens at a time

    // try every pos 
    int pos = 0;
    while(buffer[pos] != '\0')
    {
        int best_offset, best_length;

        search(pos, &best_offset, &best_length);
        
        //printf("search pos %d (%d,%d)\n", pos, best_offset, best_length);

        // good match
        // offset-distance pair is 3 bytes, so we want to save more than that
        if (best_length > 3) 
        {
            printf("output (%d,%d)\n", best_offset, best_length);
            pos += best_length;
            output_bytes += 3;
        }

        else // no match, output literal
        {
            printf("output '%c'\n", buffer[pos]);
            ++pos;
            ++output_bytes;
        }
    
        ++tokens;
    
    }

    printf("tokens %d\n", tokens);
    printf("output_bytes %d\n", output_bytes);



}