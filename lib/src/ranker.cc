/*-----------------------------------------------------------------------------
 * File:    ranker.cc
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
#include <string>
#include "zoptions.h"
#include "ranker.h"
#include "escape_sequences.h"
#include "dbg.h"
#include "MemoryInfo.h"

using namespace std;
/************* rankerinterface ******************/
unsigned int rankerinterface::count_ambiguous(unsigned int len,
					      unsigned int trials) {
  DbgTime tm("rankerinterface::count_ambiguous"+toString(len)+"."+toString(trials));
  update_matches(len);
  MPInt n = num_matches(len);
  if (n == 0)
    return 0;
  MPInt max_rank = n - 1;
  if (max_rank < trials)
    trials = max_rank.get_ui();
  if (trials == 0)
    trials = 1;
  assert(trials > 0);
  MPInt step = max_rank/trials;
  n = max_rank;
  unsigned int num_ambiguous = 0;
  for (unsigned int i = 0; i < trials; ++i, (n = n - step)) {
    string str;
    MPInt copy_n = n;
    unrank(copy_n, len, str);
    MPInt r = 0;
    bool ok = this->rankerinterface::rank(r, str);
    assert(ok);
    bool ambiguous = ( r != n );
    if ( ambiguous)
      ++num_ambiguous;
  }
  
  return num_ambiguous;
}

/************* ranker ***************************/
bool dbg_internal = false;
void ranker::test_unrank_rank(unsigned int max_len, unsigned int count) {
  MyTime tm("ranker::test_unrank_rank."+toString(max_len)+"."+toString(count));
  update_matches(max_len);
  max_len = max_achievable_len(); // may be smaller than original max_len
  MPInt n = num_matches(max_len);
  tm.lap("max_len.set-to."+toString(max_len));
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
  bool allow_ambiguous = zop.dbgBoolOption("allow_ambiguous");
  bool show_ambiguous = zop.dbgBoolOption("show_ambiguous");
  unsigned int ambiguous = 0;
  if (dbg_internal) {
    DMPNL(max_len);
    DMPNL(count);
    DMPNL(max_rank);
    DMPNL(step);
  }

  MyTime tm_rank("rank::test_unrank_rank("+toString(max_len)+","+toString(count)+")", false);
  MyTime tm_unrank("unrank::test_unrank_rank("+toString(max_len)+","+toString(count)+")", false);
  for (unsigned int i = 0; i < count; ++i, (n = n - step)) {
    string str;
    MPInt copy_n = n;
    tm_unrank.resume();
    unrank(copy_n, max_len, str);
    tm_unrank.stop();
    MPInt r = 0;
    tm_rank.resume();
    bool ok = this->rankerinterface::rank(r, str);
    tm_rank.stop();
    assert(ok);
    bool is_ambiguous = ( r != n );
    if (allow_ambiguous) {
      if ( is_ambiguous) {
#if 0
        string str2;
	MPInt copy_r = r;
	unrank(copy_r, max_len, str2);
	assert(str == str2);
#else
	ambiguous++;
	if (show_ambiguous)
	  cout << endl << rex << " " << max_len << " ambiguous: "
	       << output_string2string(str)
	       << endl;
#endif
      }
    }
    else
      assert(!is_ambiguous);//this fails for ambiguous grammars. Ex /(11*)*/
    if (dbg_internal) {
      cout << "Iteration i= " << i << endl
	   << " unrank(copy_n)= " << str << endl
	   << " ambiguous= " << is_ambiguous << endl;
    }
  }
  /*
  if (ambiguous)
    cout << endl << rex << " " << max_len
	 << " ambiguous# " << ambiguous << endl;
  */
}

void ranker::ranker_report_size(const char* msg, int verbose_level) {
  DbgTime tm(std::string("ranker_report_size:")+msg);
  std::cout << "RANKER:SIZE:" << msg << ": Max.Len.Memoized= " << _memoized_size-1 << std::endl;
  std::cout << "RANKER:SIZE:" << msg << ": max_achievable_len= " << max_achievable_len() << std::endl;
  std::cout << "RANKER:SIZE:" << msg << ": capacity(Max.Len.Memoized)= " << capacity(_memoized_size-1) << std::endl;
  std::cout << "RANKER:SIZE:" << msg << ": capacity(max_achievable_len)= " << capacity(max_achievable_len()) << std::endl;
  MemoryInfo mi; mi.read();
  mi.dump(std::string("RANKER:SIZE:")+msg);
  if (verbose_level > 0)
    report_size("RANKER-SIZE:", verbose_level);
    std::cout << std::endl;
}

