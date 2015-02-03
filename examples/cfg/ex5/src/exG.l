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

extern "C" int exG_error(const char *);
/* this is solely to avoid an annoying compiler warning */
static void yyunput( int c, register char *yy_bp ) __attribute((unused));
char dbg_c;
static int dbg_num_chars=0;
/* This is the language:    /12[^3]*3[^4]*4/, where '.' is [a-zA-Z0-9_]
 *    i.e. the language is  /12[a-zA-Z0-24-9_]*3[a-zA-Z0-35-9_]*4/
 * This is grammar G:
 * *** grammar rules ***
 * r0: L -> '12' T '3' F '4'
 * r1: T -> [^3]*
 * r2: F -> [^4]*
 */
%}
%s AFTER12 AFTER3
not3		 [a-zA-Z0-24-9_]
not4		 [a-zA-Z0-35-9_]
%%
"12" {
  //printf("lex(12)\n");fflush(stdout);
  BEGIN AFTER12; return _c12;
 }
<AFTER12>{not3}* {//rule 1
  printf("lex(not3*)\n");fflush(stdout);
  yylval._LParseTree = new LRule::LParseTree(r1,
					     byte_copy(yytext, yyleng), yyleng);
  return _T;
 }
<AFTER12>"3" {
  //printf("lex(3)\n");fflush(stdout);
  BEGIN AFTER3; return '3';
 }
<AFTER3>{not4}* {//rule 2
  //printf("lex(not4*)\n");fflush(stdout);
  yylval._LParseTree = new LRule::LParseTree(r2,
					     byte_copy(yytext, yyleng), yyleng);
  return _F;
 }
<AFTER3>"4" {
  //printf("lex(4)\n");fflush(stdout);
  BEGIN 0; return '4';
 }
. {//shouldn't get here
  printf("\nDBG:LEX:BAD:%u is_nl=%d\n", (unsigned int)((unsigned char*)yytext)[0], (yytext[0]=='\n'));
  exG_error("unexpeted char in lex: exiting");
  exit(1);
 }
%%

int yywrap(void)
{
   return 1;
}   
