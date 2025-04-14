#include <stdlib.h>
#include <string.h>
#define RUN_SIZE 8192

int main(void) {
  void *a = malloc(16);

  // We might need less, but this should fail at all times
  memset(a, 0x42, RUN_SIZE + 1);
  return 0;
}
