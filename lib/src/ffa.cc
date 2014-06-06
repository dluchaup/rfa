/*-----------------------------------------------------------------------------
 * File:    ffa.cc
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
#include <map>
#include "ffa.h"
#include "dbg.h"
#include "escape_sequences.h"
#include "dfa_tab_t.h"

using namespace std;
/************** rank_ffa_tab_t ******************/
void rank_ffa_tab_t::dotty(const char *dot_name) {
  (const_cast<dfa_tab_t*>(pdt))->dfa2nfa2dotty(dot_name);
}

void rank_ffa_tab_t::dump_tab() {
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

void rank_ffa_tab_t::init() {
  DbgTime tm("rank_ffa_tab_t::init");

  DbgTime tm_parse("rank_ffa_tab_t:parse", false);
  DbgTime tm_to_trex("rank_ffa_tab_t:to_trex", false);
  trex *trx = regex2trex(rex, &tm_parse, &tm_to_trex);

  DbgTime tm_toNFA("rank_ffa_tab_t:toNFA");
  stateNFA *snfa = trx->toNFA(NULL);
  tm_toNFA.stop();
  
  assert(snfa->is_root());
  

  DbgTime tm_make_dfa("rank_ffa_tab_t:make_dfa");
  local_pdt = pdt = snfa->make_dfa();
  tm_make_dfa.stop();
  
  tab.clear();  tab.resize(pdt->num_states);

  DbgTime tm_delete("rank_ffa_tab_t:delete");
  stateNFA::delete_all_reachable(snfa);
  delete trx;
  tm_delete.stop();
}

rank_ffa_tab_t::rank_ffa_tab_t(const string & rex0, size_t max_len)
  : ranker(rex0) {
  DbgTime tm("rank_ffa_tab_t::rank_ffa_tab_t");
  init();
  update_matches(max_len);
}

rank_ffa_tab_t::rank_ffa_tab_t(const string & rex0)
  : ranker(rex0) {
  DbgTime tm("rank_ffa_tab_t::rank_ffa_tab_t");
  init();
}

rank_ffa_tab_t::rank_ffa_tab_t(const std::string & rex0,
			       const std::string & neg)
  : ranker(rex0) {
  DbgTime tm("rank_ffa_tab_t::rank_ffa_tab_t");
  init();
  {
    DbgTime tm("rank_ffa_tab_t::...neg");
    dfa_tab_t pdt_neg(neg);
    dfa_tab_t *new_pdt = dt_difference(pdt, &pdt_neg);
    delete pdt;
    local_pdt = pdt = new_pdt;
    tab.clear();  tab.resize(pdt->num_states);
  }
}

void rank_ffa_tab_t::init_vsi() {
  static DbgTime tm_update_matches("STATIC:rank_ffa_tab_t:init_vsi", false);
  tm_update_matches.resume();
  unsigned int num_states = pdt->num_states;
  vsi.resize(num_states);
  for (state_id_t s = 0; s < num_states; ++s) {
    /********** find the successor set of s **********/
    set<state_id_t> successors;
    for (unsigned int c = 0; c < MAX_SYMS; c++) {
      state_id_t next = ARR((pdt->tab), s, c);
      successors.insert(next);
    }
    vsi[s].resize(successors.size());
    state_id_t dbg_last_succ = 0;
    map<state_id_t, unsigned int> succ2index;
    unsigned int n_index = 0;
    /** set proper successor indexes in vsi[s][..] ***/
    for (set<state_id_t>::iterator sit = successors.begin();
	 sit != successors.end();
	 ++sit, ++n_index) {
      state_id_t next = *sit;
      assert(dbg_last_succ <= next); dbg_last_succ = next;
      vsi[s][n_index].s = next;
      succ2index[next] = n_index;
    }
    /***  set proper s_chars for each successor     ***/
    for (unsigned int c = 0; c < MAX_SYMS; c++) {
      state_id_t next = ARR((pdt->tab), s, c);
      assert(CONTAINS(succ2index, next));
      unsigned int index = succ2index[next];
      assert(vsi[s][index].s == next);
      vsi[s][index].s_chars.insert(c);
    }

    /***  set _unrank for chars for each successor   ***/
    n_index = 0;
    for (set<state_id_t>::iterator sit = successors.begin();
	 sit != successors.end();
	 ++sit, ++n_index) {
      state_id_t next = *sit;
      assert(vsi[s][n_index].s == next);
      //for (unsigned int c = 0; c < MAX_SYMS; c++) {vsi[s][n_index]._unrank[c] = 0;}
      bzero(vsi[s][n_index]._unrank,
	    MAX_SYMS * sizeof(vsi[s][n_index]._unrank[0]));
      bzero(vsi[s][n_index]._rank,
	    MAX_SYMS * sizeof(vsi[s][n_index]._rank[0]));
      
      std::set<unsigned int>::iterator cit;
      unsigned int c_index = 0;
      unsigned int dbg_last_c = 0;
      // assumed ordered set
      for (cit =  vsi[s][n_index].s_chars.begin();
	   cit !=  vsi[s][n_index].s_chars.end();
	   ++cit, ++ c_index) {
	unsigned int c = *cit;
	assert(dbg_last_c <= c); dbg_last_c = c;
	vsi[s][n_index]._rank[c] = c_index;
	vsi[s][n_index]._unrank[c_index] = c;
      }
    }
  }
  tm_update_matches.stop();
#if 0 // debugging
  //////////////////////////////////////////////////////////////////////
  for (state_id_t cur = 0; cur < num_states; ++cur) {
    vector<succesor_info> ::iterator it;
    cout << "cur=" << cur << ":: ";
    for(it = vsi[cur].begin(); it != vsi[cur].end(); ++it) {
      state_id_t next = (*it).s;
      cout << "[" << next << ":";
      unsigned int c_rank;
      // assumed ordered set
      for(c_rank = 0; c_rank < ((*it).s_chars.size()); ++c_rank) {
	cout << "("
	     << ((unsigned int)(*it)._unrank[c_rank]) << "#" << c_rank << ")";
      }
      cout << "],";
    }
    cout << endl;
  }
  //////////////////////////////////////////////////////////////////////
#endif
}

void rank_ffa_tab_t::update_matches(unsigned int len) {
  static DbgTime tm_update_matches("STATIC:rank_ffa_tab_t:update_matches", false);

  DbgTime tm("rank_ffa_tab_t::update_matches."+toString(len));
  unsigned int num_states = pdt->num_states;
  if (len < _memoized_size)
    return;
  tm_update_matches.resume();
  init_vsi();
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
      vector<succesor_info> ::iterator it;
      for(it = vsi[s].begin(); it != vsi[s].end(); ++it) {
	state_id_t next = (*it).s;
	tab[s][i] = tab[s][i] + ((*it).s_chars.size()) * tab[next][i-1];
      }
    }
  }
  tm_update_matches.stop();

}
bool rank_ffa_tab_t::rank(MPInt & pc, const char *str, size_t len) {
  static DbgTime tm_rank("STATIC:rank_ffa_tab_t::rank", false);
  tm_rank.resume();
  
  assert(len < _memoized_size);
  //MyTime tm("rank_ffa_tab_t::rank");
  pc = 0;
  state_id_t cur = pdt->start;
  for (size_t i = 0; i < len; ++i) {
    unsigned char c = str[i];
    state_id_t next_cur = ARR((pdt->tab), cur, c);

    bool dbg_found = false;
    vector<succesor_info> ::iterator it;
    for(it = vsi[cur].begin(); it != vsi[cur].end(); ++it) {
      state_id_t next = (*it).s;
      if (next == next_cur) {
	pc = pc + ((*it)._rank[c]) * tab[next][len - 1 -i];
	cur = next;
	dbg_found = true;
	break;
      }
      pc = pc + ((*it).s_chars.size()) * tab[next][len - 1 -i];
    }
    assert(dbg_found);
  }
  tm_rank.stop();
  return pdt->acc[cur]!=NULL;
}

