#include <assert.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>


// Constants

const long CHUNK_SIZE = 4096;
const long NUMBER_OF_BLOCKS_IN_CHUNK = 512;

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
        out_of_bounds_start = heap_start + CHUNK_SIZE;
        return 0;
    }

    out_of_bounds_start += new_memory_size_in_bytes;   
    return 0;
}

void create_first_header() {
    struct header first_header = {0, NUMBER_OF_BLOCKS_IN_CHUNK};
    *heap_start = first_header;
}

void allocate_at(struct header* address, int size) {
    int original_header_size = address->allocated_blocks;

    struct header new_header = {1, size};

    int next_header_size = original_header_size - size - 1;

    if (next_header_size == -1) {
        return;
    }

    struct header next_header = {0, next_header_size};
    address[size + 1] = new_header;
}

void* nalloc(int allocation_size_in_bytes) {
    // The ceil of the divition by 8;
    int number_of_blocks = (allocation_size_in_bytes + 8 - 1) / 8;

    if(number_of_chunks == 0) {
        get_new_chunk();
    }

    struct header* current_header = heap_start;

    while (1) {
        // Check if still in bounds
        if (current_header >= out_of_bounds_start) {
            get_new_chunk();
            // *todo* resize last empty header or add new empty header;
            continue;
        }

        if (current_header->is_occupied == 0 
        && current_header->allocated_blocks <= number_of_blocks) {
            allocate_at(current_header, number_of_blocks);
            return (void*) current_header;
        }

        current_header = &current_header[number_of_blocks + 1];
    }
}

int main() {
    int* test = nalloc(sizeof(int));
    *test = 34;
    printf("value: %d, stored: %p \n", *test, test);

    int* test2 = nalloc(sizeof(int));
    *test2 = 7;
    printf("value: %d, stored: %p \n", *test2, test2);
}
