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
exG *g_exG = NULL; //TBD: turn into ex2G::G
void exG::init_G() {
  pi = new ParserInterface(exG_parse);
  /*** Here is how to build a representation, G, for the above grammar ***/  
  lp = new Terminal("(");
  rp = new Terminal(")");
  plus = new Terminal("+");
  
  L = new Nonterminal("L");
  S = new Nonterminal("S");
  Exp = new Nonterminal("Exp");
  Num = new Nonterminal("Num");
  IDENTIFIER = new Nonterminal("IDENTIFIER");
  INT = new Nonterminal("INT");
  FLOAT = new Nonterminal("FLOAT");

  G.terminals.push_back(lp);
  G.terminals.push_back(rp);
  G.terminals.push_back(plus);
    
  G.nonterminals.push_back(L); G.index_start = 0;
  G.nonterminals.push_back(S); //G.index_start = 0;
  G.nonterminals.push_back(Exp);
  G.nonterminals.push_back(Num);
  G.nonterminals.push_back(IDENTIFIER);
  G.nonterminals.push_back(INT);
  G.nonterminals.push_back(FLOAT);
  
  r00 = new GRule(L, "r00");
  r00->append_symbol(S);
  
  r01 = new GRule(L, "r01");
  r01->append_symbol(S); r01->append_symbol(L);
  
  r0 = new GRule(S, "r0");
  r0->append_symbol(lp);r0->append_symbol(Exp);r0->append_symbol(rp);
  
  r1 = new GRule(S, "r1");
  r1->append_symbol(lp);r1->append_symbol(Exp);r1->append_symbol(plus);r1->append_symbol(S);r1->append_symbol(rp);
  
  r2 = new GRule(Exp, "r2");
  r2->append_symbol(IDENTIFIER);
  
  r3 = new GRule(Exp, "r3");
  r3->append_symbol(Num);
  
  r4 = new GRule(Num, "r4");
  r4->append_symbol(INT);

  r5 = new GRule(Num, "r5");
  r5->append_symbol(FLOAT);

  // NOTE: we replaced the lex macros {D},{L},{H}. Ex: replaced {D} with [0-9]
  unsigned int lim = 3;
  r6 = new LRule(IDENTIFIER, new rank_ffa_tab_t("/[a-zA-Z_]([a-zA-Z_]|[0-9])*/"), "r6", 2);
  r7 = new LRule(INT, new rank_ffa_tab_t("/0[xX][a-fA-F0-9]+/"), "r7", lim);
  r8 = new LRule(INT, new rank_ffa_tab_t("/[0-9]+/"), "r8", lim);
#if 1 //ambiguous
  r9 = new LRule(FLOAT, new rank_ffa_tab_t("/[0-9]*\\.[0-9]+/"), "r9", lim);
  r10 = new LRule(FLOAT, new rank_ffa_tab_t("/[0-9]+\\.[0-9]*/"), "r10", lim);
#else //nonambiguous
  r9 = new LRule(FLOAT, new rank_ffa_tab_t("/([0-9]*\\.[0-9]+)|([0-9]+\\.[0-9]*)/"), "r9", lim);
#endif
}

exG::exG(unsigned int slice_size) {
  init_G();
  (const_cast<Nonterminal*>(G.S()))->memoize(slice_size, slice_size);
}
