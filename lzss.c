#include <string.h>
#include <assert.h>
#include "lzss.h"

// main circular buffer, storing search window and lookahead window
// mod buffer size
static unsigned char buffer[BUFFER_SIZE]; 

// search dictionary related tables
static int hash_table[DICT_SIZE];

// stores next pos in chain, indexes by pos mod buffer
// all pos entries are distinct
// (match length is not stored and instead computed)
static int next_pos[BUFFER_SIZE];

// to make deletion constant time
static int prev_pos[BUFFER_SIZE];

// TODO: cleaner?
long pack3(int pos)
{

    int a = buffer[pos % BUFFER_SIZE];
    int b = buffer[(pos+1) % BUFFER_SIZE];
    int c = buffer[(pos+2) % BUFFER_SIZE];
    return (a << 16) | (b << 8) | c;
}

// TODO: better hash function
int hash(long key)
{
    unsigned long val = ((2654435769 * key) >> 15) % DICT_SIZE;
    debug_print("hash(0x%lx) = %ld\n", key, val);



    return val % DICT_SIZE;
}



// insert key-pos pair into the front of the chain
void dict_insert(long key, int pos)
{
    int bucket = hash(key) % DICT_SIZE;
    int old_front = hash_table[bucket];
    hash_table[bucket] = pos;
    next_pos[pos % BUFFER_SIZE] = old_front;

    debug_print("insert hash_table[%d] = %d, next[%d] = %d\n",
        bucket, pos, pos % BUFFER_SIZE, old_front);

    if (old_front != NULL_POS)
    {
        prev_pos[old_front % BUFFER_SIZE] = pos;
        debug_print("prev[%d] = %d\n", old_front % BUFFER_SIZE, pos);
    }


}

// delete directly by pos table
void dict_delete(int pos)
{

    assert(pos >= 0);
    assert(pos != NULL_POS);

    debug_print("Delete %d\n", pos);


    int prev = prev_pos[pos % BUFFER_SIZE];
    int next = next_pos[pos % BUFFER_SIZE];

    if (prev != NULL_POS)
    {
        assert(prev % BUFFER_SIZE != next);
        next_pos[prev % BUFFER_SIZE] = next;
        debug_print("set next[%d] = %d\n", prev % BUFFER_SIZE, next);

    }

    if (next != NULL_POS)
    {
        assert(next % BUFFER_SIZE != prev);

        prev_pos[next % BUFFER_SIZE] = prev;
        debug_print("set prev[%d] = %d\n", next % BUFFER_SIZE, prev);

    }

}

// TODO: search needs to check the match. this comes with computing length

// returns best offset, also returns through pointer best length
int dict_search(int pos, int max_pos, int* best_length)
{
    int best_offset = 0;
    *best_length = 0;

    // if too close to the end, abort search
    if (max_pos - pos < KEY_LENGTH)
    {
        return 0;
    }

    long key = pack3(pos);

    int bucket = hash(key) % DICT_SIZE;
    int searchpos = hash_table[bucket]; // begin search


    // iterate through chain, searching for matches
    for (int i = 0; i < MAX_CHAIN_LENGTH; ++i)
    {
        debug_print("Searchpos %d\n", searchpos);
        // reached end of chain
        if (searchpos == NULL_POS) 
        {
            break;
        }


        // offset computed from pos
        int offset = pos - searchpos;


        // these values are based on pos, not mod buffer size
        int back = pos - offset;
        int fwd = pos;
        int length = 0;





        // break when monotonically decreasing chain falls out of window
        if (offset > WINDOW_LENGTH)
        {
            debug_print("Break on offset %d\n", offset);
            break;

        }



        // greedily match
        // trick: in matching, length can be greater than offset
        for (; length < LOOKAHEAD_LENGTH && fwd < max_pos; ++length)
        {
            if (buffer[back % BUFFER_SIZE] != buffer[fwd % BUFFER_SIZE])
                break;

            ++back;
            ++fwd;
        }

        debug_print("Match %d %d\n", offset, length);

        if (length > *best_length)
        {
            *best_length = length;
            best_offset = offset;
        }

        // move to next
        assert(searchpos != next_pos[searchpos % BUFFER_SIZE]);
        searchpos = next_pos[searchpos % BUFFER_SIZE];
    }

    assert(!(best_offset == 0 && *best_length > 0));

    return best_offset;
}

