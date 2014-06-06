/*-----------------------------------------------------------------------------
 * File:    trex_tests.cc
 * Author:  Daniel Luchaup
 * Date:    26 November 2013
 * Copyright 2013 Daniel Luchaup luchaup@cs.wisc.edu
 *
 * This file contains unpublished confidential proprietary work of
 * Daniel Luchaup, Department of Computer Sciences, University of
 * Wisconsin--Madison.  No use of any sort, including execution, modification,
 * copying, storage, distribution, or reverse engineering is permitted without
 * the express written consent (for each kind of use) of Daniel Luchaup.
 *
 *---------------------------------------------------------------------------*/
#include <assert.h>
#include <fstream>
#include "trex.h"
#include "tnfa.h"
#include "ranker.h"
#include "ffa.h"

#include "zoptions.h"
#include "dbg.h"
#include "MemoryInfo.h"

using namespace std;
void read_regular_expressions(vector<string> &regular_expressions) {
  FILE *file = zop.optFILE("rexExprFile");
  char * line = NULL;
  size_t len = 0;
  ssize_t read;
  unsigned int lines_processed = 0;
  regular_expressions.clear();
  
  while ((read = getline(&line, &len, file)) != -1) {
    lines_processed++;
    if (line == NULL || line[0] != '/') {
      printf(" SKIPPING line or regexp:%d:%s\n",lines_processed, line);fflush(0);
      continue;
    }
    printf(" Adding regexp:%d:%s\n",lines_processed, line);fflush(0);
    chomp(line);
    regular_expressions.push_back(line);
  }
  
  fclose(file);
  if (line)
    free(line);
}

void read_string_inputs(vector<string> &string_inputs, const char* ifile) {
  //DMP(ifile);
  char * line = NULL;
  size_t len = 0;
  ssize_t read;
  unsigned int lines_processed = 0;
  FILE *file = zop.optFILE(ifile);
  lines_processed = 0;
  string_inputs.clear();
  while ((read = getline(&line, &len, file)) != -1) {
    lines_processed++;
    if (line == NULL || line[0] == '#') {
      printf("SKIPPING: on line %d; comment:%s\n",lines_processed, line);fflush(0);
      continue;
    }    
    printf(" Adding input:%d:%s\n",lines_processed, line);fflush(0);
    chomp(line);
    string_inputs.push_back(line);
  }
  fclose(file);
  if (line)
    free(line);
}
void read_string_inputs(vector<string> &string_inputs) {
  read_string_inputs(string_inputs, "rexInputFile");
}

void ranker_sanity_tests();

static void ffa_test(vector<string> &regular_expressions) {
  int rex_id = 0;
  for (vector<string>::iterator rit =  regular_expressions.begin();
       rit !=  regular_expressions.end();
       ++rit) {
    ++rex_id;
    const std::string rex = *rit;
    cout << "TESTING: REX=" << rex << endl;
    
    unsigned int forced_len = zop.dbgNumOption("forced_len", 0);
    rank_ffa_tab_t rft(rex, forced_len);
    rank_dfa_tab_t rdt(rex, rft.pdt, forced_len);
    
    assert(rft.get_memoized_size() == rdt.get_memoized_size());
    unsigned int memoized_size = rft.get_memoized_size();
    unsigned int num_states = rft.pdt->num_states;
    state_id_t dbgs = 0;
    for (state_id_t s = 0; s < num_states; ++s) {
      for (size_t l = 0; l < memoized_size; ++l) {
	if (rft.tab[s][l] != rdt.tab[s][l]) {
	  (const_cast <dfa_tab_t*>(rft.pdt))->dfa2nfa2dotty("rft.pdt");
	  DMP(dbgs++);
	  DMP(s);
	  DMP(l);
	  DMP(rft.tab[s][l]);
	  DMPNL(rdt.tab[s][l]);
	  cout.flush();
	}
	assert(rft.tab[s][l] == rdt.tab[s][l]);
      }
    }

    rft.test_unrank_rank(forced_len, 100);
    cout << endl << "FFA-TEST-DONE" << endl;
  }
}

void ffa_tests() {
  MyTime tm("ffa_tests");
  vector<string> regular_expressions;
  read_regular_expressions(regular_expressions);
  vector<string> string_inputs;
  read_string_inputs(string_inputs);
  ffa_test(regular_expressions);
  ranker_sanity_tests();
}

