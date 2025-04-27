#include <stdlib.h>
#include <string.h>

#define COOKIE 0xf00dc4fe
#define CHUNKS 100
#define CHUNK_SIZE 32

struct asd {
  unsigned a;
  unsigned b;
};

int main(void) {
  void *spray_chunks[CHUNKS];

  // Spray the heap
  for (int i = 0; i < CHUNKS; i++) {
    spray_chunks[i] = malloc(CHUNK_SIZE);
  }

  // Make holes for application data
  for (int i = 1; i < CHUNKS; i += 2) {
    free(spray_chunks[i]);
  }

  // Application places some data we want to overwrite
  struct asd *application_data = malloc(CHUNK_SIZE);
  application_data->a = COOKIE;

  // Try to overflow the data
  for (int i = 0; i < CHUNKS; i += 2) {
    memset(spray_chunks[i], 0, 4*CHUNK_SIZE);
  }

  // If data is still intact no impactful overflow happened
  if (application_data->a == COOKIE) return 1;

  return 0;
}