void dict_reset(void)
{
    for (int i = 0; i < DICT_SIZE; ++i)
        hash_table[i] = NULL_POS;

    for (int i = 0; i < BUFFER_SIZE; ++i)
    {
        next_pos[i] = NULL_POS;
        prev_pos[i] = NULL_POS;
    }

}

void compress_stream(FILE* input, FILE* output)
{
    // stores up to 8 tokens
    char output_buffer[8 * REF_MAX_SIZE];

    // TODO: clear all tables!
    dict_reset();


    // clear buffer for new compress
    memset(buffer, 0, BUFFER_SIZE);

    // position index, can be larger than buffer size.
    int pos = 0;

    // read initial LOOKAHEAD_LENGTH bytes (or up to EOF) into buffer
    int count = fread(buffer, 1, LOOKAHEAD_LENGTH, input);
    debug_print("Initial read %d bytes\n", count);

    // lookahead end position (pos after last byte)
    int end_pos = count; 

    int tokens = 0; // track tokens (literal or offset-length ref) outputted

    unsigned char bitflags = 0; // flags for 8 tokens at a time

    int op = 0; // output buffer pointer

    // main loop: iterate through current position in input
    // lookahead buffer end maintains ahead of pos, until it stops increasing
    // at pos == end_pos, where end_pos stopped due to EOF
    while(pos < end_pos)
    {
        debug_print("POS %d\n", pos);

        char cur_c = buffer[pos % BUFFER_SIZE]; // current char

        debug_print("Hash table\n");
        for (int i = 0; i < DICT_SIZE; ++i)
            debug_print("%d ", hash_table[i]);
        debug_print("\n");

        debug_print("Next and prev table\n");
        for (int i = 0; i < BUFFER_SIZE; ++i)
            debug_print("%02d ", next_pos[i]);
        debug_print("\n");
        for (int i = 0; i < BUFFER_SIZE; ++i)
            debug_print("%02d ", prev_pos[i]);
        debug_print("\n");


        // reference pair (offset, length)
        int offset, length;

        // TODO: don't hash twice

        offset = dict_search(pos, end_pos, &length);

        debug_print("Search pos %d, found offset %d length %d\n", 
            pos, offset, length);
    

        // starting here, pos is modified
        // good match length, enough to save
        if (length >= REF_MAX_SIZE) 
        {

            // insert new keys and delete old keys for length bytes
            for (int i = 0; i < length; ++i)
            {

                if (end_pos - pos >= KEY_LENGTH)
                {
                    dict_insert(pack3(pos), pos);

                }


                // delete old key if fell out of search window
                int back = pos - WINDOW_LENGTH;
                if (back >= 0)
                {
                    //dict_delete(back);
                }

                ++pos;
            }

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

            debug_print("push ref (%d,%d)\n", offset, length);

            // mark one flag
            bitflags |= 1 << (tokens % 8);

            // read length bytes lookahead or until EOF
            int bytes_read;
            for (bytes_read = 0; bytes_read < length; ++bytes_read)
            {
                int c = fgetc(input);
                if (c == EOF)
                    break;
                
                buffer[end_pos % BUFFER_SIZE] = c;
                ++end_pos;
            }

            assert(pos <= end_pos);

            debug_print("Bytes read %d\n", bytes_read);
        }

        else // no match, output literal to buffer
        {
            // insert new key if possible
            if (end_pos - pos >= KEY_LENGTH)
            {
                dict_insert(pack3(pos), pos);

            }
            
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

            // delete old key if fell out of search window
            int back = pos - WINDOW_LENGTH;
            if (back >= 0)
            {
                //dict_delete(back);
            }


            ++pos;

        }
    
        ++tokens;

        // if reached 8 tokens, output bitflags and 8 tokens
        if (tokens % 8 == 0)
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

    // output any possible leftover tokens
    if (tokens % 8 != 0)
    {
        fputc(bitflags, output);
        debug_print("output bitflags %08b\n", bitflags);

        fwrite(output_buffer, op, 1, output);
        debug_print("output %d final bytes\n", op);
    }

}

// decompress routine
// returns 0 on success and 1 on failure
int decompress(FILE* input, FILE* output)
{
    // reset buffer to initial zero state
    memset(buffer, 0, BUFFER_SIZE);

    int pos = 0; // abstract buffer position (can be >= BUFFER_SIZE)

    while (1)
    {
        debug_print("Pos at loop start: %d", pos);

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
                    
                    ++front;
                    ++back;
                }

                // move pos forward
                pos = front;
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
            }
        }
    }
}
