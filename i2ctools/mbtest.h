#ifndef _____MBTEST__1303_H____
#define _____MBTEST__1303_H____
#include "defines.h"

#define BITS_IN_INT (sizeof(int)*8)

#define VTRIM_MEDIUM (((VTRIM_HIGH) + (VTRIM_MIN))/2)

inline int count_bits(int ii){
	int r = 0 ;
	int x = ii;
	r += x & 1;
	for (int i = 1 ; i < BITS_IN_INT ; i++) {
		x = x >> 1;
		r += x & 1;
	}
	return r;
}

#define TOP_FIRST_LOOP (0)
#define TOP_LAST_LOOP (11)
#define BOTTOM_FIRST_LOOP (12)
#define BOTTOM_LAST_LOOP (23)

#endif
