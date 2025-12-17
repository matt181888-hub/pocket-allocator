#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define ALIGNMENT 16

extern uint8_t *heap;
extern size_t heap_size;

typedef struct BlockHeader{
    size_t block_size;
    bool is_free;
} block_header_t;

_Static_assert(sizeof(block_header_t) % ALIGNMENT == 0,
                "block_header_t must be a multiple of 16");

int init_heap(size_t size);

block_header_t* next_block_header(block_header_t* current_block);

block_header_t* previous_block_header(block_header_t* current_block);

bool is_block_free(block_header_t* current_block);

block_header_t* header_from_data_ptr(void *data);

void* my_alloc_ff(size_t requested_bytes);

void my_free(void* p);

void export_heap_snapshot(const char *filename);

void visualize_heap();

void print_heap_overview();

void* my_alloc_bf();

bool check_heap_integrity();

bool is_valid_header();

void* my_realloc_general(void* ptr, size_t new_size, bool is_best_fit);

void* my_realloc_ff(void* ptr, size_t new_size);

void* my_realloc_bf(void* ptr, size_t new_size);




