/*-----------------------------------------------------------------------------
 * File:    ranker.h
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
#ifndef RANKER_H
#define RANKER_H
#include <assert.h>
#include <bitset>
#include <fstream>
#include <iostream>
#include <vector>
#include <set>
#include <string>

#include "trex.h"
#include "tnfa.h"
#include "dfa_tab_t.h"

/************* rankerinterface *****************/
struct rankerinterface {
 public:
  //unsigned int _memoized_size; memoize?
  virtual ~rankerinterface() {;}
  virtual const MPInt& num_matches(unsigned int len) = 0;
  size_t capacity(unsigned int len) {
    MPInt n = num_matches(len);
    return (n != 0)?  mpz_sizeinbase (n.get_mpz_t(), 2) : 0;
  }
  virtual void update_matches(unsigned int len) = 0;
  virtual unsigned int get_memoized_size() const = 0;
  virtual size_t get_memoized_bits(unsigned int *nz_entries=NULL) const = 0;
  unsigned int max_achievable_len() {
    unsigned int _memoized_size = get_memoized_size();
    assert(_memoized_size > 0);
    unsigned int max_len = _memoized_size-1;
    MPInt n = num_matches(max_len);
    while (n == 0 && max_len > 0) {
      --max_len;
      n = num_matches(max_len);
    }
    return max_len;
  }
  virtual bool rank(MPInt & pc, const char *str, size_t len) = 0;
  bool rank(MPInt & pc, const std::string & str) {
    return rank(pc, str.c_str(), str.length());
  }
  virtual bool matches(const std::string & str) = 0;
  virtual bool unrank(MPInt& pc, size_t len, std::string& res) = 0;
  void test_unrank_rank(unsigned int max, unsigned int count);
  virtual void report_size(const char* msg, int verbose_level) = 0;
  void ranker_report_size(const char* msg, int verbose_level);

  virtual bool can_be_ambiguous() { return true; }
  virtual bool is_ambiguous(unsigned int len, rankerinterface& exact_rkr) {
    if (this->num_matches(len) == exact_rkr.num_matches(len))
      return false;
    else
      return true;
  }
  virtual unsigned count_ambiguous(unsigned int len, unsigned int trials=100);
  virtual float get_ambiguous(unsigned int len, unsigned int trials=100) {
    return ((float)count_ambiguous(len, trials))/trials;
  }
  virtual float get_ambiguous(unsigned int len, rankerinterface& exact_rkr) {
    if (this->num_matches(len) == exact_rkr.num_matches(len))
      return 1;
    else {
      const MPInt& ne = exact_rkr.num_matches(len);
      const MPInt& n = this->num_matches(len);
      assert(n > ne);
      const int scale = 1000;
      MPInt d = (scale*n)/ne;
      assert(d.fits_uint_p());
      unsigned int ud = MPInt_to_uint(d);
      return ((float)ud)/scale;
    }
  }
};
/************* ranker ***************************/
struct ranker : public rankerinterface {
  unsigned int _memoized_size;
  const std::string rex;
  ranker(const std::string & rex0) :  _memoized_size(0), rex(rex0) {;}
  virtual ~ranker() {;}
  virtual void update_matches(unsigned int len) = 0;
  virtual const MPInt& num_matches(unsigned int len) = 0;
  inline unsigned int get_memoized_size() const { return _memoized_size;}
  virtual size_t get_memoized_bits(unsigned int *nz_entries=NULL) const = 0;
  virtual bool rank(MPInt & pc, const char *str, size_t len) = 0;
  virtual bool matches(const std::string & str) = 0;
  virtual bool unrank(MPInt& pc, size_t len, std::string& res) = 0;
  void test_unrank_rank(unsigned int max, unsigned int count);
  virtual void report_size(const char* msg, int verbose_level) = 0;
  void ranker_report_size(const char* msg, int verbose_level);
};

#define PUBLIC_RANKER : public ranker
/************** rank_dfa_tab_t ******************/
struct rank_dfa_tab_t PUBLIC_RANKER {
  const dfa_tab_t *pdt;
  const dfa_tab_t *local_pdt;
  std::vector< std::vector<MPInt> > tab;
  std::vector< std::vector<MPInt> > ftab;
  rank_dfa_tab_t(const std::string& rex0, const dfa_tab_t *pdt0,size_t max_len);
  rank_dfa_tab_t(const std::string & rex0, size_t max_len);
  rank_dfa_tab_t(const std::string & rex0);
  void init();
  ~rank_dfa_tab_t() {
    static DbgTime tm_nfa_delete("STATIC:~rank_dfa_tab_t", false);
    tm_nfa_delete.resume();
    delete local_pdt;
    tm_nfa_delete.stop();
  }
  void fast_tab();
  
