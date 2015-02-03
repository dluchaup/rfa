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
#define ACCEPT_PTR_HACK (new int[1])//((int*)0x1)

struct dfa_tab_t {
  /* since we are recording accept ids, we need to store more than
   * just a boolean "yes" or "no" */
  state_id_t *tab;
  int ** acc;
  unsigned int machine_id;
  unsigned int num_states;
  state_id_t start;
  std::string filename;
  unsigned long long *stat;
  ////////
#define FEEDBEEF 1//0xFEEDBEEF
  void clear() {
    //bzero((char *) this, sizeof(*this));
    tab = NULL;
    acc = NULL;
    machine_id = 0xbadbeef;
    num_states = 0;
    start = 0;
  }
  dfa_tab_t() { clear(); }
  dfa_tab_t(const char* rex) { clear(); init(rex); }
  dfa_tab_t(const std::string& rex, unsigned int id = FEEDBEEF)
    { clear(); init(rex.c_str(), id); }
  ~dfa_tab_t() {cleanup();}// TBD: Check if this is compatible with old uses of dfa_tab_t!

  bool txt_dfa_load(const char *filename);
  bool txt_dfa_load(const std::string& filename)
    { return txt_dfa_load(filename.c_str()); }
  bool bin_dfa_dump(const char *filename);
  bool txt_dfa_dump(const char *filename);//ex HACK_dfa_dump
  
  void init(const char *rex, unsigned int id = FEEDBEEF);
  void dfa2nfa2dotty(const char *filename);
  
  bool bin_dfa_load(const char *filename);
  
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

#define dbgCheck(dt)      /* enable function if needed */

/* dump 'dt' to filename according to kind: "bin" or "txt" */
bool dfa_dump(const dfa_tab_t *dt, const char *filename, const char *kind);


/* Get a new DFA which accepts the union of dt1 and dt2's languages        */
dfa_tab_t* dt_union       (const dfa_tab_t* dt1, const dfa_tab_t* dt2,
			   unsigned int max_states=UINT_MAX,
			   bool minimize = true);
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

/*****************************************************************************/
/***  stuff for generic programming                                        ***/
/*****************************************************************************/
inline state_id_t transition(state_id_t src, const dfa_tab_t* dt , unsigned int sym) {
  state_id_t dst = ARR(dt->tab, src, sym);
  return dst;
}
inline state_id_t num_states(const dfa_tab_t* dt) {
  return dt->num_states;
}
extern bool equalDfa(const dfa_tab_t * dfa1, const dfa_tab_t* dfa2);
extern int* clone_accept_ids(int* acc);
extern void guarded_minimize(dfa_tab_t * &dt, state_id_t &lastMinNumberOfStates);

#endif