/************** rank_dfa_tab_t ******************/
void rank_dfa_tab_t::fast_tab() {
  DbgTime tm("rank_dfa_tab_t::fast_tab");
  unsigned int num_states = pdt->num_states;
  vector<set<state_id_t> > preds;
  set<state_id_t> front_0;
  set<state_id_t> front_1;
  
  ftab.clear(); ftab.resize(num_states);
  preds.clear(); preds.resize(num_states);
  front_0.clear(); front_1.clear();
  
  for (state_id_t s = 0; s < num_states; ++s) {
    ftab[s].resize(_memoized_size, 0);
    if (pdt->acc[s]) {
      ftab[s][0] = 1;
      front_0.insert(s);
    }
    
    for (unsigned int c = 0; c < MAX_SYMS; c++) {
      state_id_t next = ARR((pdt->tab), s, c);
      preds[next].insert(s);
    }
  }
  
  
  set<state_id_t> * p_current = &front_0;
  set<state_id_t> * p_future = &front_1;
  
  
  for (size_t i = 1; i < _memoized_size; ++i) {
    get_preds(*p_future, *p_current, preds);
    p_current->clear();
    swap(p_current, p_future);
    
    for (set<state_id_t>::iterator it = p_current->begin();
	 it != p_current->end();
	 ++it) {
	   state_id_t s = *it;
	   for (unsigned int c = 0; c < MAX_SYMS; c++) {
	     state_id_t next = ARR((pdt->tab), s, c);
	     ftab[s][i] = ftab[s][i] + ftab[next][i-1];
	   }
    }
  }
}

bool rank_dfa_tab_t::check_tabs() {
  unsigned int check_tab_errors = 0;
  DbgTime tm("rank_dfa_tab_t::check_tabs");
  assert(ftab.size() == tab.size());
  unsigned int num_states = pdt->num_states;
  for (state_id_t s = 0; s < num_states; ++s) {
    assert(ftab[s].size() == _memoized_size);
    assert(tab[s].size() == _memoized_size);
    for (size_t l = 0; l < _memoized_size; ++l) 
      if (ftab[s][l] != tab[s][l]) {
	++check_tab_errors;
	DMP(s);DMP(l); DMP(ftab[s][l]);   DMP(tab[s][l]);   DMPNL(check_tab_errors);
      }
  }
  if (check_tab_errors != 0) {
    DMP(num_states); DMPNL(_memoized_size);
    for (state_id_t s = 0; s < num_states; ++s) {
      for (size_t l = 0; l < _memoized_size; ++l) {
	cout << "Tables: [" << s << "][" << l << "] "; DMP(ftab[s][l]);   DMPNL(tab[s][l]);
      }
      DMPNL(pdt->acc[s]);
    }
  }
  return check_tab_errors == 0;
}

void rank_dfa_tab_t::dotty(const char *dot_name) {
  (const_cast<dfa_tab_t*>(pdt))->dfa2nfa2dotty(dot_name);
}

void rank_dfa_tab_t::dump_tab() {
  unsigned int num_states = pdt->num_states;
  DMP(num_states); DMPNL(_memoized_size);
  for (state_id_t s = 0; s < num_states; ++s) {
    cout << " state(" << s << ":acc?" << (pdt->acc[s] != NULL) << "):" ;
    for (size_t l = 0; l < _memoized_size; ++l) {
       cout << " [" << l << "]=" << tab[s][l];
    }
    cout << endl;
  }
}

void rank_dfa_tab_t::init() {
  DbgTime tm("rank_dfa_tab_t::init");

  DbgTime tm_parse("rank_dfa_tab_t:parse", false);
  DbgTime tm_to_trex("rank_dfa_tab_t:to_trex", false);
  trex *trx = regex2trex(rex, &tm_parse, &tm_to_trex);
  
  DbgTime tm_toNFA("rank_dfa_tab_t:toNFA");
  stateNFA *snfa = trx->toNFA(NULL);
  tm_toNFA.stop();
  
  assert(snfa->is_root());
  
  DbgTime tm_make_dfa("rank_dfa_tab_t:make_dfa");
  local_pdt = pdt = snfa->make_dfa();
  tm_make_dfa.stop();

  tab.clear();  tab.resize(pdt->num_states);

  DbgTime tm_delete("rank_dfa_tab_t:delete");
  stateNFA::delete_all_reachable(snfa);
  delete trx;
  tm_delete.stop();
}

