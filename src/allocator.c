#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "allocator.h"
#define MY_API_SUCCESS 0
#define MY_API_ERROR_INVALID_ARGUMENT 1
#define MALLOC_FAIL 2
#define MAX_HEAP_SIZE 8000
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_GRAY    "\033[90m"

uint8_t *heap = NULL;    
size_t heap_size = 0;



int init_heap(size_t size){
    if (size < 1 || size > MAX_HEAP_SIZE){
        printf("Cannot allocate %ld bytes, max is %d\n", size, MAX_HEAP_SIZE);
        return MY_API_ERROR_INVALID_ARGUMENT;
    }
    if (heap != NULL){
        free(heap);
        heap_size = 0; 
    }
    if (size % 16 != 0){
        size = size + (16 - (size % 16));
    }
    heap = malloc(size);
    if (!heap){
        printf("Allocation failed, please try again\n");
        return MALLOC_FAIL;
    }
    heap_size = size;
    block_header_t* header = (block_header_t*)heap; 
    header->block_size = size - sizeof(block_header_t); 
    header->is_free = true;
    printf("Heap of %ld bytes successfully allocated\n", size);
    return MY_API_SUCCESS;
}



block_header_t* next_block_header(block_header_t* current_block) {
    uint8_t* base = (uint8_t*)current_block;
    uint8_t* next = base + sizeof(block_header_t) + current_block->block_size;


    if (next + sizeof(block_header_t) >=  heap + heap_size) {
        return NULL;
    }

    return (block_header_t*)next;
}

block_header_t* previous_block_header(block_header_t* current_block){
    if (!current_block){
        return NULL;
    }
    if (!heap){
        return NULL;
    }
    block_header_t* present = (block_header_t*)(heap); 
    while (present != NULL){
        if (next_block_header(present) == current_block){
            return present;
        }
        present = next_block_header(present);
    }
    return NULL;
}

bool is_block_free(block_header_t* current_block){
    if (!current_block){
        return false;
    }
    return current_block->is_free;
}

block_header_t* header_from_data_ptr(void *data){
    if (data == NULL){
        return NULL;
    }
    uint8_t* byte_data = (uint8_t*)data;
    uint8_t* byte_ptr = byte_data - sizeof(block_header_t);

    if (byte_ptr < heap || byte_ptr >= heap + heap_size) {
        return NULL;
    }
    return (block_header_t*)(byte_ptr);
}

void *my_alloc_ff(size_t requested_bytes){
    if (!heap){
        return NULL;
    }
    if (requested_bytes > heap_size){
        return NULL;
    }
    if (requested_bytes <= 0){
        return NULL;
    }
    if (requested_bytes % 16 != 0){
        requested_bytes = requested_bytes + (16 - (requested_bytes % 16));
    }
    block_header_t *current = (block_header_t*)(heap); 
    while (current != NULL){
        if (current->is_free && current->block_size >= requested_bytes){
            void *p_my_alloc;
            size_t original_size = current->block_size;
            p_my_alloc = (uint8_t*)(current) + sizeof(block_header_t);
            current->block_size = requested_bytes;
            current->is_free = false;
            
            size_t left_over_space = original_size - requested_bytes - sizeof(block_header_t);
            if (left_over_space < sizeof(block_header_t)){
                return p_my_alloc;
            }
            block_header_t* new_block = (block_header_t*)((uint8_t*)(current) + sizeof(block_header_t) + requested_bytes);
            new_block->block_size = left_over_space;
            new_block->is_free = true;
            return p_my_alloc;
        }
        current = next_block_header(current);
    }return NULL;
}