#if 1
void ranker_sanity_tests() {
  MyTime tm("ranker_sanity_tests");
  vector<string> regular_expressions;
  read_regular_expressions(regular_expressions);
  
  int rex_id = 0;
  
  unsigned int repeat = zop.dbgNumOption("repeat", 100);
  MemoryInfo mi; mi.read();  mi.dump("initial-memory");
  
  for (vector<string>::iterator rit =  regular_expressions.begin();
       rit !=  regular_expressions.end();
       ++rit) {
    ++rex_id;
    const std::string rex = *rit;
    cout << "TESTING: REX=" << rex << endl;
    rank_dfa_tab_t rdt(rex);
    rank_nfa_tab_t rnf(rex);
    rank_trx rtrx(rex);
    bool known_ambiguous = rnf.known_ambiguous();
    bool problematic = rtrx.snfa_root->Type()->problematic();
    bool bounded = rtrx.snfa_root->Type()->bounded();
    unsigned int len0 = (bounded)?
      rtrx.snfa_root->Type()->length_max : 4*rex.length();
    unsigned int forced_len = zop.dbgNumOption("forced_len", len0);
    forced_len = std::min(len0, forced_len);
    
    rdt.update_matches(forced_len);
    rnf.update_matches(forced_len);
    if (!problematic)
      rtrx.update_matches(forced_len);
    
    for (unsigned int l = 0; l <= forced_len; ++l) {
      assert(rdt.num_matches(l) <= rnf.num_matches(l));
      if (!problematic)
        assert(rnf.num_matches(l) <= rtrx.num_matches(l));
    }
    
    if (!known_ambiguous) {// assume all reg.exps properly ambiguous labeled
      for (unsigned int l = 0; l <= forced_len; ++l) {
        assert(rdt.num_matches(l) == rnf.num_matches(l));
        assert(rdt.num_matches(l) == rtrx.num_matches(l));
      }
    }
///////////////////////////////////////////////////////////////////////////////
    unsigned int count = repeat;
    unsigned int max_len = rdt.max_achievable_len(); // may be smaller than original max_len
    MPInt n = rdt.num_matches(max_len);
    if (n != 0) {
      MPInt max_rank = n - 1;
      if (max_rank < count)
        count = max_rank.get_ui();
      if (count == 0)
        count = 1;
      assert(count > 0);
      MPInt step = max_rank/count;
      n = max_rank;
      for (unsigned int i = 0; i < count; ++i, (n = n - step)) {
        string str;
	MPInt copy_n = n;
	rdt.unrank(copy_n, max_len, str);

	assert(rdt.matches(str));
	assert(rnf.matches(str));
	//no need for: assert(rtrx.matches(str)); follows from rnf.matches(str)

	// dfa
	MPInt r_rdt = 0;
	bool ok_rdt = rdt.rank(r_rdt, str);
	assert(ok_rdt);
	assert(r_rdt == n);

	//nfa
	MPInt r_rnf = 0;
	bool ok_rnf = rnf.rank(r_rnf, str);
	assert(ok_rnf);
	string str_nfa;
	copy_n = r_rnf;
	rnf.unrank(r_rnf, max_len, str_nfa);
	assert(str == str_nfa);

	//trx
	if (!problematic) {
	  MPInt r_rtrx = 0;
	  bool ok_rtrx = rtrx.rank(r_rtrx, str);
	  assert(ok_rtrx);
	  string str_rtrx;
	  rtrx.unrank(r_rtrx, max_len, str_rtrx);
	  assert(str == str_rtrx);
	}
      }
    }
///////////////////////////////////////////////////////////////////////////////
  }
}
#endif

