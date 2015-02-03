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


/* g++ requires these */
int exGlex();
extern "C" int exGerror(const char *);

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


%union {//TBD:simplify: "Rule::ParseTree*  _ParseTree;" should be enough
  LRule::LParseTree*  _LParseTree;
  GRule::GParseTree*  _GParseTree;
}

%token <_LParseTree> _IDENTIFIER _FLOAT _INT
%type  <_GParseTree> L S Exp Num

%%
L:          S
                {  //rule 00
		  g_exG_ParseTree = $$ = mkgptn(g_exG->r00, 1, $1);
		}
                ;
L:          S L
                {  //rule 01
		  g_exG_ParseTree = $$ = mkgptn(g_exG->r01, 2, $1, $2);
		}
                ;
S:          '(' Exp ')'
                {  //rule 0
		  $$ = mkgptn(g_exG->r0, 1, $2);
		}
                ;

S:          '(' Exp '+' S ')'
                {  //rule 1
		  $$= mkgptn(g_exG->r1, 2, $2, $4);
		}
                ;

Exp:         _IDENTIFIER
                {  //rule 2
		  $$ = mkgptn(g_exG->r2, 1, $1);
		}

Exp:         Num
                {  //rule 3
		  $$ = mkgptn(g_exG->r3, 1, $1);
		}

Num:         _INT
                {  //rule 4
		  $$ = mkgptn(g_exG->r4, 1, $1);
		}

Num:         _FLOAT
                {  //rule 5
		  $$ = mkgptn(g_exG->r5, 1, $1);
		}

%%

extern "C" int yyerror(const char *s)
{
   //printf("error: at line %d, col %d, %s near: \"%s\"\n",
   //	  lineno, colno, s,yytext);
   printf("error: %s", s);
   exit(-1);
}
