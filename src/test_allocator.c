#include <assert.h>
#include <stdio.h>
#include "allocator.h"
#define MY_API_SUCCESS 0
#define MY_API_ERROR_INVALID_ARGUMENT 1
#define MALLOC_FAIL 2
#define MAX_HEAP_SIZE 8000

/* ============================================================
   DECLARATIONS
   ============================================================ */
int init_heap(size_t size);
block_header_t* next_block_header(block_header_t* current_block);
bool is_block_free(block_header_t* current_block);
block_header_t* header_from_data_ptr(void *data);

/* Helper to reset heap before tests */
static void reset_heap(size_t size) {
    bool ok = init_heap(size);
    assert(ok == 0);
}

/* ============================================================
   TESTS FOR init_heap
   ============================================================ */

void test_init_heap_basic() {
    int r = init_heap(100);
    assert(r == 0);
    assert(heap != NULL);
    assert(heap_size == 112);

    block_header_t *h = (block_header_t*)heap;
    assert(h->is_free == true);
    assert(h->block_size == 112 - sizeof(block_header_t));
}

void test_init_heap_zero() {
    assert(init_heap(0) == 1);
}

void test_init_heap_too_large() {
    assert(init_heap(MAX_HEAP_SIZE + 1) == 1);
}

void test_init_heap_double_call() {
    assert(init_heap(1024) == 0);
    uint8_t *old_heap = heap;

    assert(init_heap(2048) == 0);
    assert(heap != old_heap);
    assert(heap_size == 2048);
}


/* ============================================================
   TESTS FOR is_block_free
   ============================================================ */

void test_is_block_free_original() {
    reset_heap(128);
    block_header_t *h = (block_header_t*)heap;

    assert(is_block_free(h) == true);
    h->is_free = false;
    assert(is_block_free(h) == false);
}

void test_is_block_free_null() {
    assert(is_block_free(NULL) == false);
}

/* ============================================================
   TESTS FOR header_from_data_ptr
   ============================================================ */

void test_header_from_data_ptr_original() {
    reset_heap(128);
    block_header_t *h = (block_header_t*)heap;
    uint8_t *data = (uint8_t*)heap + sizeof(block_header_t);

    block_header_t *back = header_from_data_ptr(data);
    assert(back == h);
}

void test_header_from_data_ptr_null() {
    assert(header_from_data_ptr(NULL) == NULL);
}

void test_header_from_data_ptr_outside_heap() {
    reset_heap(128);

    int fake;
    assert(header_from_data_ptr(&fake) == NULL);
}

void test_header_from_data_ptr_heap_pointer_itself() {
    reset_heap(128);
    assert(header_from_data_ptr(heap) == NULL);
}
/* ============================================================
   TESTS FOR my_alloc_ff
   ============================================================ */
void test_my_alloc_ff_to_exhaustion_no_fit() {
    reset_heap(128);
    my_alloc_ff(64);
    my_alloc_ff(16);
    assert(my_alloc_ff(32) == NULL);
}

void test_my_alloc_ff_to_exhaustion_does_fit() {
    reset_heap(128);
    my_alloc_ff(64);
    assert(my_alloc_ff(16)!= NULL);
}

void test_my_alloc_ff_multiple_small_allocations() {
    reset_heap(1000);
    void *p1 = my_alloc_ff(10);
    void *p2 = my_alloc_ff(10);
    void *p3 = my_alloc_ff(10);
    
    assert(p1 != NULL);
    assert(p2 != NULL);
    assert(p3 != NULL);
    assert(p1 != p2); 
    assert(p2 != p3);
}

void test_my_alloc_ff_split_spacing() {
    reset_heap(1000);
    void *p1 = my_alloc_ff(50);
    void *p2 = my_alloc_ff(50);
    
    size_t distance = (uint8_t*)p2 - (uint8_t*)p1;
    assert(distance == 64 + sizeof(block_header_t));
}

void test_my_alloc_ff_split_creates_free_block() {
    reset_heap(200);
    my_alloc_ff(50);
    
    block_header_t *first = (block_header_t*)heap;
    assert(first->is_free == false);
    assert(first->block_size == 64);
    
    block_header_t *second = next_block_header(first);
    assert(second != NULL);
    assert(second->is_free == true);
    assert(second->block_size == heap_size - sizeof(block_header_t) - first->block_size - sizeof(block_header_t));
}

void test_my_alloc_ff_allocate_zero() {
    reset_heap(100);
    assert (my_alloc_ff(0) == NULL);
}

void test_my_alloc_ff_allocate_more_than_heap_size(){
    reset_heap(100);
    assert(my_alloc_ff(101) == NULL);
}

void test_my_alloc_ff_uninitialized_heap() {
    heap = NULL;  
    assert(my_alloc_ff(50) == NULL);
}

/* ============================================================
   TESTS FOR my_free
   ============================================================ */
void test_my_free_three_way_coalescing() {
    reset_heap(1000);
    void* p1 = my_alloc_ff(10);
    void* p2 = my_alloc_ff(10);
    void* p3 = my_alloc_ff(10);
    
    my_free(p1);
    my_free(p3);
    my_free(p2);
    
    block_header_t* header = header_from_data_ptr(p1); 
    size_t size_of_header = header->block_size;
    assert(size_of_header == 1008 - sizeof(block_header_t));
}