void rank_tests() {
  MyTime tm("rank_tests");
  vector<string> regular_expressions;
  read_regular_expressions(regular_expressions);

  int rex_id = 0;

  bool do_dfa = zop.dbgBoolOption("do_dfa");
  bool do_ffa = zop.dbgBoolOption("do_ffa");
  bool do_nfa = zop.dbgBoolOption("do_nfa");
  bool do_trx = zop.dbgBoolOption("do_trx");

  unsigned int repeat = zop.dbgNumOption("repeat", 100);
  MemoryInfo mi; mi.read();  mi.dump("initial-memory");
  
  for (vector<string>::iterator rit =  regular_expressions.begin();
       rit !=  regular_expressions.end();
       ++rit) {
    ++rex_id;
    const std::string rex = *rit;
    cout << "TESTING: REX=" << rex << endl;
    
    if (unsigned int forced_len = zop.dbgNumOption("forced_len", 0)) {
      if (do_dfa) {
	rank_dfa_tab_t rdt(rex, forced_len);
	rdt.ranker_report_size("constructed-udated-rdt", 1);
	rdt.test_unrank_rank(forced_len, repeat);
	rdt.ranker_report_size("used-rdt", 1);
      }
      if (do_nfa) {
	rank_nfa_tab_t rnf(rex, forced_len);
	if (0) {
		std::ofstream dotsNFA("dot.trex_range.2");
		rnf.snfa_root->to_dot(dotsNFA); dotsNFA.close();
	}
        rnf.ranker_report_size("constructed-udated-rnf", 1);
	rnf.test_unrank_rank(forced_len, repeat);
	rnf.ranker_report_size("used-rnf", 1);
      }
      if (do_trx) {
	rank_trx rtrx(rex, forced_len);
	rtrx.ranker_report_size("constructed-udated-rtrx", 1);
	rtrx.test_unrank_rank(forced_len, repeat);
	rtrx.ranker_report_size("used-rtrx", 1);
      }
      if (do_ffa) {
	rank_ffa_tab_t rft(rex, forced_len);
	rft.ranker_report_size("constructed-udated-rft", 1);
	//DMP(rft.num_matches(forced_len));
	rft.test_unrank_rank(forced_len, repeat);
	rft.ranker_report_size("used-rft", 1);
      }
    }
  }
}
void rank_tests0() {
  //MyTime tm("rank_tests");
  vector<string> regular_expressions;
  read_regular_expressions(regular_expressions);

  int rex_id = 0;

  bool do_dfa = zop.dbgBoolOption("do_dfa");
  bool do_ffa = zop.dbgBoolOption("do_ffa");
  bool do_nfa = zop.dbgBoolOption("do_nfa");
  bool do_trx = zop.dbgBoolOption("do_trx");

  unsigned int repeat = zop.dbgNumOption("repeat", 100);
  //MemoryInfo mi; mi.read();  mi.dump("initial-memory");
  
  for (vector<string>::iterator rit =  regular_expressions.begin();
       rit !=  regular_expressions.end();
       ++rit) {
    ++rex_id;
    const std::string rex = *rit;
    cout << "TESTING: REX=" << rex << endl;
    
    if (unsigned int forced_len = zop.dbgNumOption("forced_len", 0)) {
      if (do_dfa) {
	rank_dfa_tab_t rdt(rex, forced_len);
	//rdt.ranker_report_size("constructed-udated-rdt", 1);
	rdt.test_unrank_rank(forced_len, repeat);
	//rdt.ranker_report_size("used-rdt", 1);
      }
      if (do_nfa) {
	rank_nfa_tab_t rnf(rex, forced_len);
	//rnf.ranker_report_size("constructed-udated-rnf", 1);
	rnf.test_unrank_rank(forced_len, repeat);
	//rnf.ranker_report_size("used-rnf", 1);
      }
      if (do_trx) {
	rank_trx rtrx(rex, forced_len);
	//rtrx.ranker_report_size("constructed-udated-rtrx", 1);
	rtrx.test_unrank_rank(forced_len, repeat);
	//rtrx.ranker_report_size("used-rtrx", 1);
      }
      if (do_ffa) {
	rank_ffa_tab_t rft(rex, forced_len);
	//rft.ranker_report_size("constructed-udated-rft", 1);
	rft.test_unrank_rank(forced_len, repeat);
	//rft.ranker_report_size("used-rft", 1);
      }
    }
  }
}

