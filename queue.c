/* A minimal FIFO queue implementation.
 * Author: Zach Carmichael
 */
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include "queue.h"

// Lock for modifying queue
pthread_mutex_t q_lock;


void queue_create(queue_t* queue, unsigned int size)
{
    if (queue->data = malloc(sizeof(unsigned int) * size))
    {
        fprintf(stderr, "Failed to malloc queue data.");
        exit(EXIT_FAILURE);
    }
    queue->capacity = size;
    queue->size = 0;
    queue->i_front = 0;
    queue->i_back = 0;
    // Create mutex
    pthread_mutex_init(&q_lock, NULL);
}


int queue_full(queue_t* queue)
{
    return queue->size == queue->capacity;
}


int queue_empty(queue_t* queue)
{
    return queue->size == 0;
}


int queue_push(queue_t* queue, int data)
{
    // Get lock
    pthread_mutex_lock(&q_lock);

    if (queue->size == queue->capacity)
        return -1;

    // Increase size
    queue->size += 1;
    // Append data
    queue->data[queue->i_back] = data;
    // Update back index
    if (queue->i_back < queue->capacity-1)
        ++(queue->i_back);
    else
        queue->i_back = 0;

    // Release lock
    pthread_mutex_unlock(&q_lock);

    return 0;
}


int queue_pop(queue_t* queue)
{
    int popped;

    // Get lock
    pthread_mutex_lock(&q_lock);

    if (queue->size == 0)
        return -1;

    // Decrement size
    queue->size -= 1;
    // Pop data
    popped = queue->data[queue->i_front];
    // Update front index
    if (queue->i_front < queue->capacity-1)
        ++(queue->i_front);
    else
        queue->i_front = 0;

    // Release lock
    pthread_mutex_unlock(&q_lock);

    return popped;
}


void queue_destroy(queue_t* queue)
{
    free(queue->data);
    // Destroy mutex
    pthread_mutex_destroy(&q_lock);
}