void test_my_free_and_reallocate() {
    reset_heap(100);
    void* p1 = my_alloc_ff(10);

    my_free(p1);

    void* p2 = my_alloc_ff(20);
    assert(p1 == p2);
    block_header_t* first = (block_header_t*)heap;
    assert(first->is_free == false);  
    assert(first->block_size == 32); 
}

/* ============================================================
   TESTS FOR my_alloc_bf
   ============================================================ */
void test_best_fit_chooses_smallest() {
    reset_heap(1000);
    
    void *p1 = my_alloc_ff(100);
    void *barrier1 = my_alloc_ff(10);
    void *p2 = my_alloc_ff(200);
    void *barrier2 = my_alloc_ff(10);
    void *p3 = my_alloc_ff(50);
    void *barrier3 = my_alloc_ff(10);  // Prevent p3 from coalescing with remaining space!
    
    my_free(p1);
    my_free(p2);
    my_free(p3);
    
    void *p4 = my_alloc_bf(40);
    
    assert(p4 == p3);  // Now it should work!
}

/* ============================================================
   Alignment Check
   ============================================================ */
void test_allocation_alignment() {
    reset_heap(1000);
    
    void *p1 = my_alloc_ff(7);   
    void *p2 = my_alloc_ff(33);  
    void *p3 = my_alloc_ff(100); 
    
    
    assert(((uintptr_t)p1 % 16) == 0);
    assert(((uintptr_t)p2 % 16) == 0);
    assert(((uintptr_t)p3 % 16) == 0);
}

void test_block_size_alignment() {
    reset_heap(1000);
    
    void *p1 = my_alloc_ff(7);  
    
    block_header_t *header = header_from_data_ptr(p1);
    assert(header->block_size % 16 == 0);
}

/* ============================================================
   Heap Integrity Check
   ============================================================ */

void test_integrity_checker_detects_bad_size() {
    reset_heap(1000);
    
    block_header_t *header = (block_header_t*)heap;
    header->block_size = 9999999;  // Corrupt it!
    
    assert(check_heap_integrity() == false);
}

void test_integrity_checker_passes() {
    reset_heap(1000);
    my_alloc_ff(50);
    my_alloc_ff(100);
    
    assert(check_heap_integrity() == true);
}
/* ============================================================
   Realloc Check
   ============================================================ */

void test_realloc_shrink() {
    reset_heap(1000);
    void *p = my_alloc_ff(100);
    void *p2 = my_realloc_ff(p, 50);  // Shrink
    assert(p == p2);  // Same pointer
}

void test_realloc_grow_in_place() {
    reset_heap(1000);
    void *p = my_alloc_ff(50);
    // Next block is free with plenty of space
    void *p2 = my_realloc_ff(p, 100);  // Grow
    assert(p == p2);  // Should expand in place
}

void test_realloc_must_move() {
    reset_heap(1000);
    void *p1 = my_alloc_ff(50);
    void *p2 = my_alloc_ff(50);  // Block next block
    void *p3 = my_realloc_ff(p1, 200);  // Must move
    assert(p1 != p3);  // Different pointer
}

void test_realloc_preserves_data() {
    reset_heap(1000);
    
    int *arr = (int*)my_alloc_ff(10 * sizeof(int));
    
    // Write some data
    for (int i = 0; i < 10; i++) {
        arr[i] = i * 100;
    }
    
    // Realloc to larger size (might move)
    arr = (int*)my_realloc_ff(arr, 20 * sizeof(int));
    
    // Verify original data is intact
    for (int i = 0; i < 10; i++) {
        assert(arr[i] == i * 100);
    }
}

/* ============================================================
   MAIN: run all tests
   ============================================================ */

int main() {
    printf("Running allocator tests...\n");

    test_init_heap_basic();
    test_init_heap_zero();
    test_init_heap_too_large();
    test_init_heap_double_call();

    test_is_block_free_original();
    test_is_block_free_null();

    test_header_from_data_ptr_original();
    test_header_from_data_ptr_null();
    test_header_from_data_ptr_outside_heap();
    test_header_from_data_ptr_heap_pointer_itself();

    test_my_alloc_ff_to_exhaustion_no_fit();
    test_my_alloc_ff_to_exhaustion_does_fit();
    test_my_alloc_ff_multiple_small_allocations();
    test_my_alloc_ff_split_spacing();
    test_my_alloc_ff_split_creates_free_block();
    test_my_alloc_ff_allocate_zero();
    test_my_alloc_ff_allocate_more_than_heap_size();
    test_my_alloc_ff_uninitialized_heap();

    test_my_free_three_way_coalescing();
    test_my_free_and_reallocate();

    test_best_fit_chooses_smallest();

    test_allocation_alignment();
    test_block_size_alignment();

    test_integrity_checker_detects_bad_size();
    test_integrity_checker_passes();

    test_realloc_shrink();
    test_realloc_grow_in_place();
    test_realloc_must_move();
    test_realloc_preserves_data();


    printf("All tests passed successfully.\n");
    return 0;
}

