/*
 * ringbuffer test file
 * Copyright Chris Schibi 2025
 * 
 * */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <syslog.h>
#include "ring_buffer.h"

void print_menu() {
    printf("\nCommands:\n");
    printf("  w <data>   - Write data (string) to ring buffer (bulk write)\n");
    printf("  r          - Read one character from ring buffer\n");
    printf("  R <n>      - Bulk read n characters from ring buffer\n");
    printf("  k          - Peek at the next character in ring buffer\n");
    printf("  s          - Show current size of ring buffer\n");
    printf("  a          - Show available space and usage percentage\n");
    printf("  p          - Print all contents of ring buffer\n");
    printf("  c          - Clear the ring buffer\n");
    printf("  z <cap>    - Resize the ring buffer to new capacity\n");
    printf("  q          - Quit\n");
    //printf("Enter command: ");
}

void print_buffer(RingBuffer *rb) {
    if (ring_buffer_is_empty(rb)) {
        printf("Ring buffer is empty.\n");
        return;
    }
    printf("Buffer contents:\n");
    size_t idx = rb->tail;
    for (size_t i = 0; i < rb->count; i++) {
        char c = rb->buffer[idx];
        printf("%c", c == RING_BUFFER_RECORD_SEP ? '\n' : c);
        idx = (idx + 1) % rb->capacity;
    }
    printf("\n");
}

