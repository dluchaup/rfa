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

/* This is grammar G:
 * *** grammar rules ***
 * r0: L -> T T T T T T T T T T
 * r1: T -> .{100}
 */
Grammar G;
/************* Terminals ***************/
/********  normal Nonterminals *********/
Nonterminal *L;
/******** lexical Nonterminals *********/
Nonterminal *T;

/********  normal Rules *********/
GRule *r0; //r0: L -> T T T T T T T T T T
/******** lexical Rules *********/
LRule *r1; //r1: T -> .{100}

void init_G() {
  /*** Here is how to build a representation, G, for the above grammar ***/  
  L = new Nonterminal("L");
  T = new Nonterminal("R");
  G.nonterminals.push_back(L);
  G.set_S(L); //set the start symbol
  G.nonterminals.push_back(T);
  
  r0 = new GRule(L, "r0");
  for (unsigned int i = 0; i < num_T; ++i)
    r0->append_symbol(T);
  assert(r0->right.size() == num_T);
  
  unsigned int lim = 100;
  //r1 = new LRule(T, new rank_ffa_tab_t("/.{100}/"), "r0", lim);
  r1 = new LRule(T, new rank_ffa_tab_t("/[a-zA-Z0-9_]{100}/"), "r0", lim);
}

void init_G(unsigned int slice_size) {
  init_G();
  (const_cast<Nonterminal*>(G.S()))->memoize(slice_size, slice_size);
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
