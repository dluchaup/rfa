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
//extern int switch_getinput(char *buf, int maxlen);
#undef YY_INPUT
#define YY_INPUT(buf, retval, maxlen)	(retval = g_exG->pi->switch_getinput(buf, maxlen))

/* this is solely to avoid an annoying compiler warning */
static void yyunput( int c, register char *yy_bp ) __attribute((unused));
char dbg_c;
/* This is grammar G:
 * *** grammar rules ***
 * r00: L -> S
 * r01: L -> S L
 * r0: S  -> ( Exp )
 * r1: S  -> ( Exp + S)
 * r2: Exp -> IDENTIFIER
 * r3: Exp -> Num
 * r4: Num -> INT
 * r5: Num -> FLOAT
 * *** lexical rules ***
 * r6: IDENTIFIER -> {L}({L}|{D})*
 * r7: INT -> 0[xX]{H}+
 * r8: INT -> {D}+
 * r9: FLOAT -> {D}*"."{D}+
 * r10:FLOAT -> {D}+"."{D}*
 */
%}
D			[0-9]
L			[a-zA-Z_]
H			[a-fA-F0-9]
%%
{L}({L}|{D})* {//rule 6
  //TBD:perf yylval._LParseTree can be replaced with yylval._LParseTree= MPInt/rank
  yylval._LParseTree = new LRule::LParseTree(g_exG->r6, strndup(yytext, yyleng), yyleng);
  return _IDENTIFIER;
 }
0[xX]{H}+     {//rule 7
  yylval._LParseTree = new LRule::LParseTree(g_exG->r7, strndup(yytext, yyleng), yyleng);
  return _INT;
 }
{D}+          {//rule 8
  yylval._LParseTree = new LRule::LParseTree(g_exG->r8, strndup(yytext, yyleng), yyleng);
  return _INT;
 }
.             {
         return dbg_c = yytext[0];
 }
{D}*"."{D}+   {//rule 9 : note r9/r10 overlap and this leads to ambiguity
  yylval._LParseTree = new LRule::LParseTree(g_exG->r9, strndup(yytext, yyleng), yyleng);
  return _FLOAT;
 }
{D}+"."{D}*     {//rule 10 : note r9/r10 overlap and this leads to ambiguity
  yylval._LParseTree = new LRule::LParseTree(g_exG->r10, strndup(yytext, yyleng), yyleng);
  return _FLOAT;
 }
%%
/* Non-ambiguous alternative to rules 9/10:
({D}*"."{D}+|{D}+"."{D}*)  {
  yylval._LParseTree = new LRule::LParseTree(g_exG->r9, strndup(yytext, yyleng), yyleng);
  return _FLOAT;
 }
*****************************
The ambiguous part:
{D}*"."{D}+   {//rule 9 : note r9/r10 overlap and this leads to ambiguity
  yylval._LParseTree = new LRule::LParseTree(g_exG->r9, strndup(yytext, yyleng), yyleng);
  return _FLOAT;
 }
{D}+"."{D}*     {//rule 10 : note r9/r10 overlap and this leads to ambiguity
  yylval._LParseTree = new LRule::LParseTree(g_exG->r10, strndup(yytext, yyleng), yyleng);
  return _FLOAT;
 }
*/

int yywrap(void)
{
   return 1;
}   
