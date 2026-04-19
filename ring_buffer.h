/*
 * ring_buffer.h 
 * Provides the necessary header file for functions to manage a sample ring buffer.
 * Copyright Chris Schibi 2025
 * 
 * */
 
#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stddef.h>

#define RING_BUFFER_RECORD_SEP ','

// A simple ring (circular) buffer for storing characters.
typedef struct {
    char *buffer;      // dynamically allocated buffer
    size_t capacity;   // maximum number of elements
    size_t head;       // index where new data will be written
    size_t tail;       // index where data will be read from
    size_t count;      // current number of elements stored
} RingBuffer;

// Basic functions
int ring_buffer_init(RingBuffer *rb, size_t capacity);
void ring_buffer_free(RingBuffer *rb);
int ring_buffer_write(RingBuffer *rb, char data);
int ring_buffer_read(RingBuffer *rb); // Returns next char or -1 if empty.
size_t ring_buffer_size(RingBuffer *rb);
size_t ring_buffer_capacity(RingBuffer *rb);
int ring_buffer_is_empty(RingBuffer *rb);
int ring_buffer_is_full(RingBuffer *rb);
void ring_buffer_clear(RingBuffer *rb);

// Extended functions

// Peek at the next element without removing it.
// Returns the element or -1 if empty.
int ring_buffer_peek(RingBuffer *rb);

// Return the number of available slots for writing.
size_t ring_buffer_available_space(RingBuffer *rb);

// Write multiple characters (bulk write) into the ring buffer.
// Returns 0 on success or -1 on error.
int ring_buffer_bulk_write(RingBuffer *rb, const char *data, size_t len);

// Read up to 'len' characters from the ring buffer into dest.
// Returns the number of characters actually read, or -1 on error.
int ring_buffer_bulk_read(RingBuffer *rb, char *dest, size_t len);

// Return the usage percentage of the ring buffer.
double ring_buffer_usage_percent(RingBuffer *rb);

// Resize the ring buffer to a new capacity. This operation copies the
// current data into a new buffer (preserving order) and may truncate older data
// if the new capacity is smaller than the current number of elements.
// Returns 0 on success or -1 on error.
int ring_buffer_resize(RingBuffer *rb, size_t new_capacity);

// Write data as a record: appends len bytes then RING_BUFFER_RECORD_SEP.
// Returns 0 on success, -1 on error.
int ring_buffer_write_record(RingBuffer *rb, const char *data, size_t len);

#endif // RING_BUFFER_H
