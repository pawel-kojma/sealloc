#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define RUN_SIZE 8192
#define REG_SIZE 16

unsigned get_off(void* ptr) { return (unsigned)((size_t)ptr & (RUN_SIZE - 1)); }
ptrdiff_t diff(void* a, void* b) {
  return (ptrdiff_t)((ptrdiff_t)a - (ptrdiff_t)b);
}

int main(void) {
  void* a = malloc(REG_SIZE);
  void* b = malloc(REG_SIZE);
  void* c;

  // Fill the rest of the run
  for (int i = 2; i < RUN_SIZE / REG_SIZE; i++) {
    c = malloc(REG_SIZE);
  }
  void* aa = malloc(REG_SIZE);
  void* bb = malloc(REG_SIZE);

  if (diff(a, b) != diff(aa, bb)) {
    return 1;
  }

  return 0;
}