int main(int argc, char *argv[]) {
    
    struct timeval start, end;
    struct timezone tz;
    double elapsed_sec;
    size_t capacity = 100; // default capacity
    
    
    //Set the time zone
    setenv("TZ","CST6CDT",1);
    // tzset();
    
    //open the connection to syslog
    openlog("ring_buffer_app",LOG_PID | LOG_CONS, LOG_USER);
    //Start Logging operation
    syslog(LOG_INFO, "Begin Log for ring_buffer_app with cap 100.");
        
    if (argc > 1) {
        capacity = (size_t) atoi(argv[1]);
        if (capacity == 0) {
            fprintf(stderr, "Invalid capacity provided. Using default capacity of 10.\n");
            capacity = 10;
        }
    }
    
    RingBuffer rb;
    if (ring_buffer_init(&rb, capacity) != 0) {
        fprintf(stderr, "Failed to initialize ring buffer with capacity %zu\n", capacity);
        return EXIT_FAILURE;
    }
    
    printf("Ring Buffer initialized with capacity %zu.\n", capacity);
    
    char input[256];
    
    // Main processing loop
    
    while (1) {
        printf("Enter command: ");
       // print_menu();
        if (!fgets(input, sizeof(input), stdin)) {
            break;
        }
        // Remove the trailing newline character
        input[strcspn(input, "\n")] = '\0';
        if (strlen(input) == 0) continue;
        
        char command = input[0];
        if (command == 'q') {
            break;
        } else if (command == 'w') {
            syslog(LOG_INFO, "write operation started");            
            // Bulk write: write the entire string after the command.
            char *data = input + 1;
            while (*data == ' ') data++;  // Skip spaces
            if (strlen(data) == 0) {
                printf("No data provided for writing.\n");
                syslog(LOG_DEBUG, "No Data provided for write operation");
            } else {
                // Get Start Tiime
				gettimeofday(&start, &tz);
                if (ring_buffer_write_record(&rb, data, strlen(data)) != 0) {
                    printf("Error writing data to ring buffer.\n");
                } else {
                    printf("Data written to ring buffer.\n");
                }
				gettimeofday(&end,&tz);
				elapsed_sec = (end.tv_sec - start.tv_sec) +
				(end.tv_usec - start.tv_usec) / 1000000.0;
				printf("Write operation took %.6f seconds\n", elapsed_sec);
				syslog(LOG_INFO, "Write operation successful"); 
            }	
            						 
        } else if (command == 'r') {
            syslog(LOG_INFO, "read operation started."); 
            gettimeofday(&start, &tz);
            int ch = ring_buffer_read(&rb);
            if (ch == -1) {
                printf("Ring buffer is empty or an error occurred.\n");
            } else {
                printf("Read character: %c\n", ch);
				gettimeofday(&end,&tz);
				elapsed_sec = (end.tv_sec - start.tv_sec) +
					(end.tv_usec - start.tv_usec) / 1000000.0;
				printf("Read operation took %.6f seconds\n", elapsed_sec);
				syslog(LOG_INFO, "read operation complete."); 
            }
				
        } else if (command == 'R') {
            gettimeofday(&start, &tz);
            syslog(LOG_INFO, "bulk read operation started"); 
            // Bulk read: user specifies number of characters to read.
            char *num_str = input + 1;
            while (*num_str == ' ') num_str++;  // Skip spaces
            if (strlen(num_str) == 0) {
                printf("No number provided for bulk read.\n");
            } else {
                size_t num = (size_t) atoi(num_str);
                if (num == 0) {
                    printf("Invalid number provided.\n");
                } else {
                    char *dest = (char*) malloc(num + 1);
                    if (!dest) {
                        printf("Memory allocation error.\n");
                        continue;
                    }
                    int read_count = ring_buffer_bulk_read(&rb, dest, num);
                    dest[read_count] = '\0';
                    printf("Bulk read (%d chars): %s\n", read_count, dest);
                    free(dest);
                    gettimeofday(&end,&tz);
					elapsed_sec = (end.tv_sec - start.tv_sec) +
						(end.tv_usec - start.tv_usec) / 1000000.0;
					printf("Build Read operation took %.6f seconds\n", elapsed_sec);
					syslog(LOG_INFO, "bulk read operation complete."); 
                }
            }
        } else if (command == 'k') {
            syslog(LOG_INFO, "peek operation requested."); 
            int ch = ring_buffer_peek(&rb);
            if (ch == -1) {
                printf("Ring buffer is empty or an error occurred.\n");
            } else {
                printf("Next character (peek): %c\n", ch);
            }
        } else if (command == 's') {
            printf("Current buffer size: %zu\n", ring_buffer_size(&rb));
        } else if (command == 'a') {
            syslog(LOG_INFO, "available space request initiated."); 
            size_t available = ring_buffer_available_space(&rb);
            double usage = ring_buffer_usage_percent(&rb);
            printf("Available space: %zu, Usage: %.2f%%\n", available, usage);
        } else if (command == 'p') {
            syslog(LOG_INFO, "print buffer contents initiated."); 
            print_buffer(&rb);
        } else if (command == 'm') {
           print_menu();
        } 
        else if (command == 'c') {
            syslog(LOG_INFO, "clear buffer request initiated."); 
            gettimeofday(&start, &tz);
            ring_buffer_clear(&rb);
            gettimeofday(&end,&tz);
					elapsed_sec = (end.tv_sec - start.tv_sec) +
						(end.tv_usec - start.tv_usec) / 1000000.0;
			printf("Ring Buffer clear operation took %.6f seconds\n", elapsed_sec);
            //printf("Ring buffer cleared.\n");
        } else if (command == 'z') {
            syslog(LOG_INFO, "resize ring buffer request initiated."); 
            // Resize command: expects new capacity after the command letter.
            char *cap_str = input + 1;
            while (*cap_str == ' ') cap_str++;  // Skip spaces
            if (strlen(cap_str) == 0) {
                printf("No new capacity provided for resize.\n");
            } else {
                size_t new_cap = (size_t) atoi(cap_str);
                if (new_cap == 0) {
                    printf("Invalid new capacity.\n");
                } else if (ring_buffer_resize(&rb, new_cap) != 0) {
                    printf("Error resizing the ring buffer.\n");
                    syslog(LOG_INFO, "resize request failed."); 
                } else {
                    printf("Ring buffer resized to capacity %zu.\n", new_cap);
                    syslog(LOG_INFO, "resize request completed."); 
                }
            }
        } else {
            printf("Unknown command. Please try again.\n");
        }
    }
    
    ring_buffer_free(&rb);
    syslog(LOG_INFO,"stop log for ring_buffer_app");
    closelog();
    return EXIT_SUCCESS;
}
