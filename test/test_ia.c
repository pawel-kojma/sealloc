#include <sealloc/internal_allocator.h>
#include <stdio.h>

int main(void){
    internal_allocator_init();
    void *a = internal_alloc(16);
    void *b = internal_alloc(32);
    void *c = internal_alloc(16);
    void *d = internal_alloc(16);
    printf("a=%p\n",a);
    printf("b=%p\n",b);
    printf("c=%p\n",c);
    printf("d=%p\n",d);
    return 0;
}