rank_dfa_tab_t::rank_dfa_tab_t(const string & rex0, size_t max_len)
  : ranker(rex0) {
  DbgTime tm("rank_dfa_tab_t::rank_dfa_tab_t");
  init();
  update_matches(max_len);
}

rank_dfa_tab_t::rank_dfa_tab_t(const string& rex0,
			       const dfa_tab_t *pdt0,
			       size_t max_len)
  : ranker(rex0), pdt(pdt0), local_pdt(NULL) {
  DbgTime tm("rank_dfa_tab_t::rank_dfa_tab_t");
  tab.clear();  tab.resize(pdt->num_states);
  
  update_matches(max_len);
}

rank_dfa_tab_t::rank_dfa_tab_t(const string & rex0)
  : ranker(rex0) {
  DbgTime tm("rank_dfa_tab_t::rank_dfa_tab_t");
  init();
}

void rank_dfa_tab_t::update_matches(unsigned int len) {
  static DbgTime tm_update_matches("STATIC:rank_dfa_tab_t:update_matches", false);

  DbgTime tm("rank_dfa_tab_t::update_matches."+toString(len));
  unsigned int num_states = pdt->num_states;
  if (len < _memoized_size)
    return;
  tm_update_matches.resume();
  assert(tab.size() == num_states);
  for (state_id_t s = 0; s < num_states; ++s) {
    assert(tab[s].size() == _memoized_size);
    tab[s].resize(len+1, 0);
    if (_memoized_size == 0) {
      if (pdt->acc[s])
	tab[s][0] = 1;
    }
  }
  unsigned int low = (_memoized_size == 0)? 1: _memoized_size;
  _memoized_size = len+1;
  
  for (size_t i = low; i < _memoized_size; ++i) {
    for (state_id_t s = 0; s < num_states; s++) {
      for (unsigned int c = 0; c < MAX_SYMS; c++) {
	state_id_t next = ARR((pdt->tab), s, c);
	tab[s][i] = tab[s][i] + tab[next][i-1];
      }
    }
  }
  tm_update_matches.stop();
}

bool rank_dfa_tab_t::rank(MPInt & pc, const char *str, size_t len) {
  static DbgTime tm_rank("STATIC:rank_dfa_tab_t::rank", false);
  tm_rank.resume();
  
  assert(len < _memoized_size);
  //MyTime tm("rank_dfa_tab_t::rank");
  pc = 0;
  state_id_t cur = pdt->start;
  for (size_t i = 0; i < len; ++i) {
    unsigned char c = str[i];
    for (unsigned char b = 0; b < c; ++b) {
      state_id_t next = ARR((pdt->tab), cur, b);
      pc = pc + tab[next][len - 1 -i];
    }
    cur = ARR((pdt->tab), cur, c);
  }
  tm_rank.stop();
  return pdt->acc[cur]!=NULL;
}

bool rank_dfa_tab_t::unrank(MPInt& pc, size_t len, string &res) {
  static DbgTime tm_unrank("STATIC:rank_dfa_tab_t::unrank", false);
  tm_unrank.resume();
  assert(len < _memoized_size);
  //MyTime tm("rank_dfa_tab_t::unrank");
  res.clear();
  state_id_t cur = pdt->start;
  if (tab[cur][len] < pc) return false;
  for (size_t i = 0; i < len; ++i) {
    unsigned int b=0;
    for (b = 0; b < MAX_SYMS; ++b) {
      state_id_t next = ARR((pdt->tab), cur, b);
      if (pc < tab[next][len - 1 -i])
	break;
      pc = pc - tab[next][len - 1 -i];
      //if (pc == 0) break;
    }
    assert(b < MAX_SYMS);
    //res[i] = b;
    res.push_back(b); assert(res.size() == i+1);
    cur = ARR((pdt->tab), cur, b);
  }
  assert(pc == 0);
  assert(res.length() == len);
  tm_unrank.stop();
  return true;
}

