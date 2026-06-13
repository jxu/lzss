#include <string.h>
#include <stdio.h>

// use buffer for search and lookahead
#define BUFFER_SIZE 32 // power of 2
#define WINDOW_LENGTH 8
#define LOOKAHEAD_LENGTH 8

// TODO: circular buffer
char buffer[BUFFER_SIZE]; // default init to zero

char example[] = "I AM SAM SAM I AM THAT I AM\0";

// brute-force search for best match through window
// by trying every offset and matching as much as possible
// return best offset and length
void search(int pos, int* offset, int* length)
{
    int best_length = 0;
    int best_offset = 0;

    // try offset of i
    for (int i = 1; i < WINDOW_LENGTH; ++i)
    {

        int j = (pos - i + BUFFER_SIZE) % BUFFER_SIZE;
        int k = pos; // forward match pos
        int m = 0; // match length

        // in matching, length can be greater than offset
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

    // try every pos 
    for (int pos = 0; pos < 16; ++pos)
    {
        int best_offset, best_length;

        search(pos, &best_offset, &best_length);
        
        printf("pos %d (%d,%d)\n", pos, best_offset, best_length);
    
        // good match
        if (best_length >= 3)
        {

        }
        else // no match, output literal
        {


        }
    
    
    
    }



}