  void get_preds(std::set<state_id_t> &result,
		 std::set<state_id_t> &states,
		 const std::vector<std::set<state_id_t> > &preds) {
    result.clear();
    for (std::set<state_id_t>::iterator it = states.begin();
	 it != states.end();
	 ++it) {
      state_id_t s = *it;
      const std::set<state_id_t> & s_preds = preds[s];
      for (std::set<state_id_t>::const_iterator pit = s_preds.begin();
	   pit != s_preds.end();
	   ++pit) {
	result.insert(*pit);
      }
    }
  }
  bool check_tabs();
  void dump_tab();
  void dotty(const char *dot_name);
  ////////////////////
  void update_matches(unsigned int len);
  const MPInt& num_matches(unsigned int len) {
    if (_memoized_size == 0) return MPInt_zero;
    assert(len < _memoized_size);
    return tab[pdt->start][len];
  }
  virtual size_t get_memoized_bits(unsigned int *nz_entries=NULL) const;
  bool rank(MPInt & pc, const char *str, size_t len);
  bool rank(MPInt & pc, const std::string & str) {
    return rank(pc, str.c_str(), str.length());
  }
  virtual bool matches(const std::string & str) {
    return (const_cast<dfa_tab_t*>(pdt))->
	    strict_match((unsigned char*)str.c_str(), str.length());
  }
  bool unrank(MPInt& pc, size_t len, std::string& res);
  void report_size(const char* msg, int verbose_level);
  //////////////////////
  virtual bool can_be_ambiguous() { return false; }
  virtual bool is_ambiguous(unsigned int , rankerinterface& ) {
		  return false;
  }
  virtual unsigned count_ambiguous(unsigned int , unsigned int) {
		  return 0;
  }
  virtual float get_ambiguous(unsigned int, unsigned int) {
		  return 1;
  }
  virtual float get_ambiguous(unsigned int, rankerinterface& ) {
		  return 1;
  }
};

/************** rank_nfa_tab_t ******************/
struct rank_nfa_tab_t PUBLIC_RANKER {
  stateNFA *snfa_root, *local_snfa_root;
  state_id_t start;
  std::vector<stateNFA*> nfa_nodes;
  std::vector< std::vector<MPInt> > tab;
  rank_nfa_tab_t(const std::string &rex0);
  rank_nfa_tab_t(const std::string &rex0, size_t max_len0);
  rank_nfa_tab_t(const std::string& rex0, stateNFA *snfa, size_t max_len0);
  ~rank_nfa_tab_t() {
    static DbgTime tm_nfa_delete("STATIC:~rank_nfa_tab_t", false);
    tm_nfa_delete.resume();
    if (local_snfa_root){
      delete local_snfa_root->Type();
      stateNFA::delete_all_reachable(local_snfa_root);
    }
    tm_nfa_delete.stop();
  }
  void init();
  
  void dump_tab();
  void dotty(const char *dot_name);
  ////////////////////
  void update_matches(unsigned int len);
  const MPInt& num_matches(unsigned int len) {
    if (_memoized_size == 0) return MPInt_zero;
    assert(len < _memoized_size);
    return tab[start][len];
  }
  virtual size_t get_memoized_bits(unsigned int *nz_entries=NULL) const;
  bool rank(MPInt & pc, const char *str, size_t len);
  bool rank(MPInt & pc, const std::string & str) {
    return rank(pc, str.c_str(), str.length());
  }
  virtual bool matches(const std::string & str) {
    return snfa_root->match(str.c_str(), str.length(), stateNFA::FWD);
  }
  bool unrank(MPInt& pc, size_t len, std::string& res);
  void report_size(const char* msg, int verbose_level);
  bool known_ambiguous() {return snfa_root->Type()->known_ambiguous;}
};

/***************** rank_trx *********************/
struct rank_trx PUBLIC_RANKER {
  trex *local_trx;
  stateNFA *snfa_root;
  stateNFA *local_snfa_root;
  rank_trx(const std::string& rex0);
  rank_trx(const std::string& rex0, size_t max_len);
  rank_trx(const std::string& rex0, stateNFA *snfa, size_t max_len)
    : ranker(rex0), local_trx(NULL), snfa_root(snfa), local_snfa_root (NULL) {
    update_matches(max_len);
  }
  void init();
  ~rank_trx() {
    static DbgTime tm_trx_delete("STATIC:~rank_trx", false);
    tm_trx_delete.resume();
    if (local_snfa_root)
      stateNFA::delete_all_reachable(local_snfa_root);
    delete local_trx;
    tm_trx_delete.stop();
  }
  
  ////////////////////
  void update_matches(unsigned int len) {
    static DbgTime tm_update_matches("STATIC:rank_trx:update_matches", false);

    assert(!snfa_root->Type()->problematic());
    tm_update_matches.resume();
    snfa_root->Type()->update_matches(len);
    tm_update_matches.stop();
    _memoized_size = len+1;
  }
  const MPInt& num_matches(unsigned int len) {
    return snfa_root->Type()->num_matches(len);
  }
  bool rank(MPInt & pc, const char *str, size_t len);
  bool rank(MPInt & pc, const std::string & str) {
    return rank(pc, str.c_str(), str.length());
  }
  virtual size_t get_memoized_bits(unsigned int *nz_entries=NULL) const;
  virtual bool matches(const std::string & str) {
    return snfa_root->match(str.c_str(), str.length(), stateNFA::FWD);
  }
  bool unrank(MPInt& pc, size_t len, std::string& res);
  void report_size(const char* msg, int verbose_level);
  bool known_ambiguous() {return snfa_root->Type()->known_ambiguous;}
};

#endif //RANKER_H
