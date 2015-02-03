/*-----------------------------------------------------------------------------
 * File:    exG_main.cc
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
#include <iostream>
#include "gram.h"
#include "MyTime.h"
#include <cstdarg>
#include "exG_defs.h"
#include "gram_dbg.h"
#include "zoptions.h"
#include "dbg.h"


void test_unrank_rank(unsigned int max_len, unsigned int count);
/* r0: S  -> ( Id )
 * r1: S  -> ( Id + S)
 * r2: Id -> x
 * r3: Id -> y
 */
void old1_test_gram() {
  MyTime("test_gram");
  /*** Initialize the global representation for the above grammar ***/  
  init_G(10);
  dbgG::dmp(G);

  /*** Here is how to build various parse trees for the above grammar, G  ***/
  /* NOTE: this doesn't perform real parsing, just "hand" tree construction */  
  {
    // Parse tree for: (x) is r0(r2)
    GRule::GParseTree *ptx = new GRule::GParseTree(r0);
    ptx->push_child(new GRule::GParseTree(r2));
    dbgG::dotTree(ptx, NULL);
    std::string flat = ptx->flatten();
    std::cout << "flat:" << flat << std::endl;
    assert(0 == flat.compare("(x)"));
    MPInt r_ptx = S->rank(ptx);
    std::cout << "rank ptx= " << r_ptx << std::endl;    

    ParseTree *pt_unrank = S->unrank(r_ptx, flat.length());
    std::string flat_unrank = pt_unrank->flatten();
    std::cout << "flat_unrank=" << flat_unrank << std::endl;
    assert(0 == flat_unrank.compare(flat));
    delete ptx;
    delete pt_unrank;
  }
  {
    // Parse tree for: (y) is r0(r3)
    GRule::GParseTree *pty = new GRule::GParseTree(r0);
    pty->push_child(new GRule::GParseTree(r3));
    dbgG::dotTree(pty, NULL);
    std::string flat = pty->flatten();
    std::cout << "flat:" << flat << std::endl;
    assert(0 == flat.compare("(y)"));
    MPInt r_pty = S->rank(pty);
    std::cout << "rank pty= " << r_pty << std::endl;
    
    ParseTree *pt_unrank = S->unrank(r_pty, flat.length());
    std::string flat_unrank = pt_unrank->flatten();
    std::cout << "flat_unrank=" << flat_unrank << std::endl;
    assert(0 == flat_unrank.compare(flat));
    delete pty;
    delete pt_unrank;
  }
  {
    // Parse tree for: (x + (x)) is r1(r2r0(r2))
    GRule::GParseTree *ptxx = new GRule::GParseTree(r1);
    ptxx->push_child(new GRule::GParseTree(r2));

    // the r0(r2) part ... as before
    GRule::GParseTree *ptx = new GRule::GParseTree(r0);
    ptx->push_child(new GRule::GParseTree(r2));
    ptxx->push_child(ptx);
    dbgG::dotTree(ptxx, NULL);
    std::string flat = ptxx->flatten();
    std::cout << "flat:" << flat << std::endl;
    assert(0 == flat.compare("(x+(x))"));
    MPInt r_ptxx = S->rank(ptxx);
    std::cout << "rank ptxx= " << r_ptxx << std::endl;
    
    ParseTree *pt_unrank = S->unrank(r_ptxx, flat.length());
    std::string flat_unrank = pt_unrank->flatten();
    std::cout << "flat_unrank=" << flat_unrank << std::endl;
    assert(0 == flat_unrank.compare(flat));
    delete ptxx;
    delete pt_unrank;
  }
  {
    // Parse tree for: (x + (y)) is r1(r2r0(r3))
    GRule::GParseTree *ptxy = new GRule::GParseTree(r1);
    ptxy->push_child(new GRule::GParseTree(r2));

    // the r0(r3) part ... as before
    GRule::GParseTree *pty = new GRule::GParseTree(r0);
    pty->push_child(new GRule::GParseTree(r3));
    ptxy->push_child(pty);
    dbgG::dotTree(ptxy, NULL);
    std::string flat = ptxy->flatten();
    std::cout << "flat:" << flat << std::endl;
    assert(0 == flat.compare("(x+(y))"));
    MPInt r_ptxy = S->rank(ptxy);
    std::cout << "rank ptxy= " << r_ptxy << std::endl;
    
    ParseTree *pt_unrank = S->unrank(r_ptxy, flat.length());
    std::string flat_unrank = pt_unrank->flatten();
    std::cout << "flat_unrank=" << flat_unrank << std::endl;
    assert(0 == flat_unrank.compare(flat));
    delete ptxy;
    delete pt_unrank;
  }
  {
    // Parse tree for: (y + (x)) is r1(r3r0(r2))
    GRule::GParseTree *ptyx = new GRule::GParseTree(r1);
    ptyx->push_child(new GRule::GParseTree(r3));

    // the r0(r2) part ... as before
    GRule::GParseTree *ptx = new GRule::GParseTree(r0);
    ptx->push_child(new GRule::GParseTree(r2));
    ptyx->push_child(ptx);
    dbgG::dotTree(ptyx, NULL);
    std::string flat = ptyx->flatten();
    std::cout << "flat:" << flat << std::endl;
    assert(0 == flat.compare("(y+(x))"));
    MPInt r_ptyx = S->rank(ptyx);
    std::cout << "rank ptyx= " << r_ptyx << std::endl;
    
    ParseTree *pt_unrank = S->unrank(r_ptyx, flat.length());
    std::string flat_unrank = pt_unrank->flatten();
    std::cout << "flat_unrank=" << flat_unrank << std::endl;
    assert(0 == flat_unrank.compare(flat));
    delete ptyx;
    delete pt_unrank;
  }
  {
    // Parse tree for: (y + (y)) is r1(r3r0(r3))
    GRule::GParseTree *ptyy = new GRule::GParseTree(r1);
    ptyy->push_child(new GRule::GParseTree(r3));

    // the r0(r3) part ... as before
    GRule::GParseTree *pty = new GRule::GParseTree(r0);
    pty->push_child(new GRule::GParseTree(r3));
    ptyy->push_child(pty);
    dbgG::dotTree(ptyy, NULL);
    std::string flat = ptyy->flatten();
    std::cout << "flat:" << flat << std::endl;
    assert(0 == flat.compare("(y+(y))"));
    MPInt r_ptyy = S->rank(ptyy);
    std::cout << "rank ptyy= " << r_ptyy << std::endl;

    ParseTree *pt_unrank = S->unrank(r_ptyy, flat.length());
    std::string flat_unrank = pt_unrank->flatten();
    std::cout << "flat_unrank=" << flat_unrank << std::endl;
    assert(0 == flat_unrank.compare(flat));
    delete ptyy;
    delete pt_unrank;
  }
}

