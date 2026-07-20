#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>

#define BUFFER_SIZE 32

#define NUM_SYMBOLS 32

// use extra symbol values for nodes
// should be >= 2 * NUM_SYMBOLS - 1
#define MAX_EXTRA_SYMBOLS 64

typedef struct 
{
    uint16_t symbol;

    uint16_t weight;

} node;

// package together basic items of a queue
typedef struct
{
    node* data;
    size_t start;
    size_t end; // refers to index after the last element
} node_queue;



// van Leeuwen (1976) two-queue algorithm
// https://webspace.science.uu.nl/~leeuw112/huffman.pdf
// takes as input leaf_queue array indexed [0, leaf_end)
// outputs tree in terms of parents array
void build_huffman(const node leaf_queue[], const size_t leaf_end, uint16_t parent[])
{
    // TODO: assert leaf_queue is in order

    // don't handle no leaves at all case
    assert(leaf_end > 0); 

    // if only a single leaf in queue, return nothing changed

    if (leaf_end == 1) 
        return;



    size_t leaf_start = 0;

    // track index to produce new nodes
    int next_node_index = NUM_SYMBOLS;

    node internal_queue[MAX_EXTRA_SYMBOLS];

    // indices tracking queue head and tail
    size_t internal_start = 0;
    size_t internal_end = 0; // end is index to element AFTER last


    // loop until leaf queue is empty and internal queue has only 1 node
    while (!(leaf_start == leaf_end && internal_end - internal_start == 1))
    {
        assert (leaf_start <= leaf_end && internal_start <= internal_end);

        printf("Leaf queue start %d end %d\n", leaf_start, leaf_end);



        printf("Internal queue start %d end %d\n", internal_start, internal_end);

        // TODO: use an extract_min twice instead of trying to do
        // both simultaneously here

        // determine alpha and beta, the two smallest nodes
        // from the two queues, based on what is available

        int least_weight = UINT16_MAX;

        bool is_alpha_leaf;
        bool is_beta_leaf;
        node alpha;
        node beta;

        // case 1: both from leaf queue
        if (leaf_end - leaf_start >= 2)
        {
            node a = leaf_queue[leaf_start];
            node b = leaf_queue[leaf_start + 1];
            int new_weight = a.weight + b.weight;

            if (new_weight < least_weight)
            {
                least_weight = new_weight;
                alpha = a;
                beta = b;
                is_alpha_leaf = true;
                is_beta_leaf = true;
            }
        }

        // case 2: one from each queue
        if (leaf_end - leaf_start >= 1 && internal_end - internal_start >= 1)
        {

        }

        // case 3: both from internal queue
        if (internal_end - internal_start >= 2)
        {

        }

        // create new internal node gamma
        node gamma = (node) 
        {
            .symbol = next_node_index,
            .weight = least_weight,
        };

        next_node_index++;

        // push to internal queue
        internal_queue[internal_end++] = gamma;

        // set alpha and beta parent as gamma
        parent[alpha.symbol] = gamma.symbol;


        // remove alpha and beta from their source queues
        if (is_alpha_leaf)
        {
            leaf_start++;
        } else {
            internal_start++;
        }

        if (is_beta_leaf)
        {
            leaf_start++;
        } else {
            internal_start++;
        }
        




    }



}

int main()
{
    node leaf_queue[NUM_SYMBOLS];



    uint16_t leaf_start = 0;
    uint16_t leaf_end = 5;
}
