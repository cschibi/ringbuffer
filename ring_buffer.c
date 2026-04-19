/*
 * ring_buffer.c 
 * Provides the necessary functions to manage a sample ring buffer.
 * Copyright Chris Schibi 2025
 * 
 * */

#include "ring_buffer.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int ring_buffer_init(RingBuffer *rb, size_t capacity) {
    if (rb == NULL || capacity == 0) {
        return -1;
    }
    rb->buffer = (char*) malloc(capacity * sizeof(char));
    if (rb->buffer == NULL) {
        return -1;
    }
    rb->capacity = capacity;
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
    return 0;
}

void ring_buffer_free(RingBuffer *rb) {
    if (rb && rb->buffer) {
        free(rb->buffer);
        rb->buffer = NULL;
    }
}

int ring_buffer_is_empty(RingBuffer *rb) {
    if (rb == NULL) {
        return 1;
    }
    return (rb->count == 0);
}

int ring_buffer_is_full(RingBuffer *rb) {
    if (rb == NULL) {
        return 0;
    }
    return (rb->count == rb->capacity);
}

int ring_buffer_write(RingBuffer *rb, char data) {
    if (rb == NULL || rb->buffer == NULL) {
        return -1; // error: not initialized
    }
    // If full, truncate the oldest data by advancing the tail pointer.
    if (ring_buffer_is_full(rb)) {
        rb->tail = (rb->tail + 1) % rb->capacity;
        rb->count--;
    }
    // Write new data at the head position.
    rb->buffer[rb->head] = data;
    rb->head = (rb->head + 1) % rb->capacity;
    rb->count++;
    return 0;
}

int ring_buffer_read(RingBuffer *rb) {
    if (rb == NULL || rb->buffer == NULL || ring_buffer_is_empty(rb)) {
        return -1; // error or empty buffer
    }
    int data = rb->buffer[rb->tail];
    rb->tail = (rb->tail + 1) % rb->capacity;
    rb->count--;
    return data;
}

size_t ring_buffer_size(RingBuffer *rb) {
    if (rb == NULL) {
        return 0;
    }
    return rb->count;
}

size_t ring_buffer_capacity(RingBuffer *rb) {
    if (rb == NULL) {
        return 0;
    }
    return rb->capacity;
}

void ring_buffer_clear(RingBuffer *rb) {
    if (rb == NULL) {
        return;
    }
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
}

// Extended Functions

int ring_buffer_peek(RingBuffer *rb) {
    if (rb == NULL || rb->buffer == NULL || ring_buffer_is_empty(rb)) {
        return -1;
    }
    return rb->buffer[rb->tail];
}

size_t ring_buffer_available_space(RingBuffer *rb) {
    if (rb == NULL) {
        return 0;
    }
    return rb->capacity - rb->count;
}

int ring_buffer_bulk_write(RingBuffer *rb, const char *data, size_t len) {
    if (rb == NULL || data == NULL) {
        return -1;
    }
    for (size_t i = 0; i < len; i++) {
        if (ring_buffer_write(rb, data[i]) != 0) {
            return -1;
        }
    }
    return 0;
}

int ring_buffer_bulk_read(RingBuffer *rb, char *dest, size_t len) {
    if (rb == NULL || dest == NULL) {
        return -1;
    }
    size_t i = 0;
    while (i < len && !ring_buffer_is_empty(rb)) {
        int ch = ring_buffer_read(rb);
        if (ch == -1) {
            break;
        }
        dest[i++] = (char) ch;
    }
    return (int)i; // number of characters read
}

double ring_buffer_usage_percent(RingBuffer *rb) {
    if (rb == NULL || rb->capacity == 0) {
        return 0.0;
    }
    return ((double) rb->count / rb->capacity) * 100.0;
}

int ring_buffer_dump_csv(RingBuffer *rb, const char *filename) {
    if (!rb || !filename) return -1;
    FILE *fp = fopen(filename, "w");
    if (!fp) return -1;

    fprintf(fp, "record\n");
    for (size_t i = 0; i < rb->count; i++) {
        char c = rb->buffer[(rb->tail + i) % rb->capacity];
        fputc(c == RING_BUFFER_RECORD_SEP ? '\n' : c, fp);
    }
    if (rb->count > 0) {
        char last = rb->buffer[(rb->tail + rb->count - 1) % rb->capacity];
        if (last != RING_BUFFER_RECORD_SEP)
            fputc('\n', fp);
    }

    fclose(fp);
    return 0;
}

int ring_buffer_write_record(RingBuffer *rb, const char *data, size_t len) {
    if (!rb || !data) return -1;
    int ret = ring_buffer_bulk_write(rb, data, len);
    if (ret != 0) return ret;
    return ring_buffer_write(rb, RING_BUFFER_RECORD_SEP);
}

int ring_buffer_resize(RingBuffer *rb, size_t new_capacity) {
    if (rb == NULL || new_capacity == 0) {
        return -1;
    }
    char *new_buffer = (char*) malloc(new_capacity * sizeof(char));
    if (new_buffer == NULL) {
        return -1;
    }
    // Determine the number of elements to copy: if the new capacity is less than the current count,
    // we only copy the most recent new_capacity elements.
    size_t to_copy = rb->count;
    if (to_copy > new_capacity) {
        to_copy = new_capacity;
        // Adjust tail to point to the first element to keep.
        rb->tail = (rb->tail + (rb->count - new_capacity)) % rb->capacity;
    }
    // Copy elements preserving order.
    for (size_t i = 0; i < to_copy; i++) {
        new_buffer[i] = rb->buffer[(rb->tail + i) % rb->capacity];
    }
    free(rb->buffer);
    rb->buffer = new_buffer;
    rb->capacity = new_capacity;
    rb->count = to_copy;
    rb->tail = 0;
    rb->head = to_copy % new_capacity;
    return 0;
}