void rex_classify() {
  MyTime tm("rex_classify");
  vector<string> regular_expressions;
  read_regular_expressions(regular_expressions);
  
  int rex_id = 0;
  
  bool check_ambiguity = zop.dbgBoolOption("check_ambiguity");
  unsigned int forced_len = zop.dbgNumOption("forced_len", 0);
  
  int verbosityC = zop.dbgNumOption("verbosityC", 0);//use 2 for bit counting
  
  for (vector<string>::iterator rit =  regular_expressions.begin();
       rit !=  regular_expressions.end();
       ++rit) {
    ++rex_id;
    const std::string rex = *rit;
    cout << "TESTING: REX=" << rex << endl;
    unsigned int len = check_ambiguity?
      (forced_len ? forced_len : 4*rex.length()) : 0;
    
    rank_trx rtrx(rex, len); rtrx.ranker_report_size("rtrx", verbosityC);
    if (rtrx.local_trx->problematic())
      cout << "CLASSIFY:PROBLEMATIC REX:" << rex << endl;
    else {
      if (!check_ambiguity) {
	cout << "CLASSIFY:NON-PROBLEMATIC REX:" << rex << endl;
      } else {
        rank_dfa_tab_t rdt(rex, len); rdt.ranker_report_size("rdt", verbosityC);
	bool is_ambiguous = false;
	MPInt ambig_ratio1 = 0;
	unsigned int ambig_len1 = 0;
	for (unsigned int l = 0; l <= len; ++l) {
	  if (rtrx.num_matches(l) != rdt.num_matches(l)) {
	    is_ambiguous = true;
	    ambig_ratio1 = rtrx.num_matches(l)/rdt.num_matches(l);
	    ambig_len1 = l;
	    break;
	  }
	}
	if (is_ambiguous) {
  	  rank_nfa_tab_t rnf(rex, len);rnf.ranker_report_size("rnf", verbosityC);
	  bool is_ambiguous2 = false;
	  MPInt ambig_ratio2 = 0;
	  unsigned int ambig_len2 = 0;
	  bool rnf_same_as_trx = true;
	  for (unsigned int l = 0; l <= len; ++l) {
   	    if (rnf.num_matches(l) != rtrx.num_matches(l)) {
	      rnf_same_as_trx = false;
	      break;
	    }
	  }
	  for (unsigned int l = 0; l <= len; ++l) {
   	    if (rnf.num_matches(l) != rdt.num_matches(l)) {
	      is_ambiguous2 = true;
	      ambig_ratio2 = rnf.num_matches(l)/rdt.num_matches(l);
	      ambig_len2 = l;
	      break;
	    }
	  }
	  if (is_ambiguous2) {
	    cout << "CLASSIFY:AMBIGUOUS2-trx-nfa REX:" << rex << endl;
	    DMP(ambig_ratio2) <<  " " << MPFloat(rnf.num_matches(ambig_len2))/MPFloat(rdt.num_matches(ambig_len2)) << endl;
	    DMPNL(ambig_len2);
	    DMPNL(rnf.capacity(len));
	  }
	  else
	    cout << "CLASSIFY:AMBIGUOUS1-trx REX:" << rex << endl;
	  DMP(ambig_ratio1)  <<  " " << MPFloat(rtrx.num_matches(ambig_len1))/MPFloat(rdt.num_matches(ambig_len1)) << endl;
	  DMPNL(ambig_len1);
	  if (rdt.num_matches(len)!=0 && rnf.num_matches(len) != rdt.num_matches(len) ) {
	     MPInt ambig_ratio_nfa_max = rnf.num_matches(len)/rdt.num_matches(len);
	     DMPNL(len);
	     DMPNL(rnf.num_matches(len));
	     DMPNL(rdt.num_matches(len));
	     DMPNL(rtrx.num_matches(len));
	     DMP(ambig_ratio_nfa_max) <<  " " << MPFloat(rnf.num_matches(len))/MPFloat(rdt.num_matches(len)) << endl;
	     MPInt ambig_ratio_trx_max = rtrx.num_matches(len)/rdt.num_matches(len);
	     DMP(ambig_ratio_trx_max) <<  " " << MPFloat(rtrx.num_matches(len))/MPFloat(rdt.num_matches(len)) << endl;
	  } else
	    cout << "WEIRD:ambiguous-but-no-ambiguity-at-len:" << rex << endl;
	  DMPNL(rnf_same_as_trx);
	}
	else {
	  cout << "CLASSIFY:OK REX:" << rex << endl;
	}
	DMPNL(rdt.capacity(len));
      }
    }
    DMPNL(rtrx.capacity(len));
  }
}

