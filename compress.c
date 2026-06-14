#include <string.h>
#include <stdio.h>

// use buffer for search and lookahead
// power of 2, should be >= WINDOW_LENGTH + LOOKAHEAD_LENGTH
#define BUFFER_SIZE 64
#define WINDOW_LENGTH 16
#define LOOKAHEAD_LENGTH 16
#define REF_SIZE 3 // size of offset-length pair reference

#define DEBUG 1 // turn into #ifdef?

#define debug_print(...) \
            do { if (DEBUG) fprintf(stderr, __VA_ARGS__); } while (0)

// TODO: circular buffer, default init to zero
// for starting off, just assume buffer is big enough to hold all
char buffer[BUFFER_SIZE]; 

// stores up to 8 tokens 
char output_buffer[8 * REF_SIZE];

// debugging buffer visualization
void print_buffer(void)
{
    for (int i = 0; i < BUFFER_SIZE; ++i)
    {
        debug_print("%c", buffer[i]);
    }
    debug_print("\n");
}


// brute-force search for best match through window
// by trying every offset and matching as much as possible
// return best offset and length
void search(int pos, int max_pos, int* offset, int* length)
{
    int best_length = 0;
    int best_offset = 0;

    // try offset of i
    for (int i = 1; i <= WINDOW_LENGTH; ++i)
    {

        int back = (pos - i + BUFFER_SIZE) % BUFFER_SIZE;
        int fwd = pos; // forward match pos
        int match_length = 0;

        // greedily match
        // in matching, length can be greater than offset
        // TODO: check if end of buffer?
        for (; match_length < LOOKAHEAD_LENGTH; ++match_length)
        {
            // end-of-file
            if (fwd >= max_pos)
                break;


            if (buffer[back] != buffer[fwd]) // combine with for loop
                break;


            back = (back + 1) % BUFFER_SIZE;
            fwd = (fwd + 1) % BUFFER_SIZE;
        }

        if (match_length > best_length)
        {
            best_length = match_length;
            best_offset = i;
        }


    }

    // write out result
    *offset = best_offset;
    *length = best_length;
}

void compress_stream(FILE* input, FILE* output)
{
    int pos = 0;

    // read initial LOOKAHEAD_LENGTH bytes (or up to EOF) into buffer
    int count = fread(buffer, 1, LOOKAHEAD_LENGTH, input);

    // TODO: implement this
    // lookahead end, right after last byte
    int end_pos = count; 


    debug_print("Initial read %d bytes\n", count);

    print_buffer();

    int tokens = 0; // track tokens (literal or offset-length ref) outputted

    unsigned char bitflags = 0; // flags for 8 tokens at a time


    int op = 0; // output buffer pointer


    // main loop: iterate through current position in input
    // lookahead buffer end maintains ahead of pos, until it stops increasing
    // when it hits EOF
    // TODO: pos values can exceed the buffer length

    while(pos < end_pos)
    {
        char cur_c = buffer[pos]; // current char


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
                    buffer[end_pos] = c;
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
                buffer[end_pos++] = c;
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

            for (int i = 0; i < op; ++i)
            {
                debug_print("%02x ", output_buffer[i]);
            }
            debug_print("\n");

            fwrite(output_buffer, op, 1, output);

            op = 0; // reset output buffer
            bitflags = 0; // reset bitflags
        }
        
        // viz buffer
        print_buffer();
    }

    // output possible leftover tokens
    if (tokens % 8 != 0)
    {
        debug_print("output bitflags %08b\n", bitflags);
        
        debug_print("output final bytes ");

        for (int i = 0; i < op; ++i)
        {
            debug_print("%02x ", output_buffer[i]);
        }

        debug_print("\n");
    }

    debug_print("tokens %d\n", tokens);
}

int main()
{
    compress_stream(stdin, stdout);

}