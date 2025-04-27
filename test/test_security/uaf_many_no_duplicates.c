#include <stddef.h>
#include <stdlib.h>

#define MAX_CHUNKS 100

static void *chunks[2 * MAX_CHUNKS];

int cmp(const void *a, const void *b) {
  ptrdiff_t aa = *(ptrdiff_t *)a, bb = *(ptrdiff_t *)b;
  if (aa > bb)
    return 1;
  else if (aa < bb)
    return -1;
  return 0;
}

int main(void) {
  for (int i = 0; i < MAX_CHUNKS; i++) {
    chunks[i] = malloc(16);
  }
  for (int i = 0; i < MAX_CHUNKS; i++) {
    free(chunks[i]);
  }
  for (int i = MAX_CHUNKS; i < 2 * MAX_CHUNKS; i++) {
    chunks[i] = malloc(16);
  }
  qsort(chunks, 2 * MAX_CHUNKS, sizeof(void *), cmp);
  for (int i = 0; i < 2 * MAX_CHUNKS - 1; i++) {
    if (chunks[i] == chunks[i + 1]) return 0;
  }
  return 1;
}
