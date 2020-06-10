// queue header
// Author: Grant Novota
#ifndef QUEUE_H
#define QUEUE_H

#include <stdio.h>

#define MAXQUEUESIZE 50

typedef struct queue_node_struct{
    void* element;
} queue_node;

typedef struct queue_struct{
    int max_size;
    queue_node* array;
    int front;
    int back; 
} queue;

// initialize a queue
// returns queue size
int initialize_queue(queue* q, int size);

// returns 1 (true) if empty, 0 if not
int queue_empty(queue* q);

// returns 1 (true) if full, 0 if not
int queue_full(queue* q);

// adds element to end of queue
// returns 0 if successful, -1 if not
int enqueue(queue* q, void* element);

// returns element from the start of queue
// if queue is empty, it returns NULL
void* dequeue(queue* q);

// release queue from memory
void queue_release(queue* q);

#endif