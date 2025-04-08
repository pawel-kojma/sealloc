#include <stdlib.h>

int main(void) {
  void *a = malloc(16);
  void *b = malloc(16);
  free(a);
  free(b);
  free(a);
  return 0;
}
