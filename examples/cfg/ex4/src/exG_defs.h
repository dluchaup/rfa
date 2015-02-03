/*-----------------------------------------------------------------------------
 * File:    exG_defs.h
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
#ifndef _EX2G_DEFS_
#define _EX2G_DEFS_
#include "rank_cfg.h"

/* This is grammar G:
 * *** grammar rules ***
 * r0: L -> T T T T T T T T T T
 * r1: T -> .{100}
 */
extern Grammar G;

/************* Terminals ***************/
/********  normal Nonterminals *********/
extern Nonterminal *L;
/******** lexical Nonterminals *********/
extern Nonterminal *T;

/********  normal Rules *********/
extern GRule *r0; //r0: L -> T T T T T T T T T T
/******** lexical Rules *********/
extern LRule *r1; //r1: T -> .{100}

extern void init_G();
extern void init_G(unsigned int slice_size);
extern const unsigned int num_T;
#define DBG_LEX_YACC 0

#define yyparse exG_parse
#define yylval exG_lval
extern int yyparse();
extern ParseTree *g_exG_ParseTree;

///////////// grammar parsing stuff ///////////////////
ParseTree *pt_from_file(FILE *fp);
ParseTree *pt_from_mem(const char *data, int len);
ParseTree *pt_from_mem(const std::string &data); //TBD: move in shared file
#endif
