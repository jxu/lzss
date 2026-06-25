#include <stdint.h>
#include <string.h>
#include <assert.h>
#include "lzss.h"

// Main circular buffer, storing search window and lookahead window
// all positions are indexed mod buffer size
static uint8_t buffer[BUFFER_SIZE]; 

// Search hash table "dictionary" data structure, storing positions
static off_t search_dict[DICT_SIZE];

// stores previous pos in chain, indexes by current pos
static off_t prev_pos[BUFFER_SIZE];


// Pack next 3 bytes in buffer
uint32_t pack3(off_t pos)
{
    uint32_t a = buffer[pos % BUFFER_SIZE];
    uint32_t b = buffer[(pos+1) % BUFFER_SIZE];
    uint32_t c = buffer[(pos+2) % BUFFER_SIZE];
    return (a << 16) | (b << 8) | c;
}

// Knuth multiplicative hash, mod 2 ** DICT_BITS
uint32_t knuth_hash(uint32_t key)
{
    return ((uint32_t)2654435769 * key) >> (32 - DICT_BITS);
}

// insert key-pos pair into the front of the chain
// use Knuth key hash directly instead of key
void dict_insert(uint32_t hash, off_t pos)
{
    uint32_t bucket = hash; 
    assert(bucket < BUFFER_SIZE);

    off_t old_front = search_dict[bucket];
    // strictly decreasing or end of chain
    assert(old_front == NULL_POS || old_front < pos);
    search_dict[bucket] = pos;
    prev_pos[pos % BUFFER_SIZE] = old_front;

    debug_print("insert hash_table[%d] = %ld, next[%ld] = %ld\n",
        bucket, pos, pos % BUFFER_SIZE, old_front);
}

// returns best offset, also returns through pointer best length
size_t dict_search(uint32_t hash, off_t pos, off_t end_pos, size_t* best_length)
{
    size_t best_offset = 0;
    *best_length = 0;

    // if too close to the end, abort search
    if (end_pos - pos < KEY_LENGTH)
    {
        return 0;
    }

    uint32_t bucket = hash;
    assert(bucket < DICT_SIZE);
    off_t searchpos = search_dict[bucket]; // begin search

    // iterate through chain, searching for matches
    for (int i = 0; i < MAX_CHAIN_LENGTH; ++i)
    {
        debug_print("Searchpos %zu\n", searchpos);

        // reached end of chain
        if (searchpos == NULL_POS) 
            break;

        // all pos values not mod buffer

        size_t offset = pos - searchpos;
        off_t back = pos - offset;
        off_t fwd = pos;
        size_t length = 0;

        // break when chain decreasing position falls out of window
        if (offset > WINDOW_LENGTH)
        {
            debug_print("Break on offset %zu\n", offset);
            break;
        }

        // greedily match hot loop
        // trick: in matching, length can be greater than offset
        for (; length < LOOKAHEAD_LENGTH && fwd < end_pos; ++length)
        {
            // signed off_t modulo isn't as efficient, so use unsigned
            if (buffer[(uint64_t)back % BUFFER_SIZE] != 
                    buffer[(uint64_t)fwd % BUFFER_SIZE])
                break;

            ++back;
            ++fwd;
        }

        debug_print("Match %zu %zu\n", offset, length);

        if (length > *best_length)
        {
            *best_length = length;
            best_offset = offset;
        }

        // move to next
        off_t prev = prev_pos[(uint64_t)searchpos % BUFFER_SIZE];
        assert(searchpos != prev);
        searchpos = prev;
    }

    // If best offset returned is 0, ensure it's not a nonzero length match
    assert(!(best_offset == 0 && *best_length > 0));

    return best_offset;
}

// Reset dictionary tables
void dict_reset(void)
{
    for (size_t i = 0; i < DICT_SIZE; ++i)
        search_dict[i] = NULL_POS;

    for (size_t i = 0; i < BUFFER_SIZE; ++i)
        prev_pos[i] = NULL_POS;
}