void rex_nfa_classify() {
  MyTime tm("rex_nfa_classify");
  vector<string> regular_expressions;
  read_regular_expressions(regular_expressions);
  
  int rex_id = 0;
  
  unsigned int forced_len = zop.dbgNumOption("forced_len", 0);
  
  int verbosityC = zop.dbgNumOption("verbosityC", 0);//use 2 for bit counting
  
  for (vector<string>::iterator rit =  regular_expressions.begin();
       rit !=  regular_expressions.end();
       ++rit) {
    ++rex_id;
    const std::string rex = *rit;
    cout << "TESTING: REX=" << rex << endl;
    unsigned int len = forced_len;

    //rank_nfa_tab_t rnf(rex, len);rnf.ranker_report_size("rnf", verbosityC);
    rank_nfa_tab_t rnf(rex);
    //////////////////////////////////////////////////////////////////////
    cout << endl
	 << "rex-stats: bounded= " << rnf.snfa_root->Type()->bounded()
	 << " length_max= " << (int)(rnf.snfa_root->Type()->length_max)
	 << endl;
    //////////////////////////////////////////////////////////////////////
    rnf.update_matches(len);rnf.ranker_report_size("rnf", verbosityC);
    rank_dfa_tab_t rdt(rex, len); rdt.ranker_report_size("rdt", verbosityC);
    bool is_ambiguous_rnf = false;
    MPInt ambig_ratio_low = 0;
    unsigned int ambig_len_low = 0;
    
    for (unsigned int l = 0; l <= len; ++l) {
      if (rnf.num_matches(l) != rdt.num_matches(l)) {
	is_ambiguous_rnf = true;
	ambig_ratio_low = rnf.num_matches(l)/rdt.num_matches(l);
	ambig_len_low = l;
	break;
      }
    }
    if (is_ambiguous_rnf) {
      cout << "CLASSIFY:AMBIGUOUS-nfa REX:" << rex << endl;
      DMP(ambig_ratio_low) <<  " " << MPFloat(rnf.num_matches(ambig_len_low))/MPFloat(rdt.num_matches(ambig_len_low)) << endl;
      DMPNL(ambig_len_low);
      DMPNL(rnf.capacity(ambig_len_low));
      
      if (rdt.num_matches(len)!=0 && rnf.num_matches(len) != rdt.num_matches(len) ) {
	MPInt ambig_ratio_nfa_max = rnf.num_matches(len)/rdt.num_matches(len);
	DMPNL(len);
	DMPNL(rnf.num_matches(len));
	DMPNL(rdt.num_matches(len));
	DMP(ambig_ratio_nfa_max) <<  " " << MPFloat(rnf.num_matches(len))/MPFloat(rdt.num_matches(len)) << endl;
      } else
	cout << "WEIRD:ambiguous-but-no-ambiguity-at-len:" << rex << endl;
    }
    else
      cout << "CLASSIFY:OK-rnf REX:" << rex << endl;
    
    DMPNL(rdt.capacity(len));
    DMPNL(rnf.capacity(len));
  }
}

void rex_stats() {
  MyTime tm("rex_stats");
  vector<string> regular_expressions;
  read_regular_expressions(regular_expressions);

  int rex_id = 0;

  bool do_dfa = zop.dbgBoolOption("do_dfa");
  bool do_nfa = zop.dbgBoolOption("do_nfa");
  bool do_trx = zop.dbgBoolOption("do_trx");

  int verbosityC = zop.dbgNumOption("verbosityC", 2);
  int verbosityU = zop.dbgNumOption("verbosityU", 1);
  
  for (vector<string>::iterator rit =  regular_expressions.begin();
       rit !=  regular_expressions.end();
       ++rit) {
    ++rex_id;
    const std::string rex = *rit;
    cout << "TESTING: REX=" << rex << endl;
    
    MemoryInfo mi; mi.read();  mi.dump("initial-memory");
    if (unsigned int forced_len = zop.dbgNumOption("forced_len", 0)) {
      if (do_dfa) {
	rank_dfa_tab_t rdt(rex);
	rdt.ranker_report_size("constructed-rdt", verbosityC);
	rdt.update_matches(forced_len);
	rdt.ranker_report_size("updated-rdt", verbosityU);
      } else if (do_nfa) {
	rank_nfa_tab_t rnf(rex);
	rnf.ranker_report_size("constructed-rnf", verbosityC);
	rnf.update_matches(forced_len);
	rnf.ranker_report_size("updated-rnf", verbosityU);
      } else if (do_trx) {
	rank_trx rtrx(rex);
	rtrx.ranker_report_size("constructed-rtrx", verbosityC);
	rtrx.update_matches(forced_len);
	rtrx.ranker_report_size("updated-rtrx", verbosityU);
      } 
    } else {
        rank_trx rtrx(rex);
	cout << "bounded= " << rtrx.snfa_root->Type()->bounded()
	     << " length_max= " << (int)(rtrx.snfa_root->Type()->length_max)
	     << endl;
      }
  }
}