/* r0: S  -> ( Id )
 * r1: S  -> ( Id + S)
 * r2: Id -> x
 * r3: Id -> y
 */
void old2_test_gram() {
  MyTime("test_gram");
  /*** Initialize the global representation for the above grammar ***/  
  init_G(10);
  dbgG::dmp(G);

  /*** Here is how to build various parse trees for the above grammar, G  ***/
  /* NOTE: this doesn't perform real parsing, just "hand" tree construction */  
  {
    // Parse tree for: (x) is r0(r2)
    GRule::GParseTree *ptx = mkgpt(r0, mkgpt(r2, NULL), NULL);
    dbgG::dotTree(ptx, NULL);
    std::string flat = ptx->flatten();
    std::cout << "flat:" << flat << std::endl;
    assert(0 == flat.compare("(x)"));
    MPInt r_ptx = S->rank(ptx);
    std::cout << "rank ptx= " << r_ptx << std::endl;    

    ParseTree *pt_unrank = S->unrank(r_ptx, flat.length());
    std::string flat_unrank = pt_unrank->flatten();
    std::cout << "flat_unrank=" << flat_unrank << std::endl;
    assert(0 == flat_unrank.compare(flat));
  }
  {
    // Parse tree for: (y) is r0(r3)
    GRule::GParseTree *pty = mkgpt(r0, mkgpt(r3, NULL), NULL);
    dbgG::dotTree(pty, NULL);
    std::string flat = pty->flatten();
    std::cout << "flat:" << flat << std::endl;
    assert(0 == flat.compare("(y)"));
    MPInt r_pty = S->rank(pty);
    std::cout << "rank pty= " << r_pty << std::endl;
    
    ParseTree *pt_unrank = S->unrank(r_pty, flat.length());
    std::string flat_unrank = pt_unrank->flatten();
    std::cout << "flat_unrank=" << flat_unrank << std::endl;
    assert(0 == flat_unrank.compare(flat));
  }
  {
    // Parse tree for: (x + (x)) is r1(r2r0(r2))
    GRule::GParseTree *ptxx = mkgpt(r1,
			     mkgpt(r2, NULL),
			     mkgpt(r0, mkgpt(r2, NULL), NULL),
			     NULL);
    
    dbgG::dotTree(ptxx, NULL);
    std::string flat = ptxx->flatten();
    std::cout << "flat:" << flat << std::endl;
    assert(0 == flat.compare("(x+(x))"));
    MPInt r_ptxx = S->rank(ptxx);
    std::cout << "rank ptxx= " << r_ptxx << std::endl;
    
    ParseTree *pt_unrank = S->unrank(r_ptxx, flat.length());
    std::string flat_unrank = pt_unrank->flatten();
    std::cout << "flat_unrank=" << flat_unrank << std::endl;
    assert(0 == flat_unrank.compare(flat));
  }
  {
    // Parse tree for: (x + (y)) is r1(r2r0(r3))
    GRule::GParseTree *ptxy = mkgpt(r1,
			     mkgpt(r2, NULL),
			     mkgpt(r0, mkgpt(r3, NULL), NULL),
			     NULL);
    
    dbgG::dotTree(ptxy, NULL);
    std::string flat = ptxy->flatten();
    std::cout << "flat:" << flat << std::endl;
    assert(0 == flat.compare("(x+(y))"));
    MPInt r_ptxy = S->rank(ptxy);
    std::cout << "rank ptxy= " << r_ptxy << std::endl;
    
    ParseTree *pt_unrank = S->unrank(r_ptxy, flat.length());
    std::string flat_unrank = pt_unrank->flatten();
    std::cout << "flat_unrank=" << flat_unrank << std::endl;
    assert(0 == flat_unrank.compare(flat));
  }
  {
    // Parse tree for: (y + (x)) is r1(r3r0(r2))
    GRule::GParseTree *ptyx = mkgpt(r1,
			     mkgpt(r3, NULL),
			     mkgpt(r0, mkgpt(r2, NULL), NULL),
			     NULL);

    dbgG::dotTree(ptyx, NULL);
    std::string flat = ptyx->flatten();
    std::cout << "flat:" << flat << std::endl;
    assert(0 == flat.compare("(y+(x))"));
    MPInt r_ptyx = S->rank(ptyx);
    std::cout << "rank ptyx= " << r_ptyx << std::endl;
    
    ParseTree *pt_unrank = S->unrank(r_ptyx, flat.length());
    std::string flat_unrank = pt_unrank->flatten();
    std::cout << "flat_unrank=" << flat_unrank << std::endl;
    assert(0 == flat_unrank.compare(flat));
  }
  {
    // Parse tree for: (y + (y)) is r1(r3r0(r3))
    GRule::GParseTree *ptyy = mkgpt(r1,
			     mkgpt(r3, NULL),
			     mkgpt(r0, mkgpt(r3, NULL), NULL),
			     NULL);
    
    dbgG::dotTree(ptyy, NULL);
    std::string flat = ptyy->flatten();
    std::cout << "flat:" << flat << std::endl;
    assert(0 == flat.compare("(y+(y))"));
    MPInt r_ptyy = S->rank(ptyy);
    std::cout << "rank ptyy= " << r_ptyy << std::endl;

    ParseTree *pt_unrank = S->unrank(r_ptyy, flat.length());
    std::string flat_unrank = pt_unrank->flatten();
    std::cout << "flat_unrank=" << flat_unrank << std::endl;
    assert(0 == flat_unrank.compare(flat));
  }
}

