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
#define NUM_EXTRA_SYMBOLS 64

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


size_t queue_length(const node_queue* q)
{
    assert(q->start <= q->end);
    return q->end - q->start;
}

void queue_push(node_queue* q, const node n)
{
    assert(q->start <= q->end);
    q->data[q->end++] = n;
}

node queue_pop(node_queue* q)
{
    assert(q->start < q->end);
    return q->data[q->start++];
}

node queue_peek(const node_queue* q)
{
    assert(q->start < q->end);
    return q->data[q->start];
}

// Pop the min node across two queues
node pop_queues(node_queue* q1, node_queue* q2)
{
    // Assume at least one queue is non-empty
    assert(queue_length(q1) > 0 || queue_length(q2) > 0);

    if (queue_length(q1) == 0)
        return queue_pop(q2);
    
    if (queue_length(q2) == 0)
        return queue_pop(q1);
    
    // here, both queues are non-empty
    node n1 = queue_peek(q1);
    node n2 = queue_peek(q2);

    if (n1.weight < n2.weight) 
        return queue_pop(q1); 
    else
        return queue_pop(q2);
}

// van Leeuwen (1976) two-queue algorithm
// https://webspace.science.uu.nl/~leeuw112/huffman.pdf
// takes as input leaf_queue array indexed [0, leaf_queue.end)
// outputs tree in terms of parents array
void build_huffman(node_queue leaf_queue, uint16_t parent[])
{
    // TODO: assert leaf_queue is in order

    // don't handle no leaves at all case
    assert(leaf_queue.start == 0 && leaf_queue.end > 0); 

    // if only a single leaf in queue, return nothing changed

    if (leaf_queue.end == 1) 
        return;


    // track index to produce new nodes
    int next_node_index = NUM_SYMBOLS;

    node internal_data[NUM_EXTRA_SYMBOLS];

    node_queue internal_queue =
    {
        .data = internal_data, 
        .start = 0,
        .end = 0,
    };


    // loop until leaf queue is empty and internal queue has only 1 node
    while (!(leaf_queue.start == leaf_queue.end 
             && internal_queue.end - internal_queue.start == 1))
    {

        // TODO: use an extract_min twice instead of trying to do
        // both simultaneously here

        // determine alpha and beta, the two smallest nodes
        // from the two queues, based on what is available

        // these should not fail here

    }

}

int main()
{
    // very basic test of queue functions

    node data[NUM_EXTRA_SYMBOLS];

    // zero-init start and end
    node_queue q = {.data = data};

    assert(queue_length(&q) == 0);

    node n1 = {0, 1};
    queue_push(&q, n1);

    assert(queue_length(&q) == 1);

    // test peeking twice
    n1 = queue_peek(&q);
    n1 = queue_peek(&q);
    assert(n1.symbol == 0);

    n1 = queue_pop(&q);
    assert(queue_length(&q) == 0);
    assert(n1.symbol == 0);
}
