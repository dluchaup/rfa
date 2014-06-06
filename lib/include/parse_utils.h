/*-----------------------------------------------------------------------------
 *parse_utils.h
 * Miscellaneous utilities to assist with parsing.  This code was mostly
 * taken from a codebase corresponding to "pirates ho!" 
 * (pirates.devolution.com)
 *
 * Author:  Randy Smith
 * Date:    10 August 2006
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
 * History:
 * modified by author: Daniel Luchaup
 *---------------------------------------------------------------------------*/
#ifndef PARSE_UTILS_H
#define PARSE_UTILS_H

#include "nodes.h"

/*-----------------------------------------------------------------------------
 * regex_from_mem
 *   parses a regex stored in memory.  You probably want to use parse_regex()
 *   (above) instead of this function.
 *   
 *   data - buffer holding the regex to parse
 *   len  - length of string to be parsed
 *   line - "line number" of buffer, used as an identifier
 *---------------------------------------------------------------------------*/
disjunction *regex_from_mem(const char *data, int len, int line=1);


/*-----------------------------------------------------------------------------
 * regex_from_file
 *   parses a regex stored in a file.
 *
 *---------------------------------------------------------------------------*/
disjunction *regex_from_file(FILE *fp);
      

#endif
