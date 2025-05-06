#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>

#include "nalloc.h"

// Constants

const long CHUNK_SIZE = 4096;

const uint32_t NUMBER_OF_BLOCKS_IN_CHUNK = 512;

/*
 * For standarization and alignment reasons, bools will be treated as
 * 1 and 0's of the uint32_t type
 */

struct header {
    uint32_t is_occupied;
    uint32_t allocated_blocks;
};

// Global variables

struct header* heap_start;

// Pointer to the first byte outside of the program's memory range
struct header* out_of_bounds_start;
int number_of_chunks = 0;

void update_sorrounding_headers() {

    struct header* current_header = heap_start;

    uint64_t out_of_bounds_start_ptr = (uint64_t) out_of_bounds_start;

    while (1) {
        if ((uint64_t) current_header > out_of_bounds_start_ptr) {
            fprintf(stderr, "Warning, critical structural error\n");
            return;
        }

        struct header* current_headers_next = &current_header[current_header->allocated_blocks + 1];

        if (current_headers_next != out_of_bounds_start) {
            current_header = current_headers_next;
            continue;
        }

        if (current_header->is_occupied == 1) {      
            out_of_bounds_start->is_occupied = 0;
            out_of_bounds_start->allocated_blocks = NUMBER_OF_BLOCKS_IN_CHUNK - 1;
            return;
        }
        
        current_header->allocated_blocks += NUMBER_OF_BLOCKS_IN_CHUNK;
        return;
    }

}

/*
 * If the current number of chunks is 0
 * meaning is the first allocation
 * it will automatically set the
 * first_header and out_of_bounds variables
 * aswell as handle the number_of_pages variable in general
 *
 * will return 1 if there is an error present
 */

int get_new_chunk() {
    struct header* new_allocation_ptr = sbrk(CHUNK_SIZE);

    if (new_allocation_ptr == NULL) {
        return 1;
    }

    number_of_chunks ++;

    // sets up global variables if first allocation
    if (number_of_chunks == 1) {
        heap_start = new_allocation_ptr;
        out_of_bounds_start = (struct header*) ((uint64_t) heap_start + CHUNK_SIZE);
        return 0;
    }
    
    update_sorrounding_headers();

    out_of_bounds_start = (struct header*) ((uint64_t) out_of_bounds_start + CHUNK_SIZE);

    return 0;
}

void create_first_header() {
    // minus 1 for the current header
    struct header first_header = {0, NUMBER_OF_BLOCKS_IN_CHUNK - 1};
    *heap_start = first_header;
}

/*
 * Will mark a given memory space as occupied
 * and update any sorrounding memory blocks
 * accordingly
 */

void allocate_at(struct header* address, int size) {
    int original_header_size = address->allocated_blocks;

    struct header new_header = {1, size};
    *address = new_header;

    int next_header_size = original_header_size - size - 1;

    if (next_header_size == -1) {
        return;
    }

    struct header next_header = {0, next_header_size};
    address[size + 1] = next_header; 
}

// For debugging only
void show_structure() {
    struct header* current_header = heap_start;
    printf("heap: %p --> %p, size: %lu \n", heap_start, out_of_bounds_start, (uint64_t) out_of_bounds_start - (uint64_t) heap_start);

    while (current_header < out_of_bounds_start) {
        struct header* current_headers_next = &current_header[current_header->allocated_blocks + 1];
        printf("%p --> %p; difference: %lu \n", current_header, current_headers_next, (uint64_t) current_headers_next - (uint64_t) current_header);
        current_header = current_headers_next;
    }
}

void* nalloc(int allocation_size_in_bytes) {
    // The ceil of the divition by 8;
    int number_of_blocks = (allocation_size_in_bytes + 8 - 1) / 8;

    if(number_of_chunks == 0) {
        if (get_new_chunk() == 1) {
            return NULL;
        }
        create_first_header();
    }

    struct header* current_header = heap_start;

    while (1) {
        // Check if still in bounds
        if (current_header >= out_of_bounds_start) {
            if (get_new_chunk() == 1) {
                return NULL;
            }
            current_header = heap_start;
            continue;
        }

        if (current_header->is_occupied == 0 
        && current_header->allocated_blocks >= number_of_blocks) {
            allocate_at(current_header, number_of_blocks);
            return (void*) &(current_header[1]);
        }

        current_header = &current_header[current_header->allocated_blocks + 1];
    }
}

void nalloc_free(void* to_be_free_ptr){
    struct header* to_be_free_header_ptr = &((struct header*) to_be_free_ptr)[-1];

    if (to_be_free_header_ptr == heap_start) {
        to_be_free_header_ptr->is_occupied = 0;
        return;
    }  
    
    struct header* current_header = heap_start;

    while (1) {
        struct header* current_headers_next = &current_header[current_header->allocated_blocks + 1];

        if (current_header >= out_of_bounds_start) {
            fprintf(stderr, "Warning, critical structural error\n");
            return;
        }

        if (current_headers_next != to_be_free_header_ptr) {
            current_header = current_headers_next;
            continue;
        }

        // Rejoin the one after it

        struct header* header_after_freed = &current_headers_next[current_headers_next->allocated_blocks + 1];

        if (header_after_freed->is_occupied == 0) {
            current_headers_next->allocated_blocks += header_after_freed->allocated_blocks + 1;
        }

        if (current_header->is_occupied == 0) {
            current_header->allocated_blocks += to_be_free_header_ptr->allocated_blocks + 1;
            return;
        }

        to_be_free_header_ptr->is_occupied = 0;
        return;
    }

}
