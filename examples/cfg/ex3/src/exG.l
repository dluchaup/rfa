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
#include "exG_defs.h"
#include "exG_parse.hh"

// Copied from Randy's pcre.l
extern int switch_getinput(char *buf, int maxlen);
#undef YY_INPUT
#define YY_INPUT(buf, retval, maxlen)	(retval = switch_getinput(buf, maxlen))

/* this is solely to avoid an annoying compiler warning */
static void yyunput( int c, register char *yy_bp ) __attribute((unused));
char dbg_c;
static int dbg_num_chars=0;
/* This is grammar G:
 * *** grammar rules ***
 * r0: L -> T T T T T T T T T T
 * r1: T -> .{100}
 */
%}
N		 [a-zA-Z0-9_]{100}
%%
{N} {//rule 1
  yylval._LParseTree = new LRule::LParseTree(r1, strndup(yytext, yyleng), yyleng);
  if(DBG_LEX_YACC) {
    printf("\nDBG:LEX:_T\n");
    int num_zero=0;
    int num_nl=0;
    for (int i=0; i< yyleng; ++i) {
      if (yytext[i]==0)
	++num_zero;
      if (yytext[i]=='\n')
	++num_nl;
      printf("\nDBG:yytext[%d][%d]=%d\n", dbg_num_chars++, i, (int)yytext[i]);
    }
    printf("\nDBG:LEX:num_zero=%d, num_nl=%d\n", num_zero, num_nl);
    fflush(stdout);
  }
  return _T;
 }
. {//shouldn't get here, but we might for weird characters unknown to lex (ex 253 for a 256-bytes alphabet)
  printf("\nDBG:LEX:BAD:%u is_nl=%d\n", (unsigned int)((unsigned char*)yytext)[0], (yytext[0]=='\n'));
  exit(1);
 }
%%

int yywrap(void)
{
   return 1;
}   
