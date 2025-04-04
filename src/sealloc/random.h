/* Random number generator API */


#ifndef SEALLOC_RANDOM_H_
#define SEALLOC_RANDOM_H_

#include <stdint.h>
/* Initialize random number generator */
void init_splitmix32(uint32_t seed);

/* Get next pseudo-random 32-bits */
uint32_t splitmix32(void);

#endif /* SEALLOC_RANDOM_H_ */
