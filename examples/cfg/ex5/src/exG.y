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
int num_T = 10;

/* This is the language:    /12[^3]*3[^4]*4/, where '.' is [a-zA-Z0-9_]
 *    i.e. the language is  /12[a-zA-Z0-24-9_]*3[a-zA-Z0-35-9_]*4/
 * This is grammar G:
 * *** grammar rules ***
 * r0: L -> '12' T '3' F '4'
 * r1: T -> [^3]*
 * r2: F -> [^4]*
 */
%}


%union {//TBD:simplify: "Rule::ParseTree*  _ParseTree;" should be enough
  LRule::LParseTree*  _LParseTree;
  GRule::GParseTree*  _GParseTree;
}

%token <_LParseTree> _T _F _c12
%type  <_GParseTree> L
%type  <_LParseTree> _TS _FS

%%
L:          _c12 _TS '3' _FS '4'
                { //rule 0
		  g_exG_ParseTree = $$ = mkgptn(r0, 2, $2, $4);
		  if(DBG_LEX_YACC)
		    printf("\nDBG:YACC\n");
		}
                ;
_TS:
{
  $$ = new LRule::LParseTree(r1, byte_copy("", 0), 0);
}
_TS:        _T {$$=$1;}
_FS:
{
  $$ = new LRule::LParseTree(r2, byte_copy("", 0), 0);
}
_FS:        _F {$$=$1;}
%%

extern "C" int yyerror(const char *s)
{
   //printf("error: at line %d, col %d, %s near: \"%s\"\n",
   //	  lineno, colno, s,yytext);
   printf("error: %s", s);
   exit(-1);
}
