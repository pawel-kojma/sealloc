/* Random number generator API */

#include <stdint.h>

#ifndef SEALLOC_RANDOM_H_
#define SEALLOC_RANDOM_H_

/* Initialize random number generator */
void init_splitmix32(uint32_t seed);

/* Get next pseudo-random 32-bits */
uint32_t splitmix32(void);

#endif /* SEALLOC_RANDOM_H_ */
