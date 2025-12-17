#include "allocator.h"

int main(void){
    init_heap(1000);
    
    void *p1 = my_alloc_ff(100);
    void *p2 = my_alloc_ff(200);
    
    visualize_heap(); 
    
    my_free(p1);
    my_free(p2);
    
    return 0;
};