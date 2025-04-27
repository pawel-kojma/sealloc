#include <stdlib.h>
#define MAX_CHUNKS 1000

int main(void) {
  void *chunks[MAX_CHUNKS];
  for (int i = 0; i < MAX_CHUNKS; i++) {
    chunks[i] = malloc(42);
  }
  for (int i = 0; i < MAX_CHUNKS; i++) {
    free(chunks[i]);
  }
  free(chunks[MAX_CHUNKS / 2]);
  return 0;
}
