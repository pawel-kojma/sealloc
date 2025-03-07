#include <sealloc/internal_allocator.h>
#include <stdio.h>

int main(void){
    int r = internal_allocator_init();
    if (r < 0){
        printf("Initialization failed\n");
    }
    void *a = internal_alloc(16);
    void *b = internal_alloc(16);
    internal_free(a);
    internal_free(b);
    void *c = internal_alloc(32);
    void *d = internal_alloc(32);
    printf("a=%p\n",a);
    printf("b=%p\n",b);
    printf("c=%p\n",c);
    printf("d=%p\n",d);
    return 0;
}
