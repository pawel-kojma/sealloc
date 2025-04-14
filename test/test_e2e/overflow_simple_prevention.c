#include <stdlib.h>
#include <string.h>

struct asd {
  unsigned a;
  unsigned b;
};

int main(void) {
  struct asd *a = malloc(16);
  struct asd *b = malloc(16);

  b->a = 0xf00dc4fe;
  memset(a, 0, 2 * sizeof(struct asd));
  if (b->a == 0xf00dc4fe) return 1;
  return 0;
}