void rank_dfa_tab_t::report_size(const char* msg, int verbose_level) {
  unsigned int tab_entries = pdt->num_states * _memoized_size;
  cout << "rank_dfa_tab_t::report_size:" << msg
       << " : pdt->num_states= " << pdt->num_states
       << " : tab_entries= " << tab_entries;
  if (verbose_level >= 2) {
	unsigned int nz_entries = 0;
    size_t bits = get_memoized_bits(&nz_entries);
    cout << " : nz_entries= " << nz_entries
	 << " : bits= " << bits;
  }
  cout << endl;
}
size_t rank_dfa_tab_t::get_memoized_bits(unsigned int *nz_entries) const {
  size_t bits = 0;
  if (nz_entries) *nz_entries = 0;
  for (state_id_t s = 0; s < pdt->num_states; s++) {
    for (size_t i = 0; i < _memoized_size; ++i) {
      bits += mpz_sizeinbase (tab[s][i].get_mpz_t(), 2);
	  if (nz_entries && tab[s][i] != 0) ++(*nz_entries);
	}
  }
  return bits;
}
/***************** rank_trx *********************/
void rank_trx::init() {
  DbgTime tm("rank_trx::init");

  DbgTime tm_parse("rank_trx:parse");
  DbgTime tm_to_trex("rank_trx:to_trex");
  local_trx = regex2trex(rex, &tm_parse, &tm_to_trex);

  DbgTime tm_toNFA("rank_trx:toNFA");
  local_snfa_root = snfa_root = local_trx->toNFA(NULL);
  tm_toNFA.stop();
  
  assert(snfa_root->is_root());
}

rank_trx::rank_trx(const string& rex0, size_t max_len) : ranker(rex0) {
  DbgTime tm("rank_trx::rank_trx");
  init();
  update_matches(max_len);
}

rank_trx::rank_trx(const string& rex0) : ranker(rex0) {
  DbgTime tm("rank_trx::rank_trx");
  init();
}


bool rank_trx::rank(MPInt & rank, const char *buf, size_t len) {
  static DbgTime tm_rank("STATIC:rank_trx:rank", false);
  static DbgTime tm_rank_parse("STATIC:rank_trx:rank-parse", false);
  tm_rank.resume();
  
  stateNFA* snfa = snfa_root;
  DbgTime tm("string_rank");
  rank = 0;
  bool success = false;
  rank = -1;
  tm_rank_parse.resume();
  parseTree* pt = snfa->parse(buf, len);
  tm_rank_parse.stop();
  if (pt) {
    //if(DOT_PT){ std::ofstream of(("dot.pt.txt."+toString(pt->uid)).c_str());  pt->to_dot(of, true); of.close();}
    if (!pt->_type->problematic()) {
      success = true;
      DbgTime tm("ranking-and-unranking");
      rank = pt->rank(); //if (DOT_PT) {DMP(rank)<< endl;}
      tm.lap("ranking-done");
    } else {
      cout << "skipped (un)ranking for problematic type " << pt->_type->unparse() << endl;
    }
  }
  delete pt;
  tm_rank.stop();
  return success;
}

bool rank_trx::unrank(MPInt& rank, size_t len, string& res) {
  static DbgTime tm_unrank("STATIC:rank_trx:unrank", false);
  tm_unrank.resume();
  
  trex *type = snfa_root->Type();
  DbgTime tm("string_unrank");
  bool success = false;
  res.clear();
  parseTree* upt = pt_unrank(rank, type, len);
  //if(DOT_PT){ std::ofstream of(("dot.upt.txt."+toString(upt->uid)).c_str());  upt->to_dot(of, true); of.close();}
  if (upt) {
    success = true;
    upt->get_raw_accept(res);
    delete upt;
  }
  tm_unrank.stop();
  
  return success;
}

void rank_trx::report_size(const char* msg, int verbose_level) {
  std::set<trex*> types;
  gather_trex_nodes(snfa_root->Type(), types);
  
  unsigned int nz_entries = 0;

  cout << "rank_trx::report_size:" << msg
       << " : NumTypes= " << types.size();

  if (verbose_level >= 2) {
    size_t bits = get_memoized_bits(&nz_entries);
    cout << " : nz_entries= " << nz_entries
	 << " : bits= " << bits;
  }
  cout << endl;
}

size_t rank_trx::get_memoized_bits(unsigned int *nz_entries) const {
  std::set<trex*> types;
  gather_trex_nodes(snfa_root->Type(), types);
  size_t bits = 0;
  if (nz_entries) *nz_entries = 0;
  for(std::set<trex*>::iterator ti = types.begin(); ti != types.end(); ++ti) {
    const std::vector<MPInt>& _num_matches = (*ti)->get_num_matches();
	for (std::vector<MPInt>::const_iterator cit = _num_matches.begin();
	  cit != _num_matches.end(); ++cit) {
		bits += mpz_sizeinbase ((*cit).get_mpz_t(), 2);
		if (nz_entries && (*cit) != 0) ++(*nz_entries);
	}
  }
  return bits;
}
