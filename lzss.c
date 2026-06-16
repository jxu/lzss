#include "lzss.h"

// circular buffer
static char buffer[BUFFER_SIZE]; 

// stores up to 8 tokens of 3 bytes each
static char output_buffer[8 * 3];

// brute-force search for best match through window
// by trying every offset and matching as much as possible
// returns best offset, also returns through pointer best length
int search(int pos, unsigned max_pos, int* best_length)
{
    int best_offset = 0;
    *best_length = 0;

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
            best_offset = offset;
        }
    }
    return best_offset;
}

void compress_stream(FILE* input, FILE* output)
{
    // clear buffer for new compress
    memset(buffer, 0, BUFFER_SIZE);

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

        offset = search(pos, end_pos, &length);

        // good match length, we want to save more than 3 bytes
        if (length >= 3) 
        {
            debug_print("push ref (%d,%d)\n", offset, length);
            pos += length;



            debug_print("\n");

            // write offset and length to output buffer, either 2 or 3 bytes
            // variable-length offset: if 0-127, write 0xxxxxxx
            // else up to 32767, write 1yyyyyyy xxxxxxxx
            // DEFLATE uses 1-32768
            // DEFLATE also uses length-3 instead for slightly more length
            // TODO: refactor into own small function
            if (offset < 128) 
            {
                output_buffer[op++] = offset;
            }
            else 
            {
                output_buffer[op++] = 0x80 | (offset >> 8);
                output_buffer[op++] = offset & 0xFF;
            }

            output_buffer[op++] = length;

            // mark one flag
            bitflags |= 1 << (tokens % 8);

            // read length bytes lookahead or until EOF
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
        fputc(bitflags, output);

        debug_print("output final bytes ");
        print_buffer(output_buffer, op);

        fwrite(output_buffer, op, 1, output);
    }

    debug_print("tokens %d\n", tokens);
}

// decompress routine
// returns 0 on success and 1 on failure
int decompress(FILE* input, FILE* output)
{
    // reset buffer to initial zero state
    memset(buffer, 0, BUFFER_SIZE);

    int pos = 0; // abstract buffer position (can be >= BUFFER_SIZE)
    // TODO: negative ok?

    while (1)
    {
        // read bitflags byte
        int c = fgetc(input);

        if (c == EOF) 
            return 0;

        unsigned char bitflags = (unsigned char)c;

        debug_print("Read bitflags %b\n", bitflags);

        // process (up to) next 8 tokens
        for (int i = 0; i < 8; ++i)
        {
            if (bitflags & (1 << i)) // ref pair
            {
                int offset = 0;

                // expect to read 2 or 3 bytes here, depending on offset size
                // TODO: test these
                int oa = fgetc(input);

                if (oa == EOF)
                {
                    fprintf(stderr, "Unexpected EOF");
                    return 1;
                }

                int ob = 0;

                if (oa < 0x80) // one byte offset
                {
                    offset = oa;
                }
                else // two byte offset
                {
                    ob = fgetc(input);
                    offset = ((oa & 0x7f) << 8) | ob;
                }

                int length = fgetc(input);

                if ((ob == EOF) || (length == EOF))
                {
                    fprintf(stderr, "Unexpected EOF");
                    return 1;
                }

                debug_print("Read offset %d length %d\n", offset, length);

                int back = pos - offset;
                int front = pos;

                // push and output one byte at a time
                while (front < pos + length)
                {
                    unsigned char b = buffer[back % BUFFER_SIZE];
                    buffer[front % BUFFER_SIZE] = b;
                    fputc(b, output);
                    debug_print("output %c\n", b);
                    debug_print("New back %d front %d\n", back, front);
                    
                    ++front;
                    ++back;

                    print_buffer(buffer, BUFFER_SIZE);
                }

                // move pos forward
                pos = front;
                debug_print("New pos %d\n", pos);


            }
            else // literal byte (or EOF)
            {
                int c = fgetc(input);

                // could be EOF if token count isn't multiple of 8
                if (c == EOF)
                    return 0;

                debug_print("Read literal %c\n", c);

                // output byte verbatim
                fputc(c, output);

                // push to buffer
                buffer[pos % BUFFER_SIZE] = c;
                ++pos;
                print_buffer(buffer, BUFFER_SIZE);
                debug_print("New pos %d\n", pos);

            }
        }

    }

}
