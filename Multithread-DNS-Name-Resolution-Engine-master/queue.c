// FIFO queue
// Author: Grant Novota
#include <stdlib.h>
#include "queue.h"

int initialize_queue(queue* q, int size) {

    if (size > 0) {
        q->max_size = size; // provided by user
    } else {
        q->max_size = MAXQUEUESIZE; // default
    }

    // malloc array
    q->array = malloc(sizeof(queue_node)*(q->max_size));
    
    // set NULL
    for (int i=0; i < q->max_size; ++i) {
        q->array[i].element = NULL;
    }

    // make circular
    q->front, q->back = 0;

    return q->max_size;
}

int queue_full(queue* q) {
    if((q->array[q->front].element != NULL) && (q->front == q->back)){
	    return 1; // is full
    }
    else {
	    return 0; // not full
    }
}

int queue_empty(queue* q) {
    if ((q->array[q->front].element == NULL) && (q->front == q->back)) {
	    return 1; // is empty
    }
    else {
	    return 0; // not empty
    }
}

int enqueue(queue* q, void* new_element) {
    if(queue_full(q)) {
	    return -1; // can't add
    }

    q->array[q->back].element = new_element;
    q->back = ((q->back+1) % q->max_size);

    return 0;
}

void* dequeue(queue* q) {
    void* return_element;
	
    if(queue_empty(q)) {
	    return NULL; // nothing to return
    }
	
    return_element = q->array[q->front].element;
    q->array[q->front].element = NULL;
    q->front = ((q->front + 1) % q->max_size);

    return return_element;
}

void queue_release(queue* q) { // clear queue & free memory
    while(!queue_empty(q)) {
	    dequeue(q);
    }

    free(q->array);
}