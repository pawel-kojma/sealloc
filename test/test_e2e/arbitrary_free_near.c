#include <stdlib.h>

int main(void) {
  void *a = malloc(16);
  free((void *)((char *)a + 16));
  return 0;
}