/* r0: S  -> ( Id )
 * r1: S  -> ( Id + S)
 * r2: Id -> x
 * r3: Id -> y
 */
void test_gram() {
  MyTime("test_gram");
  unsigned int forced_len = zop.dbgNumOption("forced_len", 10);
  /*** Initialize the global representation for the above grammar ***/
  {
    MyTime tm_init_G("init_G("+toString(forced_len)+")");
    init_G(forced_len);
    std::cout << "\n-------------- done init --------------\n";
  }
  dbgG::dmp(G);

#if 1
  /*** Here is how to build various parse trees for the above grammar, G  ***/
  /* NOTE: this doesn't perform real parsing, just "hand" tree construction */  
  {
    // Parse tree for: (x) is r0(r2)
    GRule::GParseTree *ptx = mkgptn(r0, 1, mkgptn(r2, 0));
    dbgG::dotTree(ptx);
    std::string flat = ptx->flatten();
    std::cout << "flat:" << flat << std::endl;
    assert(0 == flat.compare("(x)"));
    MPInt r_ptx = S->rank(ptx);
    std::cout << "rank ptx= " << r_ptx << std::endl;

    GRule::GParseTree *parse_ptx = static_cast<GRule::GParseTree*>(pt_from_mem("(x)"));
    std::string parse_flat = parse_ptx->flatten();
    assert(0 == flat.compare(parse_flat));
    
    ParseTree *pt_unrank = S->unrank(r_ptx, flat.length());
    std::string flat_unrank = pt_unrank->flatten();
    std::cout << "flat_unrank=" << flat_unrank << std::endl;
    assert(0 == flat_unrank.compare(flat));
    delete ptx;
    delete parse_ptx;
    delete pt_unrank;
  }
  {
    // Parse tree for: (y) is r0(r3)
    GRule::GParseTree *pty = mkgptn(r0, 1, mkgptn(r3, 0));
    dbgG::dotTree(pty);
    std::string flat = pty->flatten();
    std::cout << "flat:" << flat << std::endl;
    assert(0 == flat.compare("(y)"));
    MPInt r_pty = S->rank(pty);
    std::cout << "rank pty= " << r_pty << std::endl;
    
    GRule::GParseTree *parse_pty = static_cast<GRule::GParseTree*>(pt_from_mem("(y)"));
    std::string parse_flat = parse_pty->flatten();
    //std::cout << "parse_flat = " << parse_flat << std::endl;
    assert(0 == flat.compare(parse_flat));

    ParseTree *pt_unrank = S->unrank(r_pty, flat.length());
    std::string flat_unrank = pt_unrank->flatten();
    std::cout << "flat_unrank=" << flat_unrank << std::endl;
    assert(0 == flat_unrank.compare(flat));
    delete pty;
    delete parse_pty;
    delete pt_unrank;
  }
  {
    // Parse tree for: (x + (x)) is r1(r2r0(r2))
    GRule::GParseTree *ptxx = mkgptn(r1, 2,
			     mkgptn(r2, 0),
			     mkgptn(r0, 1, mkgptn(r2, 0)));
    
    dbgG::dotTree(ptxx);
    std::string flat = ptxx->flatten();
    std::cout << "flat:" << flat << std::endl;
    assert(0 == flat.compare("(x+(x))"));
    MPInt r_ptxx = S->rank(ptxx);
    std::cout << "rank ptxx= " << r_ptxx << std::endl;
    
    GRule::GParseTree *parse_ptxx = static_cast<GRule::GParseTree*>(pt_from_mem("(x+(x))"));
    std::string parse_flat = parse_ptxx->flatten();
    //std::cout << "parse_flat = " << parse_flat << std::endl;
    assert(0 == flat.compare(parse_flat));

    ParseTree *pt_unrank = S->unrank(r_ptxx, flat.length());
    std::string flat_unrank = pt_unrank->flatten();
    std::cout << "flat_unrank=" << flat_unrank << std::endl;
    assert(0 == flat_unrank.compare(flat));
    delete ptxx;
    delete parse_ptxx;
    delete pt_unrank;
  }
  {
    // Parse tree for: (x + (y)) is r1(r2r0(r3))
    GRule::GParseTree *ptxy = mkgptn(r1, 2,
			     mkgptn(r2, 0),
			     mkgptn(r0, 1, mkgptn(r3, 0)));
    
    dbgG::dotTree(ptxy);
    std::string flat = ptxy->flatten();
    std::cout << "flat:" << flat << std::endl;
    assert(0 == flat.compare("(x+(y))"));
    MPInt r_ptxy = S->rank(ptxy);
    std::cout << "rank ptxy= " << r_ptxy << std::endl;
    
    GRule::GParseTree *parse_ptxy = static_cast<GRule::GParseTree*>(pt_from_mem("(x+(y))"));
    std::string parse_flat = parse_ptxy->flatten();
    //std::cout << "parse_flat = " << parse_flat << std::endl;
    assert(0 == flat.compare(parse_flat));

    ParseTree *pt_unrank = S->unrank(r_ptxy, flat.length());
    std::string flat_unrank = pt_unrank->flatten();
    std::cout << "flat_unrank=" << flat_unrank << std::endl;
    assert(0 == flat_unrank.compare(flat));
    delete ptxy;
    delete parse_ptxy;
    delete pt_unrank;
  }
  {
    // Parse tree for: (y + (x)) is r1(r3r0(r2))
    GRule::GParseTree *ptyx = mkgptn(r1, 2,
			     mkgptn(r3, 0),
			     mkgptn(r0, 1, mkgptn(r2, 0)));

    dbgG::dotTree(ptyx);
    std::string flat = ptyx->flatten();
    std::cout << "flat:" << flat << std::endl;
    assert(0 == flat.compare("(y+(x))"));
    MPInt r_ptyx = S->rank(ptyx);
    std::cout << "rank ptyx= " << r_ptyx << std::endl;
    
    GRule::GParseTree *parse_ptyx = static_cast<GRule::GParseTree*>(pt_from_mem("(y+(x))"));
    std::string parse_flat = parse_ptyx->flatten();
    //std::cout << "parse_flat = " << parse_flat << std::endl;
    assert(0 == flat.compare(parse_flat));
    
    ParseTree *pt_unrank = S->unrank(r_ptyx, flat.length());
    std::string flat_unrank = pt_unrank->flatten();
    std::cout << "flat_unrank=" << flat_unrank << std::endl;
    assert(0 == flat_unrank.compare(flat));
    
    delete ptyx;
    delete parse_ptyx;
    delete pt_unrank;
  }
  {
    // Parse tree for: (y + (y)) is r1(r3r0(r3))
    GRule::GParseTree *ptyy = mkgptn(r1, 2,
			     mkgptn(r3, 0),
			     mkgptn(r0, 1, mkgptn(r3, 0)));
    
    dbgG::dotTree(ptyy);
    std::string flat = ptyy->flatten();
    std::cout << "flat:" << flat << std::endl;
    assert(0 == flat.compare("(y+(y))"));
    MPInt r_ptyy = S->rank(ptyy);
    std::cout << "rank ptyy= " << r_ptyy << std::endl;

    GRule::GParseTree *parse_ptyy = static_cast<GRule::GParseTree*>(pt_from_mem("(y+(y))"));
    std::string parse_flat = parse_ptyy->flatten();
    //std::cout << "parse_flat = " << parse_flat << std::endl;
    assert(0 == flat.compare(parse_flat));

    ParseTree *pt_unrank = S->unrank(r_ptyy, flat.length());
    std::string flat_unrank = pt_unrank->flatten();
    std::cout << "flat_unrank=" << flat_unrank << std::endl;
    assert(0 == flat_unrank.compare(flat));
    delete ptyy;
    delete parse_ptyy;
    delete pt_unrank;
  }
#endif
  test_unrank_rank(forced_len, 100);
}