static void rex_test(vector<string> &regular_expressions,
		     vector<string> &) {
  int rex_id = 0;
  for (vector<string>::iterator rit =  regular_expressions.begin();
       rit !=  regular_expressions.end();
       ++rit) {
    if (stateNFA::dbg_reset_stateNFA_guid) stateNFA::g_uido = 0;
    ++rex_id;
    const std::string rex = *rit;
    cout << "TESTING: REX=" << rex << endl;
    std::string suffix = toString(rex_id) + ".txt";
    //////////////////
    trex *trx = regex2trex(rex, NULL, NULL);
    std::ofstream dotout (("dot.trx."+suffix).c_str());  trx->to_dot(dotout); dotout.close();
    //////////////
    stateNFA *snfa = trx->toNFA(NULL);
    assert(snfa->is_root());
    ofstream dotsNFA(("dot.NFA-trx."+suffix+".txt").c_str());
    snfa->to_dot(dotsNFA); dotsNFA.close();
    
    if (trx->problematic())
      cout << "PROBLEMATIC REX:" << rex << endl;
    //////////////
    dfa_tab_t * pdt = snfa->make_dfa();
    
    stateNFA::delete_all_reachable(snfa);
    delete trx;
    delete pdt;
  }
}

static void unit_test_rank(ranker& rkr, vector<string> &string_inputs) {
  for (vector<string>::iterator iit = string_inputs.begin();
       iit !=  string_inputs.end(); ++iit) {
    const std::string input = *iit;
    size_t slice_len = input.length();
    MPInt r = 0;
    bool ok = rkr.rankerinterface::rank(r, input);
    if (ok) {
      string str;
      MPInt copy_r = r;
      rkr.unrank(copy_r, slice_len, str);
      assert(str.compare(input) == 0);
      cout << "Rank of: '" << input << "' is:" << endl
	   << r << endl;
    } else {
      cout << "RANK FAILED for:'" << input << "'" << endl;
    }
  }
}

void unit_test() {
  extern bool dbg_internal;
  dbg_internal = true;
  MyTime tm("unit_test");
  vector<string> regular_expressions;
  read_regular_expressions(regular_expressions);
  vector<string> string_inputs;
  if (zop.dbgDefinedOption("rexInputFile"))
    read_string_inputs(string_inputs);

  unsigned int forced_len = zop.dbgNumOption("forced_len", 0);
  
  for (vector<string>::iterator iit = string_inputs.begin();
       iit !=  string_inputs.end(); ++iit) {
    const std::string input = *iit;
    if (input.length() > forced_len)
      forced_len = input.length();
  }
  
  if (forced_len == 0)
    return;
  
  int rex_id = 0;
  stateNFA::dbg_reset_stateNFA_guid = true;
  
  bool do_dfa = zop.dbgBoolOption("do_dfa");
  bool do_ffa = zop.dbgBoolOption("do_ffa");
  bool do_nfa = zop.dbgBoolOption("do_nfa");
  bool do_trx = zop.dbgBoolOption("do_trx");
  
  unsigned int repeat = zop.dbgNumOption("repeat", 100);
  MemoryInfo mi; mi.read();  mi.dump("initial-memory"); cout << endl;
  
  for (vector<string>::iterator rit =  regular_expressions.begin();
       rit !=  regular_expressions.end();
       ++rit) {
    ++rex_id;
    const std::string rex = *rit;
    cout << "TESTING: REX=" << rex << endl;
    if (do_dfa) {
      rank_dfa_tab_t rdt(rex, forced_len);
      rdt.ranker_report_size("constructed-udated-rdt", 1);
      rdt.test_unrank_rank(forced_len, repeat);
      rdt.ranker_report_size("used-rdt", 1);
      unit_test_rank(rdt, string_inputs);
    }
    if (do_nfa) {
      rank_nfa_tab_t rnf(rex, forced_len);
      rnf.ranker_report_size("constructed-udated-rnf", 1);
      rnf.test_unrank_rank(forced_len, repeat);
      rnf.ranker_report_size("used-rnf", 1);
      unit_test_rank(rnf, string_inputs);
    }
    if (do_trx) {
      rank_trx rtrx(rex, forced_len);
      rtrx.ranker_report_size("constructed-udated-rtrx", 1);
      rtrx.test_unrank_rank(forced_len, repeat);
      rtrx.ranker_report_size("used-rtrx", 1);
      unit_test_rank(rtrx, string_inputs);
    }
    if (do_ffa) {
      rank_ffa_tab_t rft(rex, forced_len);
      rft.ranker_report_size("constructed-udated-rft", 1);
      rft.test_unrank_rank(forced_len, repeat);
      rft.ranker_report_size("used-rft", 1);
      unit_test_rank(rft, string_inputs);
    } 
  }
}

