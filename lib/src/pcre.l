%{
/*-----------------------------------------------------------------------------
 * pcre.l
 *   Lexer for pcre-style regular expressions.
 *
 * Author:  Randy Smith
 * Date:    7 August 2006
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
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include "pcre_parse.hh" // TBD make this name a makefile parameter
#include "escape_sequences.h"
#include "nodes.h"

int lineno = 1;
int colno  = 1;

extern "C" int yyerror(const char *s);
extern int errno;

/* This allows us to parse from either a string or a file.  The function
 * calls that invoke either option are contained in parse_utils.h.  Note
 * that this is specific to flex.  See Lex&Yacc manual. */

extern int eval_getinput(char *buf, int maxlen); /*defined in parse_utils.cc*/
#undef YY_INPUT
#define YY_INPUT(buf, retval, maxlen)	(retval = eval_getinput(buf, maxlen))

/* this is solely to avoid an annoying compiler warning */
static void yyunput( int c, register char *yy_bp ) __attribute((unused));

%}

control_escape   ([fnrtv]) 
symbol_escape    ([|\/\(\)\{\}\[\]\\\+\*\?\^\$])
octal_escape    ([0-7]+)
hex_escape      ("x"[0-9a-fA-F]+) 

escape_sequence ([\\]({simple_escape}|{octal_escape}|{hex_escape}))
c_char          ([^\'\\\n]|{escape_sequence})
s_char          ([^\"\\\n]|{escape_sequence})


%s custom_set
%s counted_range
%x firstsym_set

%%



<firstsym_set>"-"      {
                          /* firstsym_set is an exclusive start state that 
			     allows us to look for things like [-...] or
			     []...].  '-' and ']' have literal meanings 
			     when they are the first characters in a char
			     class.  firstsym_set allows this to happen. */
			  BEGIN custom_set;
                          colno += yyleng;
			  yylval.character = yytext[0];
			  return CHAR;
                       }

<firstsym_set>"]"      {
			  BEGIN custom_set;
                          colno += yyleng;
			  yylval.character = yytext[0];
			  return CHAR;
                       }

<firstsym_set>""/.       { 
                          /* we didn't match the two characters we were 
			     looking for, so now do standard custom_set
			     matching */
                          BEGIN custom_set;
                       }
                     


<custom_set>"-"        {
                          colno += yyleng;
                          return DASH_SYM; 
                       }

<custom_set>[|(){}[+*?^$./] {
                           /* don't need to escape these inside [] */
                           colno += yyleng;
			   yylval.character = yytext[0];
			   return CHAR;
                       }

"[-]"                  {
                           colno += yyleng;
			   yylval.character = '-';
			   return CHAR;
                       }

"[^"                   {  /* placing this after the above rule allows 
			   * '[' to be matched inside [].  If this 
			   * occurred before the <custom_set>... rules,
			   * then we would try to enter custom_set again */
                          BEGIN firstsym_set;
                          colno += yyleng;
                          return LSQUARE_NOT_SYM; 
                       }


"["                    {  /* placing this after the above rule allows 
			   * '[' to be matched inside [].  If this 
			   * occurred before the <custom_set>... rules,
			   * then we would try to enter custom_set again */ 
                          BEGIN firstsym_set;
                          colno += yyleng;
                          return LSQUARE_SYM; 
                       }


<custom_set>"]"        {
                          BEGIN INITIAL;
                          colno += yyleng;
                          return RSQUARE_SYM; 
                       }

"{"/[0-9]+"}"          {
                          BEGIN counted_range;
                          colno += yyleng;
			  return LCURLY_SYM;
                       }

"{"/[0-9]+,[0-9]+"}"   {
                          BEGIN counted_range;
                          colno += yyleng;
			  return LCURLY_SYM;
                       }

"{"/,[0-9]+"}"         {
                          BEGIN counted_range;
                          colno += yyleng;
			  return LCURLY_SYM;
                       }

"{"/[0-9]+,"}"         {
                          BEGIN counted_range;
                          colno += yyleng;
			  return LCURLY_SYM;
                       }

<counted_range>","     {
                          /* by specifing the comma here, we interpret as 
			   * a specific comma symbol when inside curly 
			   * braces.  Outside of curly braces, it has no 
			   * special meaning and is a simple character */
                          colno += yyleng;
			  return COMMA_SYM;
                       }

<counted_range>[0-9]+  {  
                          /* The only time we are interested in
			   * numbers as integers rather than characters
			   * is when we are in a counted range. */
			  colno += yyleng;
                          yylval.intval = strtoul(yytext, NULL, 10); 
                          return UNSIGNED; 
                       }


<counted_range>"}"     {
                          BEGIN INITIAL;
                          colno += yyleng;
			  return RCURLY_SYM;
                       }

"|"                    { 
                          colno += yyleng;
                          return ALT_SYM; 
                       }

"("                    {
                          colno += yyleng;
			  return LPAREN_SYM;
                       }

")"                    {
                          colno += yyleng;
			  return RPAREN_SYM;
                       }

"+?"                   {
                          yyerror("Greedy operator +? not supported.");
		       }

"*?"                   {
                          yyerror("Greedy operator *? not supported.");
		       }

"??"                   {
                          yyerror("Greedy operator ?? not supported.");
		       }

"+"                    {
                          colno += yyleng;
			  return PLUS_SYM;
                       }

"*"                    {
                          colno += yyleng;
			  return STAR_SYM;
                       }

"?"                    {
                          colno += yyleng;
			  return QM_SYM;
                       }

"^"                    {
                          colno += yyleng;
			  return CIRCUM_SYM;
                       }

"$"                    {
                          colno += yyleng;
			  return DOLLAR_SYM;
                       }

"/"                    {
                          colno += yyleng;
			  yylval.character = '/';
			  return SLASH_SYM;
                       }

[.]                    {
                          colno += yyleng;
			  return DOT;
                       }


[\\]{symbol_escape}    {
                          colno += yyleng;
			  yylval.character = yytext[1];
			  return CHAR;
                       }

[\\]{control_escape}   {
                          int r;
                          colno += yyleng;
			  yylval.character = parse_char_constant(yytext, &r);
			  return CHAR;
                       }

                    
[\\][xX][0-9a-fA-F]{2}   { 
                          int r;  /* unused here */
			  colno += yyleng;
			  yylval.character = parse_char_constant(yytext, &r);
			  return CHAR; 
                       }

[\\][0-7]+             { 
                          int r;  /* unused here */
			  colno += yyleng;
			  yylval.character = parse_char_constant(yytext, &r);
			  return CHAR; 
                       }

[\\][sSdDwW]           {
                          colno += yyleng;
			  yylval.class_escape = yytext[1];
			  return CHAR_CLASS_ESCAPE;
                       }

[\\][.]                {   /* cisco rules have some of these */
                          /* alternatively - this could be considered a bad 
			   * escape and could be reported as an error.
			   * In escape_sequences.cc, we give a warning and
			   * treat it as the standard character, which is
			   * what Perl does. */
                          int r;  /* unused here */
			  colno += yyleng;
			  yylval.character = parse_char_constant(yytext, &r);
			  return CHAR; 
                       }

[\\].                { 
                          /* alternatively - this could be considered a bad 
			   * escape and could be reported as an error.
			   * In escape_sequences.cc, we give a warning and
			   * treat it as the standard character, which is
			   * what Perl does. */
                          int r;  /* unused here */
			  colno += yyleng;
			  yylval.character = parse_char_constant(yytext, &r);
			  return CHAR; 
                       }

[\t]+                  {
			  colno += yyleng;
                       }

[\n]                   { 
                            colno = 1;
                            lineno++; 
                       }
 
.                      {    
                            colno += yyleng;
			    yylval.character = yytext[0];
			    //printf("lex grabbed: %c\n", yytext[0]);
                            return CHAR;
                       }

%%

/* by including yywrap, we don't need to link in the flex library */
int yywrap(void)
{
   return 1;
}   

extern "C" int yyerror(const char *s)
{
   printf("error: at line %d, col %d, %s near: \"%s\"\n",
	  lineno, colno, s,yytext);
   exit(-1);
}


