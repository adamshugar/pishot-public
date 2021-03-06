#ifndef SONIC_RB_H
#define SONIC_RB_H

#include <stdbool.h>
#include "sonic.h"

/*
 * This module defines a ring buffer data structure that provides
 * a fixed-length FIFO (first-in-first-out) queue of int elements.
 *
 * The queue is designed to allow concurrent access by 1 reader (sonic_rb_dequeue)
 * and 1 writer (sonic_rb_enqueue). The writer is typically the interrupt handler,
 * who is enqueuing data to be dequeued by the main program, the reader.
 *
 * Author: Philip Levis <pal@cs.stanford.edu>
 * Author: Julie Zelenski <zelenski@cs.stanford.edu>
 *
 * Modified by Adam Shugar to support specific, heap-allocated, HC-SR04 sensor
 * data readout type as element type for Pi Shot project.
 */

typedef volatile struct sonic_rb sonic_rb_t;

/* 
 * Initializes a new empty ring buffer and returns a pointer to it.
 *
 * @return  pointer to the new ring buffer or NULL if cannot satisfy request.
 *
 * To set up a ring buffer in your code:
 *
 *     sonic_rb_t *rb = sonic_rb_new();
 *
 * Notice that this interface is slightly different from the _init exposed by
 * other libpi modules. This _new pattern allows a user to have multiple ring
 * buffers, like objects in Java. It also means that users of this
 * module don't need to know the implementation details (like size) of sonic_rb_t,
 * since they just keep a pointer. 
 */
sonic_rb_t *sonic_rb_new(void);

/* 
 * Check if a ring buffer is currently empty.
 *
 * @param   `sonic_rb_t` the ring buffer to check
 * @return  true if rb is empty, false otherwise
 */
bool sonic_rb_empty(sonic_rb_t *rb);

/* 
 * Check if a ring buffer is currently full. When full, existing
 * elements must first be dequeued before further elements can 
 * be enqueued.
 *
 * @param   `rb` the ring buffer to check
 * @return  true if rb is full, false otherwise
 */
bool sonic_rb_full(sonic_rb_t *rb);

/*
 * Add an element to the back of a ring buffer. If the ring buffer
 * is full, no changes are made and false is returned.
 *
 * @param   `rb` the ring buffer to enqueue to
 * @param   `elem` the element to enqueue
 * @return  true if elem was successfully enqueued, false otherwise
 */
bool sonic_rb_enqueue(sonic_rb_t *rb, sonic_data_t *elem);

/* 
 * If the ring buffer is not empty, remove frontmost element,
 * store into *p_elem, and return true.  p_elem should be the address
 * of a valid memory location into which to store the dequeued value.
 * If the ring buffer is empty, no changes are made to either the ring 
 * buffer or *p_elem and the return value is false.
 *
 * @param    `rb` the ring buffer to dequeue from
 * @param    `p_elem` address at which to store the dequeued element
 * @return   true if an element was written to *p_elem, false otherwise
 */
bool sonic_rb_dequeue(sonic_rb_t *rb, sonic_data_t **p_elem);

#endif
