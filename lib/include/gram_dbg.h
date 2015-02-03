/*-----------------------------------------------------------------------------
 * File:    gram_dbg.h
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
#ifndef _GRAM_DBG_H_
#define _GRAM_DBG_H_

class dbgG {
  template<class T> static void dmpMemoization(const T& m) {
    assert(0 && "TBD");(void)m;
#if 0
    std::cout << "// memoization: memoize_size=" << m.memoized_size << " size=" << m._num_matches.size() << std::endl;
    for (unsigned int i = 0; i < m.memoized_size; ++i) {
      std::cout << "[" << i << "]= " << m._num_matches[i] << std::endl;
    }
#endif
  }
public:
  static void dmp(const GRule &r) {
    std::cout << r.toString() << "; //";
    if (r.children.size() != 0) {
      for (std::vector<Nonterminal*>::const_iterator in = r.children.begin();
	   in != r.children.end(); ++in) {
	Nonterminal* n = *in;
	std::cout << " " << n->name;
      }
    }
    std::cout << " terminal_length=" << r.terminal_length << std::endl;
    dmpMemoization<Rule>(r);
  }
  static void dmp(const Nonterminal &N) {
    std::cout << "/////\t\t" << N.name << "\t\t///// " << std::endl;
    dmpMemoization<Nonterminal>(N);
    
    for(std::vector<Rule*>::const_iterator rit = N.rules.begin();
	rit != N.rules.end(); ++rit) {
      GRule *r = dynamic_cast<GRule*>(*rit);
      dmp(*r);
    }
  }
  static void dmp(const Grammar& G) {
    for(std::vector<Nonterminal*>::const_iterator nit = G.nonterminals.begin();
	nit != G.nonterminals.end(); ++nit) {
      const Nonterminal *n = *nit;
      dmp(*n);
    }
  }
  static void dotTree(const GRule::GParseTree* pt, const GRule::GParseTree* parent = 0) {
    if (parent == 0) {
      std::cout << "digraph t {" << std::endl;
    }
    std::string str = "\""+pt->r->toString()+"\"";
    std::cout << str << std::endl;
    if (parent) {
      std::string pstr = "\""+parent->r->toString()+"\"";
      std::cout << pstr << " -> " << str << std::endl;
    }
    for (std::vector<ParseTree*>::const_iterator pit = pt->children.begin();
	 pit != pt->children.end(); ++pit) {
      const ParseTree* child = (*pit);
      dotTree(dynamic_cast<const GRule::GParseTree*>(child), pt);//TBD: hack
    }
    if (parent == 0) {
      std::cout << "}" << std::endl;
    }
  }
};

#endif