// Compress input stream and write to output stream.
// Advances both streams's file positions.
void compress(FILE* input, FILE* output)
{
    // stores up to 8 tokens
    uint8_t output_buffer[8 * REF_MAX_SIZE];

    // Clear search window and buffers
    dict_reset();
    memset(buffer, 0, BUFFER_SIZE);

    // curren position index
    off_t pos = 0;

    // read initial LOOKAHEAD_LENGTH bytes (or up to EOF) into buffer
    // lookahead end position (pos after last byte)
    off_t end_pos = fread(buffer, 1, LOOKAHEAD_LENGTH, input);
    debug_print("Initial read %zu bytes\n", end_pos);

    off_t tokens = 0; // track tokens (literal or offset-length ref) outputted
    uint8_t bitflags = 0; // flags for 8 tokens at a time
    int op = 0; // output buffer pointer

    // main loop: iterate through current position in input
    // lookahead buffer end maintains ahead of pos, until it stops increasing
    // at pos == end_pos, where end_pos stopped due to EOF
    while(pos < end_pos)
    {
        debug_print("POS %zu\n", pos);

        uint8_t cur_c = buffer[pos % BUFFER_SIZE]; // current char

        // reference pair (offset, length)
        uint32_t hash = 0;
        size_t offset, length = 0;

        // Search if not too close to the end
        if (pos + KEY_LENGTH < end_pos)
        {
            hash = knuth_hash(pack3(pos));
            offset = dict_search(hash, pos, end_pos, &length);

            debug_print("Search pos %zu, found offset %zu length %zu\n", 
            pos, offset, length);
        }
        else 
        {
            offset = 0;
        }

        // starting here, pos is modified
        // good match length, enough to save
        if (length >= REF_MAX_SIZE) 
        {
            // write offset and length to output buffer, either 2 or 3 bytes
            // variable-length offset: if 0-127, write 0xxxxxxx
            // else up to 32767, write 1yyyyyyy xxxxxxxx
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

            debug_print("push ref (%zu,%zu)\n", offset, length);

            // mark one flag
            bitflags |= 1 << (tokens % 8);

            // read length bytes lookahead or until EOF
            size_t bytes_read;
            for (bytes_read = 0; bytes_read < length; ++bytes_read)
            {
                int c = fgetc(input);
                if (c == EOF)
                    break;
                
                buffer[end_pos % BUFFER_SIZE] = c;
                ++end_pos;
            }

            debug_print("Bytes read %zu\n", bytes_read);
        }

        else // no match, output literal to buffer
        {
            length = 1; // used for moving forward

            // read new byte and store at end_pos
            int c = fgetc(input);

            if (c != EOF)
            {
                buffer[end_pos % BUFFER_SIZE] = c;
                ++end_pos;
            }

            // write literal to output buffer
            output_buffer[op++] = cur_c;
            debug_print("push literal '%c'\n", cur_c);

            // mark zero flag by doing nothing
        }

        // insert new keys up to but not including length bytes ahead
        for (size_t i = 0; i < length; ++i)
            {
                if (pos + KEY_LENGTH < end_pos)
                {
                    hash = knuth_hash(pack3(pos));
                    dict_insert(hash, pos);
                }
                ++pos;
            }
    
        ++tokens;

        // finally, if reached 8 tokens or end
        // output bitflags and output buffer tokens
        if ((tokens % 8 == 0) || (pos == end_pos))
        {
            fputc(bitflags, output);
            debug_print("output bitflags %08b\n", bitflags);

            // write output buffer to output stream
            fwrite(output_buffer, op, 1, output);
            debug_print("output %d bytes\n", op);

            op = 0; // reset output buffer
            bitflags = 0; // reset bitflags
        }

        debug_print("\n"); // end loop
    }
}

// Decompress input stream and write to output stream.
// Advances both streams's file positions.
// Returns status code for success or failure.
status decompress(FILE* input, FILE* output)
{
    // reset buffer to initial zero state
    memset(buffer, 0, BUFFER_SIZE);

    off_t pos = 0; // abstract buffer position

    while (1)
    {
        debug_print("POS %zu\n", pos);

        // read bitflags byte
        int c = fgetc(input);

        if (c == EOF) // valid to EOF here
            return STATUS_SUCCESS; 

        uint8_t bitflags = c;

        debug_print("Read bitflags %b\n", bitflags);

        // process (up to) next 8 tokens
        for (int i = 0; i < 8; ++i)
        {
            if (bitflags & (1 << i)) // ref pair
            {
                size_t offset = 0;

                // expect to read 2 or 3 bytes here, depending on offset size
                int oa = fgetc(input);
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

                if ((oa == EOF) || (ob == EOF) || (length == EOF))
                {
                    fprintf(stderr, "Unexpected EOF when reading ref\n");
                    return STATUS_FAIL;
                }

                debug_print("Read offset %zu length %d\n", offset, length);

                off_t back = pos - offset;
                off_t front = pos;

                // push and output one byte at a time
                while (front < pos + length)
                {
                    uint8_t b = buffer[back % BUFFER_SIZE];
                    buffer[front % BUFFER_SIZE] = b;
                    fputc(b, output);
                    debug_print("output %c\n", b);
                    
                    ++front;
                    ++back;
                }

                // move pos forward
                pos = front;
            }
            else // literal byte (or EOF)
            {
                int c = fgetc(input);

                // valid to EOF if token count isn't multiple of 8
                if (c == EOF)
                    return STATUS_SUCCESS;

                debug_print("Read literal %c\n", c);

                // output byte verbatim
                fputc(c, output);

                // push to buffer
                buffer[pos % BUFFER_SIZE] = c;
                ++pos;
            }
        }
    }
}
