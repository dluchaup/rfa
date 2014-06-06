/*-----------------------------------------------------------------------------
 * File:    MPInt.h
 * Author:  Daniel Luchaup
 * Date:    December 2013
 * Copyright 2013 Daniel Luchaup luchaup@cs.wisc.edu
 *
 * This file contains unpublished confidential proprietary work of
 * Daniel Luchaup, Department of Computer Sciences, University of
 * Wisconsin--Madison.  No use of any sort, including execution, modification,
 * copying, storage, distribution, or reverse engineering is permitted without
 * the express written consent (for each kind of use) of Daniel Luchaup.
 *
 *---------------------------------------------------------------------------*/
#ifndef _MPINT_H
#define _MPINT_H

#include "gmpxx.h"
typedef mpz_class MPInt;
typedef mpf_class  MPFloat;
#define MPInt_to_uint(mpi) ((mpi).get_ui())

extern MPInt MPInt_zero;
extern MPInt MPInt_one;

#define MPBits(x) (mpz_sizeinbase((x).get_mpz_t(), 2))
#define MPCapacity(x) (mpz_sizeinbase((x).get_mpz_t(), 2)) //should adjust? x-1

inline float MPdiv2float(const MPInt&num, const MPInt&den) {
		const unsigned int scale = 1000000000;
		MPInt num_scaled = num*scale;
		MPInt int_res = num_scaled / den;
		float res = ((MPInt_to_uint(int_res))/(float)scale);
		return res;
}

#endif //_MPINT_H
