/*-----------------------------------------------------------------------------
 * File:    rank_cfg.h
 * Author:  Daniel Luchaup
 * Date:    30 January 2014
 * Copyright 2013-2014 Daniel Luchaup luchaup@cs.wisc.edu
 *
 * This file contains unpublished confidential proprietary work of
 * Daniel Luchaup, Department of Computer Sciences, University of
 * Wisconsin--Madison.  No use of any sort, including execution, modification,
 * copying, storage, distribution, or reverse engineering is permitted without
 * the express written consent (for each kind of use) of Daniel Luchaup.
 *
 *---------------------------------------------------------------------------*/
#ifndef RANK_CFG_H
#define RANK_CFG_H
#include <assert.h>
#include <bitset>
#include <fstream>
#include <iostream>
#include <vector>
#include <set>
#include <string>

#include "ranker.h"
#include "gram.h"

/************************************************/
struct ParserInterface {
  ///////////// grammar parsing stuff ///////////////////
  // copied from Randy's parse_utils. TBD: refactor this.
  typedef int (*YYPARSER)();
  YYPARSER yyparse;
  ParserInterface(YYPARSER yyparse0)  : yyparse(yyparse0) {;}
  typedef enum {
    EVAL_SRC_FILE,
    EVAL_SRC_MEM
  } eval_source;
	
  /* Ugly, but it gets the job done... */
  struct {
    eval_source type;
    union {
      FILE *file;
	    struct {
		    const char *data;
		    int len;
	    } mem;
    } src;
  } eval_data;// TBD use one of this per yacc file, to allow multilpe parsers
  ParseTree *pt_result;
  ParseTree *pt_from_file(FILE *fp) {
          pt_result = NULL;
	  eval_data.type = EVAL_SRC_FILE;
	  eval_data.src.file = fp;
	  this->yyparse();
	  return pt_result;
  }
	
  ParseTree *pt_from_mem(const char *data, int len) {
          pt_result = NULL;
	  eval_data.type = EVAL_SRC_MEM;
	  eval_data.src.mem.data = data;
	  eval_data.src.mem.len = len;
	  this->yyparse();
	  return pt_result;
  }
  
  ParseTree *pt_from_mem(const std::string &data) {
    return pt_from_mem(data.c_str(), data.length());
  }
  
  int switch_getinput(char *buf, int maxlen) {
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
};
/************************************************/
/********** rank_cfg ****************************/
class rank_cfg : public rankerinterface {
public:
  ParserInterface *pi;
  Grammar G;
  ~rank_cfg() {
	  delete pi;
  }
  inline MPInt rank(const std::string &str) {
    GRule::GParseTree *pt = static_cast<GRule::GParseTree*>(pi->pt_from_mem(str));
    MPInt r = G.S()->rank(pt);
    delete pt;
    return r;
  }
  std::string unrank(MPInt &r, unsigned int len) {//note: this will set r to 0;
    ParseTree *pt_unrank = G.S()->unrank(r, len);
    std::string str = pt_unrank->flatten();
    delete pt_unrank;
    return str;
  }
  const MPInt& num_matches(unsigned int len) {
    return G.S()->num_matches(len);
  }
  inline virtual unsigned int get_memoized_size() const {
	  return const_cast<Nonterminal*>(G.S())->get_memoized_size();
  }
  //////////////////////////////
  virtual void update_matches(unsigned int len) {
    (const_cast<Nonterminal*>(G.S()))->memoize(len, len);
  }
  virtual size_t get_memoized_bits(unsigned int *nz_entries=NULL) const {
    assert(false && "TBD: unimplemented ...");(void)nz_entries;
    return 0;
  }
  //////////////////////////////
  virtual bool rank(MPInt & pc, const char *str, size_t len) {
    GRule::GParseTree *pt = static_cast<GRule::GParseTree*>(pi->pt_from_mem(str, len));
    if (pt) {
      pc = G.S()->rank(pt);
      delete pt;
      return true;
    }
    return false;
  }
  virtual bool matches(const std::string & str) {
    // This may fail: the scanner may die if parser fails. TBD: use exceptions
    GRule::GParseTree *pt = NULL;
    bool res = (pt = static_cast<GRule::GParseTree*>(pi->pt_from_mem(str)));
    delete pt;
    return res;
  }
  virtual bool unrank(MPInt& pc, size_t len, std::string& res) {//note: this will set pc to 0;
    ParseTree *pt_unrank = G.S()->unrank(pc, len);
    bool ok = (pt_unrank != NULL);
    if (ok) {
      res = pt_unrank->flatten();
      delete pt_unrank;
    }
    return ok;
  }
  virtual void report_size(const char*, int) {assert(0&&"Unimplemented");};
#if 0
  std::string unrank(MPInt &r, unsigned int len) {//note: this will set r to 0;
    ParseTree *pt_unrank = S()->unrank(r, len);
    std::string str = pt_unrank->flatten();
    delete pt_unrank;
    return str;
  }
#endif
};
#endif //RANK_CFG_H