void match_tests(ranker& rkr,
		 const vector<string>& string_matches,
		 const vector<string>& string_no_matches) {
  int errors = 0;
  for (vector<string>::const_iterator it =  string_matches.begin();
       it !=  string_matches.end();
       ++it) {
	  string mstr = *it;
	  //  assert(rkr.matches(str));
	  if (!rkr.matches(mstr)) {
		  errors++;
		  DMPNL(mstr);
	  }
  }
  for (vector<string>::const_iterator it =  string_no_matches.begin();
       it !=  string_no_matches.end();
       ++it) {
	  string nstr = *it;
	  if (rkr.matches(nstr)) {
		  errors++;
		  DMPNL(nstr);
	  }
  }
  DMPNL(errors);
}
void match_test() {
  extern bool dbg_internal;
  dbg_internal = true;
  MyTime tm("match_test");
  vector<string> regular_expressions;
  read_regular_expressions(regular_expressions);
  vector<string> string_matches;
  read_string_inputs(string_matches, "GOOD");
  vector<string> string_no_matches;
  read_string_inputs(string_no_matches, "BAD");
  
  int rex_id = 0;
  stateNFA::dbg_reset_stateNFA_guid = true;
  
  bool do_dfa = zop.dbgBoolOption("do_dfa");
  bool do_ffa = zop.dbgBoolOption("do_ffa");
  bool do_nfa = zop.dbgBoolOption("do_nfa");
  bool do_trx = zop.dbgBoolOption("do_trx");
  
  for (vector<string>::iterator rit =  regular_expressions.begin();
       rit !=  regular_expressions.end();
       ++rit) {
    ++rex_id;
    const std::string rex = *rit;
    cout << "TESTING: REX=" << rex << endl;
    if (do_dfa) {
      rank_dfa_tab_t rdt(rex);
      match_tests(rdt, string_matches, string_no_matches);
    }
    if (do_nfa) {
      rank_nfa_tab_t rnf(rex);
      if (0) {
	      std::ofstream dotsNFA("dot.trex_range.2");
	      rnf.snfa_root->to_dot(dotsNFA); dotsNFA.close();
	      rnf.update_matches(1);
	      rnf.ranker_report_size("constructed-rnf", 1);
      }
      match_tests(rnf, string_matches, string_no_matches);
    }
    if (do_trx) {
      rank_trx rtrx(rex);
      match_tests(rtrx, string_matches, string_no_matches);
    }
    if (do_ffa) {
      rank_ffa_tab_t rft(rex);
      match_tests(rft, string_matches, string_no_matches);
    } 
  }
}

void trex_tests() {
  MyTime tm("run_tests");
  vector<string> regular_expressions;
  read_regular_expressions(regular_expressions);
  vector<string> string_inputs;
  read_string_inputs(string_inputs);
  stateNFA::dbg_reset_stateNFA_guid = true;
  //extern void old_rex_test(vector<string> &,vector<string> &);
  //old_rex_test(regular_expressions,string_inputs);
  rex_test(regular_expressions,string_inputs);
}
