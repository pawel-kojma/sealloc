#include <sealloc/internal_allocator.h>
#include <stdio.h>
#include <stdlib.h>

#define CHUNKS 10000

const size_t SIZES[] = {5, 10, 16, 24, 32, 50, 4535, 544223};

int main(void) {
  srand(123);
  void* chunks[CHUNKS];
  size_t size;
  unsigned sizes_len = sizeof(SIZES) / sizeof(size_t);
  printf("sizes_len=%u\n", sizes_len);
  int r = internal_allocator_init();
  if (r < 0) {
    printf("Initialization failed\n");
  }
  printf("Starting allocations...\n");
  for (int i = 0; i < CHUNKS; i++) {
    size = SIZES[rand() % sizes_len];
    printf("size=%zu\n", size);
    chunks[i] = internal_alloc(size);
    printf("chunk[%d]=%p\n", i, chunks[i]);
    putchar('\n');
  }
  printf("Starting cleanup...\n");
  for (int i = 0; i < CHUNKS; i++) {
    if (rand() % 2 == 0) {
      printf("Freeing chunk[%d]\n", i);
      internal_free(chunks[i]);
    }
  }
  printf("Done\n");
  return 0;
}
