#ifndef _SCALE_BIT_VECTOR_
#define _SCALE_BIT_VECTOR_

#include "./msstats.h"
//#define MAX_NUM_NS 100
#define MZ_SCALE "MZ_SCALE"

#include <bitset>

class ScaleBitVector {
	public:
		int num_nf;
		std::bitset<MAX_NUM_MS> bits;
};

#endif 
