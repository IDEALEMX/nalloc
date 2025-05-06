#include <assert.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>


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
struct header* out_of_bounds_start;
int number_of_chunks = 0;

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
    int number_of_new_chunks = pow(2, number_of_chunks);
    int new_memory_size_in_bytes = CHUNK_SIZE * number_of_new_chunks;
    struct header* new_allocation_ptr = sbrk(new_memory_size_in_bytes);

    if (new_allocation_ptr == NULL) {
        return 1;
    }

    number_of_chunks += number_of_new_chunks;

    // sets up global variables if first allocation
    if (number_of_chunks == 1) {
        heap_start = new_allocation_ptr;
        assert((unsigned long)heap_start % 8 == 0);
        out_of_bounds_start = heap_start + CHUNK_SIZE;
        return 0;
    }
    
    assert((unsigned long) out_of_bounds_start + (unsigned long) new_memory_size_in_bytes == (unsigned long) (out_of_bounds_start + new_memory_size_in_bytes));
    out_of_bounds_start += new_memory_size_in_bytes;   
    return 0;
}

void create_first_header() {
    struct header first_header = {0, NUMBER_OF_BLOCKS_IN_CHUNK};
    *heap_start = first_header;
    assert(heap_start->allocated_blocks == NUMBER_OF_BLOCKS_IN_CHUNK);
}

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

void* nalloc(int allocation_size_in_bytes) {
    // The ceil of the divition by 8;
    int number_of_blocks = (allocation_size_in_bytes + 8 - 1) / 8;

    if(number_of_chunks == 0) {
        get_new_chunk();
        create_first_header();
    }

    struct header* current_header = heap_start;

    while (1) {
        // Check if still in bounds
        if (current_header >= out_of_bounds_start) {
            get_new_chunk();
            // *todo* resize last empty header or add new empty header;
            continue;
            printf("had to resize \n");
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
            printf("Warning, critical structural error\n");
            return;
        }

        if (current_headers_next != to_be_free_header_ptr) {
            current_header = current_headers_next;
            continue;
        }

        if (current_header->is_occupied == 0) {
            current_header->allocated_blocks += to_be_free_header_ptr->allocated_blocks + 1;
            return;
        }

        to_be_free_header_ptr->is_occupied = 0;
        return;
    }

}

struct stest {
    long test1;
    long test2;
    long test3;
    long test4;
};

int main() {
    struct stest* test = nalloc(sizeof(struct stest));
    printf("value of size: %lu, stored: %p \n", sizeof(*test), test);

    printf("Heap beggins at: %p, with alloctated_blocks: %lu \n", heap_start, 
            (unsigned long) (heap_start->allocated_blocks));
   
    short* test2 = nalloc(sizeof(short));
    *test2 = 2;
    printf("value: %d, stored: %p \n", *test2, test2);

    struct stest* test3 = nalloc(sizeof(struct stest));
    printf("value of size: %lu, stored: %p \n", sizeof(*test3), test3);

    nalloc_free(test2);
    printf("pointer: %p was freed \n", test2);

    struct stest* test4 = nalloc(sizeof(struct stest));
    printf("value of size: %lu, stored: %p \n", sizeof(*test4), test4);

    short* test5 = nalloc(sizeof(short));
    *test5 = 5;
    printf("value: %d, stored: %p \n", *test5, test5);
}
