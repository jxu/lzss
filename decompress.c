// Simple decompression program
// Usage: gcc -O2 -Wall -Wextra -o decompress decompress.c 
// ./decompress < sam.lzss > sam.new 2> decompress.log

#include <string.h>
#include <stdio.h>

#define BUFFER_SIZE 65536 // probably should match compress program

#define DEBUG 0 // turn into #ifdef?

#define debug_print(...) \
            do { if (DEBUG) fprintf(stderr, __VA_ARGS__); } while (0)


char buffer[BUFFER_SIZE];

// debugging buffer visualization: prints directly to stderr
void print_buffer(char* buf, size_t size)
{
    for (size_t i = 0; i < size; ++i)
    {
        debug_print("%c", buf[i]);
    }
    debug_print("\n");
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

int main()
{
    decompress(stdin, stdout);
}