#include <stdlib.h>
#define MAX_CHUNKS 1000

int main(void) {
  void *a = malloc(16);
  free(a);
  void *b = malloc(16);
  if(a == b)
      return 0;
  return 1;
}
