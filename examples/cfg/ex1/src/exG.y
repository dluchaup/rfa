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

/* This is grammar G:
 * r0: S  -> ( Id )
 * r1: S  -> ( Id + S)
 * r2: Id -> x
 * r3: Id -> y
 */
%}


%union {
      GRule::GParseTree*  _GParseTree;
}

//%token <_Terminal>    LP RP X Y
%type  <_GParseTree> S Id

%%
S:          '(' Id ')'
                {  //rule 0
		  g_exG_ParseTree = $$ = mkgptn(r0, 1, $2);
		}
                ;

S:          '(' Id '+' S ')'
                {  //rule 1
		  g_exG_ParseTree = $$= mkgptn(r1, 2, $2, $4);
		}
                ;

Id:         'x'
                {  //rule 2
		  $$ = mkgptn(r2, 0);
		}

Id:         'y'
                {  //rule 3
		  $$ = mkgptn(r3, 0);
		}

%%

extern "C" int yyerror(const char *s)
{
   //printf("error: at line %d, col %d, %s near: \"%s\"\n",
   //	  lineno, colno, s,yytext);
   printf("error: %s", s);
   exit(-1);
}
