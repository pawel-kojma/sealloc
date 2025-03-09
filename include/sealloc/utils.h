/* Misc utils */

#ifndef UTILS_H
#define UTILS_H

#define ALIGNUP_8(n) ((n + 7) & ~7)
#define ALIGNUP_16(n) ((n + 15) & ~15)
#define BITS2BYTES_CEIL(bits) (ALIGNUP_8(bits) / 8)

#endif
