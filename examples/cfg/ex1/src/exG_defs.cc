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

/* Grammar example in exG_defs. G is:
 * r0: S  -> ( Id )
 * r1: S  -> ( Id + S)
 * r2: Id -> x
 * r3: Id -> y
 *
 */
Grammar G;

/************* Terminals ***************/
Terminal *lp;
Terminal *rp;
Terminal *plus;
Terminal *x;
Terminal *y;

/************* Nonterminals ************/
Nonterminal *S;
Nonterminal *Id;

/************* Rules *******************/
GRule *r0; //r0: S  -> ( Id )
GRule *r1; //r1: S  -> ( Id + S)
GRule *r2; //r2: Id -> x
GRule *r3; //r3: Id -> y

void init_G() {
  /*** Here is how to build a representation, G, for the above grammar ***/  
  lp = new Terminal("(");
  rp = new Terminal(")");
  plus = new Terminal("+");
  x = new Terminal("x");
  y = new Terminal("y");
  
  S = new Nonterminal("S");
  Id = new Nonterminal("Id");
  
  G.terminals.push_back(lp);
  G.terminals.push_back(rp);
  G.terminals.push_back(plus);
  G.terminals.push_back(x);
  G.terminals.push_back(y);

  G.nonterminals.push_back(S); G.set_S(S);
  G.nonterminals.push_back(Id);
  
  r0 = new GRule(S, "r0");
  r0->append_symbol(lp);r0->append_symbol(Id);r0->append_symbol(rp);
  
  r1 = new GRule(S, "r1");
  r1->append_symbol(lp);r1->append_symbol(Id);r1->append_symbol(plus);r1->append_symbol(S);r1->append_symbol(rp);
  
  r2 = new GRule(Id, "r2");
  r2->append_symbol(x);
  
  r3 = new GRule(Id, "r3");
  r3->append_symbol(y);
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

ParseTree *pt_from_file(FILE *fp)
{
	eval_data.type = EVAL_SRC_FILE;
	eval_data.src.file = fp;
	//lineno=1;
	yyparse();
	return g_exG_ParseTree;
}

ParseTree *pt_from_mem(const char *data, int len)
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