void *my_alloc_bf(size_t requested_bytes){
    if (!heap){
        return NULL;
    }
    if (requested_bytes > heap_size){
        return NULL;
    }
    if (requested_bytes <= 0){
        return NULL;
    }
    if (requested_bytes % 16 != 0){
    requested_bytes = requested_bytes + (16 - (requested_bytes % 16));
    }
    block_header_t* current = (block_header_t*)(heap); 
    block_header_t* smallest_block = NULL;

    while (current != NULL){
        if (current->is_free && current->block_size >= requested_bytes){
            if (smallest_block == NULL || current->block_size < smallest_block->block_size){
                smallest_block = current;
            }
        }   
        current = next_block_header(current);
    }
    if (smallest_block == NULL){
        return NULL;
    }
    void *p_my_alloc;
    size_t original_size = smallest_block->block_size;
    p_my_alloc = (uint8_t*)(smallest_block) + sizeof(block_header_t);
    smallest_block->block_size = requested_bytes;
    smallest_block->is_free = false;
    
    size_t left_over_space = original_size - requested_bytes - sizeof(block_header_t);
    if (left_over_space < sizeof(block_header_t)){
        return p_my_alloc;
    }
    block_header_t* new_block = (block_header_t*)((uint8_t*)(smallest_block) + sizeof(block_header_t) + requested_bytes);
    new_block->block_size = left_over_space;
    new_block->is_free = true;
    return p_my_alloc;
}

void my_free(void* p){
    if (p == NULL){
        printf("pointer is null\n");
        return;
    }
    block_header_t* p_block = header_from_data_ptr(p);
    if (!p_block){
        printf("no header\n");
        return;
    }
    if (p_block->is_free){
        printf("already freed\n");
        return;
    }
    p_block->is_free = true;
    
    block_header_t* next_block = next_block_header(p_block);
    if (next_block && next_block->is_free){
        p_block->block_size = p_block->block_size + sizeof(block_header_t) + next_block->block_size;
    }
    
    block_header_t* previous_block = previous_block_header(p_block);
    if (previous_block && previous_block->is_free){
        previous_block->block_size = previous_block->block_size + sizeof(block_header_t) + p_block->block_size;
    }
}

void export_heap_snapshot(const char *filename) {
    FILE *f = fopen(filename, "w");
    if (!f) {
        printf("Failed to open file\n");
        return;
    }
    
    fprintf(f, "{\n");
    fprintf(f, "  \"heap_size\": %zu,\n", heap_size);
    fprintf(f, "  \"blocks\": [\n");
    
    block_header_t *current = (block_header_t*)heap;
    while (current != NULL) {
        size_t offset = (uint8_t*)current - heap;
        
        fprintf(f, "    {\"offset\": %zu, \"size\": %zu, \"is_free\": %s, \"block_header_size\": %ld}", 
                offset, current->block_size, current->is_free ? "true" : "false", sizeof(block_header_t));
        
        if (next_block_header(current) != NULL) {
            fprintf(f, ",");
        }
        fprintf(f, "\n");
        current = next_block_header(current);
    }
    
    fprintf(f, "  ]\n");
    fprintf(f, "}\n");
    fclose(f);
}

void visualize_heap() {
    printf("\n" COLOR_BLUE "=== HEAP VISUALIZATION ===" COLOR_RESET "\n");
    
    block_header_t *current = (block_header_t*)heap;
    
    while (current != NULL) {
        const char *color = current->is_free ? COLOR_GREEN : COLOR_RED;
        const char *status = current->is_free ? "FREE" : "USED";
        
        printf("%s[%s]%s offset=%zu size=%zu\n", 
               color, status, COLOR_RESET,
               (uint8_t*)current - heap,
               current->block_size);
        
        printf("  %s", color);
        int bar_length = (current->block_size * 100) / heap_size; 
        if (bar_length < 1) bar_length = 1;  
        for (int i = 0; i < bar_length; i++) {
            printf(current->is_free ? "░" : "█");
        }
        printf("%s\n\n", COLOR_RESET);
        
        current = next_block_header(current);
    }
    print_heap_overview();
    printf("==========================\n");
}

void print_heap_overview() {
    printf("\nOverview:\n[");
    
    block_header_t *current = (block_header_t*)heap;
    
    while (current != NULL) {
        const char *color = current->is_free ? COLOR_GREEN : COLOR_RED;
        int bar_length = (current->block_size * 100) / heap_size;
        if (bar_length < 1) bar_length = 1;
        
        printf("%s", color);
        for (int i = 0; i < bar_length; i++) {
            printf(current->is_free ? "░" : "█");
        }
        printf(COLOR_RESET);
        
        current = next_block_header(current);
    }
    
    printf("]\n");
}

