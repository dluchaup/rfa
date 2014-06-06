/*-----------------------------------------------------------------------------
 * File:    ffa.h
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
#ifndef FFA_H
#define FFA_H
#include <assert.h>
#include <bitset>
#include <fstream>
#include <iostream>
#include <vector>
#include <set>
#include <string>

#include "ranker.h"
/************** rank_ffa_tab_t ******************/
struct rank_ffa_tab_t PUBLIC_RANKER {
  const dfa_tab_t *pdt;
  const dfa_tab_t *local_pdt;

  struct succesor_info {
    state_id_t s;
    std::set<unsigned int> s_chars;
    unsigned char _unrank[MAX_SYMS];
    unsigned char _rank[MAX_SYMS];
  };
  std::vector<std::vector<succesor_info> > vsi;
  
  std::vector< std::vector<MPInt> > tab;
  rank_ffa_tab_t(const std::string & rex0, size_t max_len);
  rank_ffa_tab_t(const std::string & rex0);
  rank_ffa_tab_t(const std::string & rex0, const std::string & neg);//rex0-neg
  void init();
  void init_vsi();
  ~rank_ffa_tab_t() {
    static DbgTime tm_nfa_delete("STATIC:~rank_ffa_tab_t", false);
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
  void dump_tab();
  void dotty(const char *dot_name);
  ////////////////////
  void update_matches(unsigned int len);
  const MPInt& num_matches(unsigned int len) {
    if (_memoized_size == 0) return MPInt_zero; assert(len < _memoized_size);
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
  /////////////////////
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

#endif //FFA_H
