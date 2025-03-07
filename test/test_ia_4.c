#include <fcntl.h>
#include <sealloc/internal_allocator.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define CHUNKS 10000

const size_t SIZES[] = {5, 10, 16, 24, 32, 50, 4535, 544223};

int main(void) {
  srand(123);
  void* chunks_se[CHUNKS];
  void* chunks_ma[CHUNKS];
  size_t sizes[CHUNKS];
  char buf[1000000];
  unsigned sizes_len = sizeof(SIZES) / sizeof(size_t);
  int r = internal_allocator_init();
  if (r < 0) {
    printf("Initialization failed\n");
    exit(1);
  }
  int fd = open("/dev/urandom", O_RDONLY);
  if (fd < 0) {
    printf("fd < 0");
    exit(1);
  }

  for (int i = 0; i < CHUNKS; i++) {
      sizes[i] = SIZES[rand() % sizes_len];
      read(fd, buf, sizes[i]);
      chunks_ma[i] = malloc(sizes[i]);
      chunks_se[i] = internal_alloc(sizes[i]);
      memcpy(chunks_ma[i], buf, sizes[i]);
      memcpy(chunks_se[i], buf, sizes[i]);
  }

  for (int i = 0; i < CHUNKS; i++) {
    if (memcmp(chunks_ma[i], chunks_se[i], sizes[i]) != 0) {
      printf("Chunks of size %zu at index %d differ\n", sizes[i], i);
      exit(1);
    }
  }

  printf("Done\n");
  return 0;
}
