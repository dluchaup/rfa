/*-----------------------------------------------------------------------------
 * errmsg.cc
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
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

void warnf(unsigned int lineno, const char *message,...)
{
   va_list arglist;

   fprintf(stdout, "%u: WARNING, ", lineno);

   va_start(arglist, message);
   vfprintf(stdout, message, arglist);
   fprintf(stdout, "\n");
   va_end(arglist);
}

void errf(unsigned int lineno, const char *message,...)
{
   va_list arglist;

   fprintf(stdout, "%u: ERROR, ", lineno);

   va_start(arglist, message);
   vfprintf(stdout, message, arglist);
   va_end(arglist);
   fprintf(stdout, "\n");
   exit(-1);
}
