#define NALLOC_H
#pragma once


void* nalloc(int allocation_size_in_bytes);

void nalloc_free(void* to_be_free_ptr);
