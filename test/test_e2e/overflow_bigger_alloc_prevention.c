#include <stdlib.h>
#include <string.h>

#define COOKIE 0xf00dc4fe

struct asd {
  unsigned a;
  unsigned b;
};

int main(void) {
  struct asd *a = malloc(1000000);
  struct asd *b = malloc(1000000);
  struct asd *c = malloc(1000000);

  a->a = COOKIE;
  b->a = COOKIE;
  c->a = COOKIE;
  memset(a, 0, 2 * sizeof(struct asd));
  memset(b, 0, 2 * sizeof(struct asd));
  memset(c, 0, 2 * sizeof(struct asd));
  if (a->a == COOKIE && b->a == COOKIE && c->a == COOKIE) return 1;
  return 0;
}
