/*-----------------------------------------------------------------------------
 * File:    globals.h
 *
 *
 * Author:  Randy Smith
 * Date:    18 May 2007
 *
 *
 *    Copyright 2006,2007 Randy Smith, smithr@cs.wisc.edu
 *
 *    This file contains unpublished confidential proprietary
 *    work of Randy Smith, Department of Computer Sciences,
 *    University of Wisconsin--Madison.  No use of any sort, including
 *    execution, modification, copying, storage, distribution, or reverse
 *    engineering is permitted without the express written consent of
 *    Randy Smith.
 *
 *-----------------------------------------------------------------------------
 * History
 * modified by author: Daniel Luchaup
 *---------------------------------------------------------------------------*/
#ifndef GLOBALS_H
#define GLOBALS_H

#include"alphabet_byte.h"

typedef struct flags_t
{
      unsigned int case_insensitive:1;
      unsigned int multi_line:1;
      unsigned int implicit_star:1;
      unsigned int known_ambiguous:1;

      void reset(void) {
	 case_insensitive = multi_line = implicit_star = known_ambiguous = 0;
      }
} flags_t;



extern flags_t g_flags;

#endif
