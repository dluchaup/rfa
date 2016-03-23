/*-----------------------------------------------------------------------------
 * File:    exG_defs.cc
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
#include <string.h>
#include "exG_defs.h"
#include "ffa.h"

/* This is the language:    /12[^3]*3[^4]*4/, where '.' is [a-zA-Z0-9_]
 *    i.e. the language is  /12[a-zA-Z0-24-9_]*3[a-zA-Z0-35-9_]*4/
 * This is grammar G:
 * *** grammar rules ***
 * r0: L -> '12' T '3' F '4'
 * r1: T -> [^3]*
 * r2: F -> [^4]*
 */
Grammar G;
/************* Terminals ***************/
Terminal *c12;
Terminal *c3;
Terminal *c4;
/********  normal Nonterminals *********/
Nonterminal *L;
/******** lexical Nonterminals *********/
Nonterminal *T;
Nonterminal *F;

/********  normal Rules *********/
GRule *r0;
/******** lexical Rules *********/
LRule *r1;
LRule *r2;

void init_G() {
  /*** Here is how to build a representation, G, for the above grammar ***/
  c12 = new Terminal("12");
  c3 = new Terminal("3");
  c4 = new Terminal("4");

	
  L = new Nonterminal("L");
  T = new Nonterminal("T");
  F = new Nonterminal("F");
  G.nonterminals.push_back(L);
  G.set_S(L); //set the start symbol
  G.nonterminals.push_back(T);
  G.nonterminals.push_back(F);

  r0 = new GRule(L, "r0");
  r0->append_symbol(c12); r0->append_symbol(T); r0->append_symbol(c3); r0->append_symbol(F); r0->append_symbol(c4);

  
  //unsigned int lim = 10000;
  r1 = new LRule(T, new rank_ffa_tab_t("/[a-zA-Z0-24-9_]*/"), "r1");
  r2 = new LRule(F, new rank_ffa_tab_t("/[a-zA-Z0-35-9_]*/"), "r2");
}

void init_G(unsigned int slice_size) {
  init_G();
  {MyTime tm("init_G:memoize");
    (const_cast<Nonterminal*>(G.S()))->memoize(slice_size, slice_size);
  }
}

///////////// grammar parsing stuff ///////////////////
// copied from Randy's parse_utils. TBD: refactor this.
typedef enum {
	EVAL_SRC_FILE,
	EVAL_SRC_MEM
} eval_source;


/* Ugly, but it gets the job done... */
static struct {
	eval_source type;
	union {
		FILE *file;
		struct {
			const char *data;
			int len;
		} mem;
	} src;
} eval_data;// TBD use one of this per yacc file, to allow multilpe parsers

extern ParseTree *g_exG_ParseTree;

ParseTree *pt_from_file(FILE *fp) //TBD: move in shared file
{
	eval_data.type = EVAL_SRC_FILE;
	eval_data.src.file = fp;
	//lineno=1;
	yyparse();
	return g_exG_ParseTree;
}

ParseTree *pt_from_mem(const char *data, int len) //TBD: move in shared file
{
	eval_data.type = EVAL_SRC_MEM;
	eval_data.src.mem.data = data;
	eval_data.src.mem.len = len;
	//lineno = line;
	yyparse();
	return g_exG_ParseTree;
}

ParseTree *pt_from_mem(const std::string &data) //TBD: move in shared file
{
  return pt_from_mem(data.c_str(), data.length());
}

int switch_getinput(char *buf, int maxlen)
{
   int retval = 0;
   
   switch (eval_data.type) {
      case EVAL_SRC_FILE:
	 retval = fread(buf, 1, maxlen, eval_data.src.file);
	 break;
      case EVAL_SRC_MEM:
	 if ( maxlen > eval_data.src.mem.len ) {
	    maxlen = eval_data.src.mem.len;
	 }
	 memcpy(buf, eval_data.src.mem.data, maxlen);
	 eval_data.src.mem.data += maxlen;
	 eval_data.src.mem.len -= maxlen;
	 retval = maxlen;
	 break;
   }
   return retval;
}
