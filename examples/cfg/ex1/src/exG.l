/*-----------------------------------------------------------------------------
 * File:    exG.h
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
%{
#include <stdio.h>
#include "gram.h"
#include "exG_parse.hh"

// Copied from Randy's pcre.l
extern int switch_getinput(char *buf, int maxlen);
#undef YY_INPUT
#define YY_INPUT(buf, retval, maxlen)	(retval = switch_getinput(buf, maxlen))

/* this is solely to avoid an annoying compiler warning */
static void yyunput( int c, register char *yy_bp ) __attribute((unused));

char dbg_c;
  
%}
%%
.     {
         return dbg_c = yytext[0];
      }
%%
int yywrap(void)
{
   return 1;
}   
