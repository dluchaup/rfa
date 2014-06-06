/*-----------------------------------------------------------------------------
 *parse_utils.cc
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nodes.h"
#include "parse_utils.h"
#include "globals.h"

/* types defined by lex/yacc but used here */
int yyparse(void);
extern int lineno;


typedef enum {
	EVAL_SRC_FILE,
	EVAL_SRC_MEM
} eval_source;


/* Ugly, but it gets the job done... */
static struct {
	eval_source type;
	union {
		FILE *file;
		struct {
			const char *data;
			int len;
		} mem;
	} src;
} eval_data;

disjunction *g_regex = 0;

disjunction *regex_from_file(FILE *fp)
{
	eval_data.type = EVAL_SRC_FILE;
	eval_data.src.file = fp;
	lineno=1;
	yyparse();
	return g_regex;
}

disjunction *regex_from_mem(const char *data, int len, int line)
{
	eval_data.type = EVAL_SRC_MEM;
	eval_data.src.mem.data = data;
	eval_data.src.mem.len = len;
	lineno = line;
	yyparse();
	return g_regex;
}


int eval_getinput(char *buf, int maxlen)
{
   int retval = 0;
   
   switch (eval_data.type) {
      case EVAL_SRC_FILE:
	 retval = fread(buf, 1, maxlen, eval_data.src.file);
	 break;
      case EVAL_SRC_MEM:
	 if ( maxlen > eval_data.src.mem.len ) {
	    maxlen = eval_data.src.mem.len;
	 }
	 memcpy(buf, eval_data.src.mem.data, maxlen);
	 eval_data.src.mem.data += maxlen;
	 eval_data.src.mem.len -= maxlen;
	 retval = maxlen;
	 break;
   }
   return retval;
}