///////////////////////////////////////////////////////////////////////////////
// -zforced_len:20
void get_options(int argc, char *argv[])
{
  int c;
  extern char *optarg;
  
  while ((c = getopt(argc, argv, "a:z:T:")) != -1) {
    switch (c)
      {
      case 'z':
	zop.dbgOptionSet.insert(std::string(optarg));
	break;
      default:
	zop.usage_and_die("unknown option");
	break;
      }
  }
  
  return;
}
void init(int argc, char *argv[]) {
  // Debug: dump the arguments
  std::cout << "Command line: ";
  for (int i = 0; i < argc; ++i) {
    std::cout << argv[i] << " ";
  }
  std::cout << std::endl;
#ifndef SVN_VERSION
#define SVN_VERSION "UNKNOWN"
#endif

#ifndef SVN_CHANGED
#define SVN_CHANGED "UNCHANGED"
#endif

  DMPNL(SVN_VERSION);
  DMPNL(SVN_CHANGED);
  
  zop.dumpOptions();
}

void test_unrank_rank(unsigned int max_len, unsigned int count) {
  const Nonterminal *S = G.S();
  while (S->num_matches(max_len) == 0 && max_len > 0)
    --max_len;
  MPInt n = S->num_matches(max_len);
  if (n == 0)
    return;
  MPInt max_rank = n - 1;
  if (max_rank < count)
    count = max_rank.get_ui();
  if (count == 0)
    count = 1;
  assert(count > 0);
  MPInt step = max_rank/count;
  n = max_rank;
  bool allow_ambiguous = true;
  bool show_ambiguous = true;
  unsigned int ambiguous = 0;

  MyTime tm_test_gram("test_unrank_rank"+toString(max_len));
  for (unsigned int i = 0; i < count; ++i, (n = n - step)) {
    MPInt copy_n = n;
    ParseTree *pt_unrank = S->unrank(copy_n, max_len);
    std::string str = pt_unrank->flatten();
    delete pt_unrank;
    GRule::GParseTree *pt = static_cast<GRule::GParseTree*>(pt_from_mem(str.c_str()));
    //std::string flat = pt->flatten();    assert(flat.compare(str) == 0);
    
    MPInt r = S->rank(pt);
    delete pt;
    bool ambiguous = ( r != n );
    if (allow_ambiguous) {
      if ( ambiguous) {
	ambiguous++;
	if (show_ambiguous)
	  std::cout << std::endl << " " << max_len << " ambiguous: "
		    << str << std::endl;
      }
    }
    else
      assert(!ambiguous);//this fails for ambiguous grammars. Ex /(11*)*/
    bool dbg_internal = true;
    if (dbg_internal) {
      std::cout << "Iteration i= " << i << std::endl
		<< " unrank(copy_n)= " << str << std::endl
		<< " ambiguous= " << ambiguous << std::endl;
    }
  }
  if (ambiguous)
    std::cout << std::endl << " " << max_len
	      << " ambiguous# " << ambiguous << std::endl;
}

int main(int argc, char* argv[]) {
  get_options(argc, argv);
  init(argc, argv);
  test_gram();
}