bool check_heap_integrity(){
    if (!heap){
        printf("ERROR: Heap not initialized\n");
        return false;
    }
    block_header_t *current = (block_header_t*)heap;
    size_t total_accounted = 0;
    while (current != NULL) {
        if (!is_valid_header(current)){
            return false;
        }
        total_accounted += sizeof(block_header_t) + current->block_size;
        current = next_block_header(current);
    }
    if (total_accounted != heap_size){
        printf("ERROR: Total accounted (%zu) doesn't match heap_size (%zu)\n", 
                total_accounted, heap_size);
        return false;
    }
    return true;
}

bool is_valid_header(block_header_t* header) {
    uint8_t* ptr = (uint8_t*)header;
    if (!(ptr >= heap && ptr < heap + heap_size)){
        printf("ERROR: Header at %p is outside heap bounds [%p, %p)\n", 
        header, heap, heap + heap_size);
        return false;
    }
    if ((uintptr_t)header % ALIGNMENT != 0) {
        printf("Header not aligned!\n");
        return false;  
    }
    if (header->block_size % ALIGNMENT != 0) {
    printf("Block_size not aligned!\n");
    return false; 
    }
    uint8_t* block_end = (uint8_t*)header + sizeof(block_header_t) + header->block_size;
    if (block_end > heap + heap_size) {
    printf("Block extends past heap!\n");
    return false;  
    }
    if (header->is_free != true && header->is_free != false) {
    printf("is_free boolean is corrupted\n");
    return false;  
    }
    return true;
}

void* my_realloc_general(void* ptr, size_t new_size, bool is_best_fit){
    if (new_size <= 0){
        my_free(ptr);    
        return NULL;
        }
    if (new_size % ALIGNMENT != 0){
        new_size = new_size + (ALIGNMENT - (new_size % ALIGNMENT));
        }
    if (ptr == NULL){
        if (is_best_fit){
            return my_alloc_bf(new_size);
        }
        return my_alloc_ff(new_size);
    }
    if (new_size > heap_size){
        return NULL;
    }
    block_header_t* ptr_header = header_from_data_ptr(ptr);
    block_header_t* next_header = next_block_header(ptr_header);
    if (new_size < ptr_header->block_size){
        if (new_size + sizeof(block_header_t) < ptr_header->block_size){
            size_t leftover_space = (ptr_header->block_size) - (new_size + sizeof(block_header_t));
            ptr_header->block_size = new_size;
            block_header_t* new_free = (block_header_t*)((uint8_t*)(ptr) + new_size);
            new_free->block_size = leftover_space;
            new_free->is_free = true; 
            return ptr;
        }else{
            ptr_header->block_size = new_size;
            return ptr;
        }
    }
    size_t new_required_space = new_size - ptr_header->block_size;
    if (next_header && next_header->is_free && (next_header->block_size + sizeof(block_header_t)) >= new_required_space){
        size_t available_space = sizeof(block_header_t) + next_header->block_size;
        if (available_space >= new_required_space + sizeof(block_header_t) + ALIGNMENT){
            ptr_header->block_size = new_size;
            block_header_t* new_free = (block_header_t*)((uint8_t*)(ptr) + new_size);
            new_free->block_size = available_space - new_required_space - sizeof(block_header_t);
            new_free->is_free = true;
        }else{
            ptr_header->block_size += available_space;
        }
        return ptr;
    }
    if (is_best_fit){
        void* new_ptr = my_alloc_bf(new_size);
        if (!new_ptr) return NULL;
        size_t copy_size = (ptr_header->block_size < new_size) ? ptr_header->block_size : new_size;
        memcpy(new_ptr, ptr, copy_size);
        my_free(ptr);
        return new_ptr;
    }else{
        void* new_ptr = my_alloc_ff(new_size);
        if (!new_ptr) return NULL;
        size_t copy_size = (ptr_header->block_size < new_size) ? ptr_header->block_size : new_size;
        memcpy(new_ptr, ptr, copy_size);
        my_free(ptr);
        return new_ptr;
    }
    return NULL;
}

void* my_realloc_ff(void* ptr, size_t new_size){
    return my_realloc_general(ptr, new_size, false);
}

void* my_realloc_bf(void* ptr, size_t new_size){
    return my_realloc_general(ptr, new_size, true);
}