bool rank_ffa_tab_t::unrank(MPInt& pc, size_t len, string &res) {
  static DbgTime tm_unrank("STATIC:rank_ffa_tab_t::unrank", false);
  tm_unrank.resume();
  assert(len < _memoized_size);
  //MyTime tm("rank_ffa_tab_t::unrank");
  res.clear();
  state_id_t cur = pdt->start;
  if (tab[cur][len] < pc) return false;
  for (size_t i = 0; i < len; ++i) {
    assert(res.length() == i);
    unsigned int rest_len = len - 1 - i;
    vector<succesor_info> ::iterator it;
    bool dbg_found = false;
    for(it = vsi[cur].begin(); it != vsi[cur].end(); ++it) {
      state_id_t candidate_next = (*it).s;
      MPInt candidate_paths;
      candidate_paths = ((*it).s_chars.size())* tab[candidate_next][rest_len];
      if (pc < candidate_paths) {
	unsigned int c_rank;
	// assumed ordered set
	MPInt q = pc / tab[candidate_next][rest_len];
	pc = pc % tab[candidate_next][rest_len];//weird: mpz_tdiv_qr = no gains
	c_rank = MPInt_to_uint(q);
	assert(c_rank < (*it).s_chars.size());
	t_term c = (*it)._unrank[c_rank];/*unrank not needed: see s_chars*/
	res.push_back(c);
	assert(candidate_next == ARR((pdt->tab), cur, c));
	cur = candidate_next;
	dbg_found = true;
	break;
      }
      pc = pc - candidate_paths;
    }
    assert(dbg_found);
  }
  assert(pc == 0);
  assert(res.length() == len);
  tm_unrank.stop();
  return true;
}

void rank_ffa_tab_t::report_size(const char* msg, int verbose_level) {
  unsigned int tab_entries = pdt->num_states * _memoized_size;
  unsigned int nz_entries = 0;
  cout << "rank_ffa_tab_t::report_size:" << msg
       << " : pdt->num_states= " << pdt->num_states
       << " : tab_entries= " << tab_entries;
  if (verbose_level >= 2) {
    size_t bits = get_memoized_bits(&nz_entries);
    cout << " : nz_entries= " << nz_entries
	 << " : bits= " << bits;
  }
  cout << endl;
}
size_t rank_ffa_tab_t::get_memoized_bits(unsigned int *nz_entries) const {
  size_t bits = 0;
  if (nz_entries) *nz_entries=0;
  for (state_id_t s = 0; s < pdt->num_states; s++) {
    for (size_t i = 0; i < _memoized_size; ++i) {
      size_t bits0 = mpz_sizeinbase (tab[s][i].get_mpz_t(), 2);
      bits += bits0;
	  if (nz_entries && tab[s][i] != 0) ++(*nz_entries);
	  //cout << "KIND1: ";DMP(s);DMP(i);DMPNL(tab[s][i]);
	  //cout << "KIND2: ";DMP(s);DMP(i);DMP(tab[s][i]); DMPNL(bits0);
	  //cout << "KIND3: ";DMP(s);DMP(i);DMP(tab[s][i]); DMP(bits0); DMPNL(bits);
	}
  }
  return bits;
}
