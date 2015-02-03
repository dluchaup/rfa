/*-----------------------------------------------------------------------------
 * File:    exG.y
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
#include "gram.h"
#include "exG_defs.h"

int yylex();
extern "C" int yyerror(const char *);


ParseTree *g_exG_ParseTree = NULL;
/* g++ requires these */
int exGlex();
extern "C" int exGerror(const char *);
const unsigned int num_T = 10;

/* This is grammar G:
 * *** grammar rules ***
 * r0: L -> T T T T T T T T T T
 * r1: T -> .{100}
 */
%}


%union {//TBD:simplify: "Rule::ParseTree*  _ParseTree;" should be enough
  LRule::LParseTree*  _LParseTree;
  GRule::GParseTree*  _GParseTree;
}

%token <_LParseTree> _T
%type  <_GParseTree> L

%%
L:          _T _T _T _T _T _T _T _T _T _T
                { //rule 0
		  assert(r0->right.size() == num_T);
		  g_exG_ParseTree = $$ = mkgptn(r0, 10,$1,$2,$3,$4,$5,$6,$7,$8,$9,$10);
		  //g_exG_ParseTree = $$ = mkgptn(r0, num_T,$1,$2);
		  if(DBG_LEX_YACC)
		    printf("\nDBG:YACC\n");
		}
                ;
%%

extern "C" int yyerror(const char *s)
{
   //printf("error: at line %d, col %d, %s near: \"%s\"\n",
   //	  lineno, colno, s,yytext);
   printf("error: %s", s);
   exit(-1);
}

extern int switch_getinput(char *buf, int maxlen);
int exG_lex (void) {
  char buf_T[100];
  int yy_n_chars = switch_getinput(buf_T, 100);
  if (yy_n_chars == 100) {
    yylval._LParseTree = new LRule::LParseTree(r1, byte_copy(buf_T, 100), 100);
    return _T;
  }
  else if (yy_n_chars != 0) {
    yyerror("Lexer error: unnexpected input length\n");
    exit(1);
  }
  return 0;
}

int yywrap(void)
{
   return 1;
}   

