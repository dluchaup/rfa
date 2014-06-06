/*-----------------------------------------------------------------------------
 * errmsg.h
 *   Error reporting routines for the lexer/parser
 *
 * Author:  Randy Smith
 * Date:    11 July 2006
 *
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
#ifndef ERR__MSGS
#define ERR__MSGS




void warnf(unsigned int lineno, const char *message,...);
void errf(unsigned int lineno, const char *message,...);


#endif
