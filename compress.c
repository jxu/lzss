// Example usage
// gcc -O2 -Wall -Wextra -std=c99 -o compress compress.c 
// ./compress < sam.txt > sam.lzss 2> compress.log

#include <stdio.h>

// use buffer for search and lookahead
// BUFFER_SIZE should be power of 2 >= WINDOW_LENGTH + LOOKAHEAD_LENGTH
#define BUFFER_SIZE 32
#define WINDOW_LENGTH 16   // fit into 2 bytes
#define LOOKAHEAD_LENGTH 16 // fit into 1 byte
#define REF_SIZE 3 // size of offset-length pair reference

#define DEBUG 1 // turn into #ifdef?

#define debug_print(...) \
            do { if (DEBUG) fprintf(stderr, __VA_ARGS__); } while (0)

// circular buffer, default init to zero
char buffer[BUFFER_SIZE]; 

// stores up to 8 tokens 
char output_buffer[8 * REF_SIZE];

// debugging buffer visualization: prints directly to stderr
void print_buffer(char* buf, size_t size)
{
    
    for (size_t i = 0; i < size; ++i)
    {
        debug_print("%c", buf[i]);
    }
    debug_print("\n");
}


// brute-force search for best match through window
// by trying every offset and matching as much as possible
// return best offset and length
void search(int pos, unsigned max_pos, int* best_offset, int* best_length)
{
    *best_length = 0;
    *best_offset = 0;

    // try offset
    for (int offset = 1; offset <= WINDOW_LENGTH; ++offset)
    {
        // these values are based on pos, not mod buffer size
        unsigned back = pos - offset;
        unsigned fwd = pos;
        int length = 0;

        // greedily match
        // trick: in matching, length can be greater than offset
        for (; length < LOOKAHEAD_LENGTH && fwd < max_pos; ++length)
        {
            if (buffer[back % BUFFER_SIZE] != buffer[fwd % BUFFER_SIZE])
                break;

            ++back;
            ++fwd;
        }

        if (length > *best_length)
        {
            *best_length = length;
            *best_offset = offset;
        }
    }
}

void compress_stream(FILE* input, FILE* output)
{
    // position index, can be larger than buffer size.
    int pos = 0;

    // read initial LOOKAHEAD_LENGTH bytes (or up to EOF) into buffer
    int count = fread(buffer, 1, LOOKAHEAD_LENGTH, input);

    // lookahead end position (pos after last byte)
    int end_pos = count; 

    debug_print("Initial read %d bytes\n", count);
    print_buffer(buffer, BUFFER_SIZE);

    int tokens = 0; // track tokens (literal or offset-length ref) outputted

    unsigned char bitflags = 0; // flags for 8 tokens at a time

    int op = 0; // output buffer pointer

    // main loop: iterate through current position in input
    // lookahead buffer end maintains ahead of pos, until it stops increasing
    // at pos == end_pos, where end_pos stopped due to EOF
    while(pos < end_pos)
    {
        char cur_c = buffer[pos % BUFFER_SIZE]; // current char


        // reference pair (offset, length)
        int offset, length;

        search(pos, end_pos, &offset, &length);
        

        // good match length, we want to save more than REF_SIZE bytes
        if (length > REF_SIZE) 
        {
            debug_print("push ref (%d,%d)\n", offset, length);
            pos += length;

            // read length bytes ahead or until EOF
            for (int i = 0; i < length; ++i)
            {
                int c = fgetc(input);
                if (c == EOF)
                {
                    debug_print("Hit EOF\n");
                    break;
                }
                else 
                {
                    debug_print("Read %c\n", c);
                    buffer[end_pos % BUFFER_SIZE] = c;
                    ++end_pos;
                }
            }

            debug_print("\n");


            // write offset and length to output buffer
            // Idea: write offset as 1 or 2 byte ULEB128
            // Idea: write length-3 instead for slightly more length
            output_buffer[op] = offset & 0xFF; // little-endian
            output_buffer[op+1] = offset >> 8;
            output_buffer[op+2] = length;

            op += 3;

            // mark one flag
            bitflags |= 1 << (tokens % 8);

        }

        else // no match, output literal to buffer
        {
            debug_print("push literal '%c'\n", cur_c);
            ++pos;
            
            // read new byte
            int c = fgetc(input);

            if (c != EOF)
            {
                debug_print("read new '%c' to end pos %d\n", c, end_pos);
                buffer[end_pos % BUFFER_SIZE] = c;
                ++end_pos;
            }
            else 
            {
                debug_print("EOF, not changing end_pos\n");
            }

            // write literal to output buffer
            output_buffer[op++] = cur_c;

            // mark zero flag by doing nothing
        }
    
        ++tokens;

        // if reached 8 tokens, output bitflags and 8 tokens
        if (tokens % 8 == 0)
        {
            debug_print("output bitflags %08b\n", bitflags);
            fputc(bitflags, output);
            
            debug_print("output %d bytes ", op);

            print_buffer(output_buffer, op);

            // write output buffer to output stream
            fwrite(output_buffer, op, 1, output);

            op = 0; // reset output buffer
            bitflags = 0; // reset bitflags
        }
        
        // viz buffer
        print_buffer(buffer, BUFFER_SIZE);
    }

    // output any possible leftover tokens
    if (tokens % 8 != 0)
    {
        debug_print("output bitflags %08b\n", bitflags);
        debug_print("output final bytes ");

        print_buffer(output_buffer, op);

        fwrite(output_buffer, op, 1, output);
    }

    debug_print("tokens %d\n", tokens);
}

int main()
{
    compress_stream(stdin, stdout);

}