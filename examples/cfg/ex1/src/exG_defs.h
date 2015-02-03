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
#ifndef _EX1G_DEFS_
#define _EX1G_DEFS_
#include "rank_cfg.h"

/* Grammar example. G is:
 * r0: S  -> ( Id )
 * r1: S  -> ( Id + S)
 * r2: Id -> x
 * r3: Id -> y
 *
 */
extern Grammar G;

/************* Terminals ***************/
extern Terminal *lp;
extern Terminal *rp;
extern Terminal *plus;
extern Terminal *x;
extern Terminal *y;

/************* Nonterminals ************/
extern Nonterminal *S;
extern Nonterminal *Id;

/************* Rules *******************/
extern GRule *r0; //r0: S  -> ( Id )
extern GRule *r1; //r1: S  -> ( Id + S)
extern GRule *r2; //r2: Id -> x
extern GRule *r3; //r3: Id -> y

extern void init_G();
extern void init_G(unsigned int slice_size);

#define yyparse exG_parse
extern int yyparse();
extern ParseTree *g_exG_ParseTree;

///////////// grammar parsing stuff ///////////////////
ParseTree *pt_from_file(FILE *fp);
ParseTree *pt_from_mem(const char *data, int len);
ParseTree *pt_from_mem(const std::string &data); //TBD: move in shared file
#endif
