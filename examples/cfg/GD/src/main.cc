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
#include "dbg.h"
#include "gram.h"
#include "MyTime.h"
#include <cstdarg>
#include "exG_defs.h"
#include "gram_dbg.h"
#include "zoptions.h"
#include "MemoryInfo.h"


void test_unrank_rank(rank_cfg *pRankCFG, unsigned int max_len, unsigned int count);
void random_test_unrank_rank(rank_cfg *pRankCFG, unsigned int max_len, unsigned int count);
#include "dbg.h"
void memoize(rank_cfg *pRankCFG, unsigned int min_slice,unsigned int max_slice)  {
  MyTime tm("exG:::memoize "+toString(min_slice)+":"+toString(max_slice));
  (const_cast<Nonterminal*>(pRankCFG->G.S()))->memoize(min_slice, max_slice);
}


void test_memoize_slice(rank_cfg *pRankCFG,
			unsigned int slice,
			unsigned int max_slice) {
  assert(slice <= max_slice);  
  std::string slice_info = "slice("+toString(slice)+")";
  MemoryInfo mi;
  // This is not quite accurate.
  // For accurate results use -zpure_memoization, but in this case,
  // the test must be run for each slice.
  mi.read();  mi.dump("memory-before-"+slice_info); std::cout << std::endl;
  {
    static MyTime tm_memoize("memoize", false);
    tm_memoize.resume();
    memoize(pRankCFG, slice, max_slice);
    tm_memoize.stop();
    tm_memoize.print(slice_info);
  }
  mi.read();  mi.dump("memory-after-"+slice_info); std::cout << std::endl;
  std::cout << "NUM_MATCHES-" << slice_info << " = " << pRankCFG->num_matches(slice) << std::endl;
}

void test_gram(rank_cfg *pRankCFG,
	       unsigned int min_slice,
	       unsigned int step_slice,
	       unsigned int max_slice) {
  assert(min_slice <= max_slice);
  assert((max_slice-min_slice) % step_slice == 0);
  MyTime tm_test_gram("test_gram");
  
  for (unsigned int slice = min_slice; slice <= max_slice; slice+=step_slice)
    test_memoize_slice(pRankCFG, slice, max_slice);
  std::cout << "\n-------------- done memoize --------------\n";


  /* For C=100 and C=1000 test ranking/unranking in 2 ways:
     - picking C random elements
     - picking C equidistant elements
  */
  
  for (unsigned int slice = min_slice; slice <= max_slice; slice+=step_slice)
    random_test_unrank_rank(pRankCFG, slice, 100);
  std::cout << "\n-------------- done RND:100 (un)ranking --------------\n";
  for (unsigned int slice = min_slice; slice <= max_slice; slice+=step_slice)
    random_test_unrank_rank(pRankCFG, slice, 1000);
  std::cout << "\n-------------- done RND:1000 (un)ranking --------------\n";

  for (unsigned int slice = min_slice; slice <= max_slice; slice+=step_slice)
    test_unrank_rank(pRankCFG, slice, 100);
  std::cout << "\n-------------- done DET:100 (un)ranking --------------\n";
  for (unsigned int slice = min_slice; slice <= max_slice; slice+=step_slice)
    test_unrank_rank(pRankCFG, slice, 1000);
  std::cout << "\n-------------- done DET:1000 (un)ranking --------------\n";
}

