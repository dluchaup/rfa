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
#include <string>

/* This is grammar G:
 * *** grammar rules ***
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

class exG : public rank_cfg {
  public:
	/************* Terminals ***************/
	Terminal *lp;
	Terminal *rp;
	Terminal *plus;
	
	/********  normal Nonterminals *********/
	Nonterminal *L;
	Nonterminal *S;
	Nonterminal *Exp;
	/******** lexical Nonterminals *********/
	Nonterminal *Num;
	Nonterminal *IDENTIFIER;
	Nonterminal *INT;
	Nonterminal *FLOAT;
	
	/********  normal Rules *********/
	GRule *r00;//r00: L  -> S
	GRule *r01;//r01: L  -> S L
	GRule *r0; //r0: S  -> ( Exp )
	GRule *r1; //r1: S  -> ( Exp + S)
	GRule *r2; //r2: Exp -> IDENTIFIER
	GRule *r3; //r3: Exp -> Num
	GRule *r4; //r4: Num -> INT
	GRule *r5; //r5: Num -> FLOAT
	/******** lexical Rules *********/
	LRule *r6; //r6: IDENTIFIER -> {L}({L}|{D})*
	LRule *r7; //r7: INT -> 0[xX]{H}+
	LRule *r8; //r8: INT -> {D}+
	LRule *r9; //r9: FLOAT -> {D}*"."{D}+
	LRule *r10;//r10: FLOAT -> {D}+"."{D}*
	//////////////////////////////////
	exG(unsigned int slice_size);
	void init_G();
};

extern exG *g_exG;

#define yyparse exG_parse
#define yylval exG_lval
extern int yyparse();

//extern ParseTree *g_exG_ParseTree;
#define g_exG_ParseTree (g_exG->pi->pt_result)

///////////// grammar parsing stuff ///////////////////
ParseTree *pt_from_file(FILE *fp);
ParseTree *pt_from_mem(const char *data, int len);
ParseTree *pt_from_mem(const std::string &data);

#endif
