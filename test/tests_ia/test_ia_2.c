#include <sealloc/internal_allocator.h>
#include <stdio.h>

int main(void) {
  int r = internal_allocator_init();
  if (r < 0) {
    printf("Initialization failed\n");
  }
  void *full_mapping = internal_alloc(INTERNAL_ALLOC_CHUNK_SIZE_BYTES);
  printf("full_mapping=%p\n", full_mapping);
  void *a = internal_alloc(16);
  void *b = internal_alloc(16);
  printf("a=%p\n", a);
  printf("b=%p\n", b);
  return 0;
}