///////////////////////////////////////////////////////////////////////////////
// -zmax_slice:20
void get_options(int argc, char *argv[])
{
  int c;
  extern char *optarg;
  
  while ((c = getopt(argc, argv, "z:")) != -1) {
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
void show_options(int argc, char *argv[]) {
  // Debug: dump the arguments
  std::cout << "Command line: ";
  for (int i = 0; i < argc; ++i) {
    std::cout << argv[i] << " ";
  }
  std::cout << std::endl;
  zop.dumpOptions();
}

#define R_VERBOSE 0
void test_unrank_rank(rank_cfg *pRankCFG, unsigned int max_len, unsigned int count) {
  std::string tag = ":DET:test_unrank_rank("+toString(max_len)+","+toString(count)+")";
  const Nonterminal *S = pRankCFG->G.S();

  //while (S->num_matches(max_len) == 0 && max_len > 0) --max_len;
  if (S->num_matches(max_len) == 0 || max_len == 0)
    return;
  
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

  MyTime tm_test_gram(tag);
  MyTime tm_RRank(":RRank"+tag, false);
  MyTime tm_rank(":rank"+tag, false);
  MyTime tm_RUnrank(":RUnrank"+tag, false);
  MyTime tm_unrank(":unrank"+tag, false);
  MyTime tm_flatten(":flatten"+tag, false);
  MyTime tm_yield(":yield"+tag, false);
  MyTime tm_dbgVisit(":dbgVisit"+tag, false);
  MyTime tm_pt_from_mem(":pt_from_mem"+tag, false);
  for (unsigned int i = 0; i < count; ++i, (n = n - step)) {
#if R_VERBOSE    
    std::cout << "Start Iteration i= " << i << std::endl;
#endif
    //std::cout << "DBG:i=" << i <<"1";
    MPInt copy_n = n;
    tm_RUnrank.resume();tm_unrank.resume();
    ParseTree *pt_unrank = S->unrank(copy_n, max_len);
    tm_unrank.stop();tm_RUnrank.stop();

    //std::cout << "DBG:i=" << i <<"2";
    PTDerivateVisitor dbgv;
    tm_dbgVisit.resume();
    std::string deriv = dbgv.derivation(pt_unrank);
    tm_dbgVisit.stop();
#if R_VERBOSE    
    std::cout << std::endl << "dbg_deriv=" << deriv << std::endl;
#endif
    
    //std::cout << "DBG:i=" << i <<"3";
    tm_flatten.resume();
    std::string flat_str = pt_unrank->flatten();
    tm_flatten.stop();
#if R_VERBOSE    
    std::cout << std::endl << "flat_str=" << flat_str << std::endl;
#endif
    
    //std::cout << "DBG:i=" << i <<"4";
    tm_RUnrank.resume();tm_yield.resume();
    PTYieldVisitor yieldv(" ");
    std::string yield = yieldv.yield(pt_unrank);
    tm_yield.stop();tm_RUnrank.stop();
    delete pt_unrank;
#if R_VERBOSE
    if (yield.compare(flat_str) != 0)
      std::cout << std::endl << "yield=" << yield << std::endl;
#endif
    
    //std::cout << "DBG:i=" << i <<"5";
    tm_RRank.resume(); tm_pt_from_mem.resume();
    /* apparently the following prints some white spaces */
    GRule::GParseTree *pt = static_cast<GRule::GParseTree*>(pRankCFG->pi->pt_from_mem(yield));
    tm_pt_from_mem.stop();tm_RRank.stop();
    //std::string flat = pt->flatten();    assert(flat.compare(str) == 0);
    
    //std::cout << "DBG:i=" << i <<"6";
    tm_RRank.resume();tm_rank.resume();
    MPInt r = S->rank(pt);
    tm_rank.stop();tm_RRank.stop();
    delete pt;
    
    //std::cout << "DBG:i=" << i <<"7";
    bool ambiguous = ( r != n );
    if (allow_ambiguous) {
      if ( ambiguous) {
	ambiguous++;
	MPInt copy_r = r;
	ParseTree *r_pt_unrank = S->unrank(copy_r, max_len);
	std::string r_str = yieldv.yield(r_pt_unrank);
	assert(r_str.compare(yield) == 0);
	delete r_pt_unrank;
#if R_VERBOSE
	if (show_ambiguous)
	  std::cout << std::endl << " " << max_len << " ambiguous: "  << yield << std::endl;
#endif
      }
    }
    else
      assert(!ambiguous);//this fails for ambiguous grammars. Ex /(11*)*/
#if R_VERBOSE
    bool dbg_internal = true;
    if (dbg_internal) {
      std::cout << "Iteration i= " << i << std::endl
		<< " unrank(copy_n)= " << yield << std::endl
		<< " ambiguous= " << ambiguous << std::endl;
    }
#endif
  }
  //if (ambiguous)
  std::cout << std::endl << " " << max_len
	    << " ambiguous# " << ambiguous << std::endl;
}

void random_test_unrank_rank(rank_cfg *pRankCFG, unsigned int max_len, unsigned int count) {
  std::string tag = ":RND:test_unrank_rank("+toString(max_len)+","+toString(count)+")";
  const Nonterminal *S = pRankCFG->G.S();

  //while (S->num_matches(max_len) == 0 && max_len > 0) --max_len;
  if (S->num_matches(max_len) == 0 || max_len == 0)
    return;
  
  MPInt num_matches = S->num_matches(max_len);
  MPInt max_rank = num_matches - 1;
  assert(count > 0);
  bool allow_ambiguous = true;
  bool show_ambiguous = true;
  unsigned int ambiguous = 0;

  MyTime tm_test_gram(tag);
  MyTime tm_RRank(":RRank"+tag, false);
  MyTime tm_rank(":rank"+tag, false);
  MyTime tm_RUnrank(":RUnrank"+tag, false);
  MyTime tm_unrank(":unrank"+tag, false);
  MyTime tm_flatten(":flatten"+tag, false);
  MyTime tm_yield(":yield"+tag, false);
  MyTime tm_dbgVisit(":dbgVisit"+tag, false);
  MyTime tm_pt_from_mem(":pt_from_mem"+tag, false);
  gmp_randclass grc(GMP_RAND_ALG_LC, GMP_RAND_ALG_DEFAULT);
  for (unsigned int i = 0; i < count; ++i) {
#if R_VERBOSE    
    std::cout << "Start Iteration i= " << i << std::endl;
#endif
    //std::cout << "DBG:i=" << i <<"1";
    MPInt n = grc.get_z_range(num_matches);
    
    MPInt copy_n = n;
    tm_RUnrank.resume();tm_unrank.resume();
    ParseTree *pt_unrank = S->unrank(copy_n, max_len);
    tm_unrank.stop();tm_RUnrank.stop();

    //std::cout << "DBG:i=" << i <<"2";
    PTDerivateVisitor dbgv;
    tm_dbgVisit.resume();
    std::string deriv = dbgv.derivation(pt_unrank);
    tm_dbgVisit.stop();
#if R_VERBOSE    
    std::cout << std::endl << "dbg_deriv=" << deriv << std::endl;
#endif
    
    //std::cout << "DBG:i=" << i <<"3";
    tm_flatten.resume();
    std::string flat_str = pt_unrank->flatten();
    tm_flatten.stop();
#if R_VERBOSE    
    std::cout << std::endl << "flat_str=" << flat_str << std::endl;
#endif
    
    //std::cout << "DBG:i=" << i <<"4";
    tm_RUnrank.resume();tm_yield.resume();
    PTYieldVisitor yieldv(" ");
    std::string yield = yieldv.yield(pt_unrank);
    tm_yield.stop();tm_RUnrank.stop();
    delete pt_unrank;
#if R_VERBOSE
    if (yield.compare(flat_str) != 0)
      std::cout << std::endl << "yield=" << yield << std::endl;
#endif
    
    //std::cout << "DBG:i=" << i <<"5";
    tm_RRank.resume(); tm_pt_from_mem.resume();
    /* apparently the following prints some white spaces */
    GRule::GParseTree *pt = static_cast<GRule::GParseTree*>(pRankCFG->pi->pt_from_mem(yield));
    tm_pt_from_mem.stop();tm_RRank.stop();
    //std::string flat = pt->flatten();    assert(flat.compare(str) == 0);
    
    //std::cout << "DBG:i=" << i <<"6";
    tm_RRank.resume();tm_rank.resume();
    MPInt r = S->rank(pt);
    tm_rank.stop();tm_RRank.stop();
    delete pt;
    
    //std::cout << "DBG:i=" << i <<"7";
    bool ambiguous = ( r != n );
    if (allow_ambiguous) {
      if ( ambiguous) {
	ambiguous++;
	MPInt copy_r = r;
	ParseTree *r_pt_unrank = S->unrank(copy_r, max_len);
	std::string r_str = yieldv.yield(r_pt_unrank);
	assert(r_str.compare(yield) == 0);
	delete r_pt_unrank;
#if R_VERBOSE
	if (show_ambiguous)
	  std::cout << std::endl << " " << max_len << " ambiguous: "  << yield << std::endl;
#endif
      }
    }
    else
      assert(!ambiguous);//this fails for ambiguous grammars. Ex /(11*)*/
#if R_VERBOSE
    bool dbg_internal = true;
    if (dbg_internal) {
      std::cout << "Iteration i= " << i << std::endl
		<< " unrank(copy_n)= " << yield << std::endl
		<< " ambiguous= " << ambiguous << std::endl;
    }
#endif
  }
  //if (ambiguous)
    std::cout << std::endl << " " << max_len
	      << " ambiguous# " << ambiguous << std::endl;
}

/*
  Usage:
  (1) bin/exG -zpure_memoization -zmax_slice:MAX
      This will test only the memoization time/space, performed once for slice=MAX
  (2) bin/exG -zmax_slice:MAX
      This will test the rankin/unranking time and ambiguity at 100 bytes
      interval till slice=MAX. The memoization space and time is also reported,
      but the space will be slihgtly not accurate, because the memoization
      arrays are pre-allocaded, and filled in incrementally.
 */
int main(int argc, char *argv[]) {
  MyTime tm("main");
  get_options(argc, argv);
  show_options(argc, argv);
  unsigned int max_slice = zop.dbgNumOption("max_slice", 100);

  bool pure_memoization = zop.dbgBoolOption("pure_memoization");

  MemoryInfo mi;
  mi.read();  mi.dump("memory-before-constructor"); std::cout << std::endl;
  if (pure_memoization) {
    MyTime tm_init_G("constructor-grammar("+toString(max_slice) + " , " + toString(max_slice) + ")");
    g_exG = new exG(max_slice, max_slice);
  } else {
    MyTime tm_init_G("constructor-grammar(0, "+toString(max_slice)+")");
    g_exG = new exG(0, max_slice);
  }
  mi.read();  mi.dump("memory-after-constructor"); std::cout << std::endl;
  std::cout << "\n-------------- done constructor --------------\n";
  
  if (!pure_memoization)
    test_gram(g_exG, 100, 100, max_slice);
  mi.read();  mi.dump("memory-end-main"); std::cout << std::endl;
  delete g_exG;
}
