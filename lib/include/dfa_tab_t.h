/*-----------------------------------------------------------------------------
 * File:    dfa_tab_t.h
 * Author:  Daniel Luchaup
 * Date:    26 November 2013
 * Copyright 2007-2013 Daniel Luchaup luchaup@cs.wisc.edu
 *
 * This file contains unpublished confidential proprietary work of
 * Daniel Luchaup, Department of Computer Sciences, University of
 * Wisconsin--Madison.  No use of any sort, including execution, modification,
 * copying, storage, distribution, or reverse engineering is permitted without
 * the express written consent (for each kind of use) of Daniel Luchaup.
 *
 *-----------------------------------------------------------------------------
 * History
 * evolved from legacy code/interface written in 2007 by author: Randy Smith
 *---------------------------------------------------------------------------*/
#ifndef DFA_TAB_T_H
#define DFA_TAB_T_H

#include <set>
#include <string>
#include <climits> // for UINT_MAX

// TBD: duplicate from nfa.h
typedef unsigned int uint;
typedef uint state_id_t;

const state_id_t illegal_state = (state_id_t)(-1);

#define ARR(arr, state, chr) ((arr)[((state)<<8) + (chr)]) // state*MAX_SYMS
#define ARR2(arr, state, chr2) ((arr)[((state)<<(8*2)) + (chr2)])
#define ACCEPT_PTR_HACK (new int[1])//((int*)0x1)

struct dfa_tab_t {
  /* since we are recording accept ids, we need to store more than
   * just a boolean "yes" or "no" */
  state_id_t *tab;
  int ** acc;
  int  *sink;//=1=accept; 2=reject; 0 = none
  state_id_t reject_state;
  unsigned int machine_id;
  unsigned int num_states;
  state_id_t start;
  std::string filename;
  /* In fact for postfix closed regular expressions, there should be only
  ** one accepting state!
  */
  state_id_t accepting_state;
  unsigned long long *stat;
  ////////
  void clear() {
    //bzero((char *) this, sizeof(*this));
    tab = NULL;
    acc = NULL;
    sink = NULL;
    machine_id = 0xbadbeef;
    num_states = 0;
    start = 0;
    reject_state = illegal_state;
  }
  dfa_tab_t() { clear(); }
  dfa_tab_t(const char* rex) { clear(); init(rex); }
  dfa_tab_t(const std::string& rex) { clear(); init(rex.c_str()); }
  ~dfa_tab_t() {cleanup();}// TBD: Check if this is compatible with old uses of dfa_tab_t!

  bool DFAload(const char *filename);
  bool DFAload(const std::string& filename) {return DFAload(filename.c_str());}
  bool dump(const char *filename);
  bool HACK_dfa_dump(const char *filename);
  
  void init(const char *rex);
  void dfa2nfa2dotty(const char *filename);
  
  bool load(const char *filename);
  
  void cleanup(void);
  void partial_cleanup_for_resize(state_id_t lim);
  void minimize(bool clean_old = true);
  
  bool strict_match(const unsigned char *buf, unsigned int len) ;
  
  bool dbg_same(dfa_tab_t &other);
  bool is_accepting(state_id_t s) const { return this->acc[s] != NULL;}
  int num_accept_ids(state_id_t s);
  int* clone_accept_ids(state_id_t s);
  //gather_accept_ids 'appends' the accept_id from s to accept_ids
  void gather_accept_ids(state_id_t s, std::set<int> & accept_ids) const;
//#define STATS
};

/* Get a new DFA which accepts the union of dt1 and dt2's languages        */
dfa_tab_t* dt_union       (const dfa_tab_t* dt1, const dfa_tab_t* dt2,
			   unsigned int max_states=UINT_MAX);
/* Get a new DFA which accepts the intersection of dt1 and dt2's languages */
dfa_tab_t* dt_intersection(const dfa_tab_t* dt1, const dfa_tab_t* dt2,
			   unsigned int max_states=UINT_MAX);  
/* Get a new DFA which accepts the difference of dt1 and dt2's languages   */
dfa_tab_t* dt_difference  (const dfa_tab_t* dt1, const dfa_tab_t* dt2,
			   unsigned int max_states=UINT_MAX);


/* Get a new identical copy/clone of dt                                    */
dfa_tab_t* dt_clone(const dfa_tab_t* dt);

/* Get a new DFA which accepts the complement of dt's language             */
dfa_tab_t* dt_complement  (const dfa_tab_t* dt);

/* Change dt in place s.t. it accepts the complement of original language  */
void dt_negate  (dfa_tab_t* dt);

extern bool minimMDFA;
#endif
