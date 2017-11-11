/* A minimal FIFO queue implementation.
 * Author: Zach Carmichael
 */
#ifndef _QUEUE_H_
#define _QUEUE_H_

typedef struct queue_t
{
    unsigned int size;
    unsigned int capacity;
    int*         data;
    unsigned int i_front;
    unsigned int i_back;
} queue_t;

void queue_create(queue_t* queue, unsigned int size);
void queue_push(queue_t* queue, int data);
int  queue_pop(queue_t* queue);
void queue_destroy(queue_t* queue);
int  queue_full(queue_t* queue);
int  queue_empty(queue_t* queue);

#endif

