/*-----------------------------------------------------------------------------
 * File:    tnfa.cc
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
#include <algorithm>
#include <fstream>
#include <sstream>
#include <map>
#include <set>

#include "tnfa.h"
#include "trex.h"
#include "escape_sequences.h"
#include "ranker.h"

#include "dbg.h"
#include "zoptions.h"


#define REX_VERBOSE 0

using namespace std;
bool stateNFA::dbg_reset_stateNFA_guid = false;
/********** uidObject ***************************/
unsigned int uidObject::g_uido;

/************************************************/
/* filters are used to gather reachanbe sets    */
/********** type_filter *************************/
class type_filter {
protected:
  set<stateNFA*,uidObject::cmpUidObject> candidates;
  unsigned int index;
public:
  virtual bool add_candidate(stateNFA*) = 0;
  void set_index(unsigned int ix) {index = ix;};
  void clear_candidates(){candidates.clear();};
  virtual set<stateNFA*,uidObject::cmpUidObject>& get_candidates() {
    return candidates;
  }
};
////////////////////
class permisive_filter : public type_filter {
  bool add_candidate(stateNFA* snfa) {
    candidates.insert(snfa);
    return true;
  }
};
////////////////////
class match_filter : public type_filter {
protected:
  bool in_record_mode;
  vector <set<trex*,uidObject::cmpUidObject> > dbg_type_recorded;
  vector <set<trex*,uidObject::cmpUidObject> > dbg_type_seen;
  vector <set<stateNFA*,uidObject::cmpUidObject> > recorded_sets;
  vector <set<stateNFA*,uidObject::cmpUidObject> > seen_state_sets;
public:
  match_filter(int len) :
    in_record_mode(true),
    dbg_type_recorded(len), dbg_type_seen(len),
    recorded_sets(len), seen_state_sets(len)
    {}
  bool add_candidate(stateNFA* snfa) {
    trex_byte *trx = dynamic_cast<trex_byte*>(snfa->Type());
    assert(trx);
    bool allow = false;
    if (in_record_mode) {
      allow = true;
      recorded_sets[index].insert(snfa->other_end());
      dbg_type_recorded[index].insert(trx);
      //DMP(recorded_sets[index].size()); DMP(dbg_type_recorded[index].size());
      /* The following assertion should hold for Thomson NFA. However, for
	 performance reasons, we don't use Thomson for expressions with R{l,h}
	 See: term::to_trex() ...  case quantifier::RANGE:
	 If zop.dbgBoolOption("thompson_expand_range") then the assertion should hold
      */
      //assert(recorded_sets[index].size() == dbg_type_recorded[index].size());
    } else if (CONTAINS(recorded_sets[index], snfa)) {
      allow = true;
      assert(!CONTAINS(seen_state_sets[index], snfa));// I think this should hold. If it doesn't, it would be interesting to understand why not...
      seen_state_sets[index].insert(snfa);
      dbg_type_seen[index].insert(trx);
      assert(seen_state_sets[index].size() == dbg_type_seen[index].size());
    }
    if (allow)
      candidates.insert(snfa);
    return allow;
  }
  void set_index(unsigned int index0) {
    assert(index0 < recorded_sets.size());
    index = index0;
  }
  void no_record_mode() { in_record_mode = false;}
  virtual bool check(const char *buf, unsigned int len, bool is_match) {
    bool empty_seen = false;
    assert(len == seen_state_sets.size());
#define DBGFILTER REX_VERBOSE
    for (unsigned int i = 0; i < len; ++i) {
      if(DBGFILTER) cout << "FILTER-CHECK[" << i << "] '" << buf[i] << "' ";
      if (seen_state_sets[i].empty()) {
	assert(!is_match);
	empty_seen = true;
      } else {
	assert(!empty_seen);
	if(DBGFILTER) cout << "#" << seen_state_sets[i].size() << ": ";
	for(set<stateNFA*,uidObject::cmpUidObject>::iterator it = seen_state_sets[i].begin();
	    it != seen_state_sets[i].end();
	    ++it) {
	  trex_byte *tb = dynamic_cast<trex_byte*>((*it)->Type()); assert(tb);
	  assert(tb->matches(buf[i]));
	  if(DBGFILTER) cout << (*it)->uid << "(t:" << (*it)->Type()->uid << "),";
	}
      }

      if(DBGFILTER) cout << " %%recorded%% ";
      for(set<stateNFA*,uidObject::cmpUidObject>::iterator it = recorded_sets[i].begin();
	    it != recorded_sets[i].end();
	    ++it) {
	trex_byte *tb = dynamic_cast<trex_byte*>((*it)->Type()); assert(tb);
	assert(tb->matches(buf[i]));
	if(DBGFILTER) cout << (*it)->uid << "(t:" << (*it)->Type()->uid << "),";
      }
      if(DBGFILTER) cout << endl;
    }
    return true;
  }
};
////////////////////
class match_single_path_filter : public match_filter {
  stateNFA* root;
  vector <stateNFA*> trace;
public:
  match_single_path_filter(stateNFA* root0, int len) :
    match_filter(len), root(root0), trace(len) {}
  set<stateNFA*,uidObject::cmpUidObject> &get_candidates() {
    if (!in_record_mode) {
      if (!candidates.empty()) {
	stateNFA* best = *candidates.begin();// works iff. candidates is sorted!!!
	candidates.clear();
	candidates.insert(best);
	assert(index < trace.size());
	trace[index] = best;
      }
    }
    return candidates;
  }
  vector <stateNFA*>& get_trace() { return trace;}
  parseTree * getParseTree(const char *buf, unsigned int len, bool is_match) {
    match_filter::check(buf, len, is_match);
    if (is_match) {
      std::list<stateNFA*> path;
      if (len != 0) {
	unsigned int rindex = len-1;
	bool ok0 = trace[rindex]->Exit()->find_path(root->Exit(), path, NULL, stateNFA::FWD); assert(ok0);
	for (unsigned int i = 0; i < len; ++i, --rindex) {
	  if (rindex > 0) {
	    bool found = trace[rindex-1]->Exit()->find_path(trace[rindex], path, NULL, stateNFA::FWD);
	    assert(found);
	  }
	}
	bool ok = root->find_path(trace[0], path, NULL, stateNFA::FWD); assert(ok);
      } else {
	assert (trace.size() == 0);
	bool ok = root->find_path(root->Exit(), path, NULL, stateNFA::FWD); assert(ok);
      }
      assert(!path.empty());assert(root == path.front());
      dbgDMPNL(path.size());
      if(DBGFILTER) {
        if (path.size() < 1000)
	  dumpTContainer(path, "Accepting path:");
	else
	  cout << "Accepting path is too long: size=" << path.size() << endl;
	std::ofstream of("dot.path.txt");
	dotty_path(path, of); of.close();
      }

	
      parseTree* pt =  path2parseTree(path, buf, len);
      assert(pt);
      return pt;
    }
    return NULL;
  }
};

/********** stateNFA ****************************/
stateNFA::~stateNFA() {}

void get_matching(setPtrStateNFA& result,
		  setPtrStateNFA& state_set,
		  stateNFA::direction dir) {
  result.clear();
  for (setPtrStateNFA::iterator it = state_set.begin();
       it != state_set.end();
       ++it)
    if ((*it)->is_matching(dir))
      result.insert(*it);
}


void closure_shortcut_links(setPtrStateNFA& result,
			    setPtrStateNFA& state_set,
			    stateNFA::direction dir) {
  result.clear();
  for (setPtrStateNFA::iterator it = state_set.begin();
       it != state_set.end();
       ++it) { // should use set union, but avoid since I intersection is problematic
    stateNFA* snfa = *it;
    if (!snfa->is_matching(dir)) {// stop at matching boundary
      std::vector<stateNFA*> &frontier = snfa->shortcut_links[dir];
      for (std::vector<stateNFA*>::iterator fit = frontier.begin();
	   fit != frontier.end();
	   ++fit) {
	stateNFA* sf = *fit;
	result.insert(sf);
      }
    }
    result.insert(snfa);
  }
}

void stateNFA::link_shortcuts(direction dir) {
  //cout << "stateNFA::link_shortcuts " << dir << " " << uid << endl;
  setPtrStateNFA eps_closed;
  setPtrStateNFA frontier;
  get_closure(eps_closed, NULL, dir);
  get_matching(frontier, eps_closed, dir);
  //assert(this->shortcut_links[dir].empty());//for now prepared to only link once
  for (setPtrStateNFA::iterator fit = frontier.begin();
       fit != frontier.end();
       ++fit) {
    stateNFA* sf = *fit;
    //if (!this->shortcut_links[dir].empty())  assert(this->shortcut_links[dir].back()->uid < sf->uid);// In case this is used for deterministic/"lexicographic" order.
    if (this != sf)
      this->add_shortcut(dir, sf);
  }
}

void stateNFA::link_eps_free_shortcuts(direction dir) {
  //cout << "stateNFA::link_shortcuts " << dir << " " << uid << endl;
  setPtrStateNFA eps_closed;
  setPtrStateNFA frontier;
  get_closure(eps_closed, NULL, dir);
  get_matching(frontier, eps_closed, dir);
  //assert(this->shortcut_links[dir].empty());//for now prepared to only link once
  direction other_dir = (dir==FWD)? BCK:FWD;
  for (setPtrStateNFA::iterator fit = frontier.begin();
       fit != frontier.end();
       ++fit) {
    stateNFA* sf = *fit;
    //assert(this != sf->ends[other_dir]);
    this->add_eps_free_shortcut(dir, sf->ends[other_dir]);
  }
}

void stateNFA::setup_front(direction dir) {
  assert(is_root());
  DbgTime tm(string("link_fwd_front::")+(toString(uid)));
  
  search_scope ss;
  gather_thompson_nodes(this, ss);
  setPtrStateNFA &all_nodes = ss.get_seen();
  setPtrStateNFA matching_nodes;
  get_matching(matching_nodes, all_nodes, dir);
  
  dbgDMP(all_nodes.size()); dbgDMPNL(matching_nodes.size());
  
  //dumpTContainer(all_nodes," all_nodes");
  //dumpTContainer(matching_nodes," matching_nodes");

  direction other_dir = (dir==FWD)? BCK:FWD;
  for (setPtrStateNFA::iterator it = matching_nodes.begin();
       it != matching_nodes.end();
       ++it) {
    setPtrStateNFA eps_closed;
    setPtrStateNFA frontier;
    stateNFA *s = *it;
    trex_byte *tb = dynamic_cast<trex_byte*>(s->Type()); assert(tb);
    s->ends[dir]->add_shortcut(dir, s->ends[other_dir]);
    s->ends[other_dir]->link_shortcuts(dir);
  }
  /***************************************************/
  /***** this is mostly a hack ..... *****************/
  /* links from start node to its' matching frontier */
  this->ends[dir]->link_shortcuts(dir);
  {
    /* links from the reverse matching frontier of the end node to the end node */
    setPtrStateNFA eps_closed;
    setPtrStateNFA frontier;
    this->ends[other_dir]->get_closure(eps_closed, NULL, other_dir);
    get_matching(frontier, eps_closed, other_dir);
    if (CONTAINS(eps_closed, this->ends[dir]))
      frontier.insert(this->ends[dir]);
    for (setPtrStateNFA::iterator fit = frontier.begin();
	 fit != frontier.end();
	 ++fit) {
      stateNFA* sf = *fit;
      assert(find(sf->shortcut_links[dir].begin(),
		  sf->shortcut_links[dir].end(),
		  this->ends[other_dir]) == sf->shortcut_links[dir].end());
      if (sf != this->ends[other_dir])
	sf->add_shortcut(dir, this->ends[other_dir]);
    }
  }
}

void stateNFA::setup_eps_free_front(direction dir) {
  assert(is_root());
  // I am not sure if this works fine for '//' i.e. trex_epsilon
  // assert(this->Type()->rk != rk_epsilon);
  if (!this->eps_free_links[dir].empty()) // must have been setup before
    return;
  DbgTime tm(string("stateNFA::setup_eps_free_front")+(toString(uid)));
  
  search_scope ss;
  gather_thompson_nodes(this, ss);
  setPtrStateNFA &all_nodes = ss.get_seen();
  setPtrStateNFA matching_nodes;
  get_matching(matching_nodes, all_nodes, dir);
  
  dbgDMP(all_nodes.size()); dbgDMPNL(matching_nodes.size());
  
  //dumpTContainer(all_nodes," all_nodes");
  //dumpTContainer(matching_nodes," matching_nodes");
  
  this->ends[dir]->link_eps_free_shortcuts(dir);
  
  direction other_dir = (dir==FWD)? BCK:FWD;
  for (setPtrStateNFA::iterator it = matching_nodes.begin();
       it != matching_nodes.end();
       ++it) {
    setPtrStateNFA eps_closed;
    setPtrStateNFA frontier;
    stateNFA *s = *it;
    s->ends[other_dir]->link_eps_free_shortcuts(dir);
  }
}

///////////////////////////////////////////////////////////////////////////////
#if 1
typedef setPtrStateNFA DfaState;
struct stateSetCmp {
  bool operator() (const DfaState* plhs, const DfaState* prhs) const {
    assert(plhs && prhs);
    bool result = false;
    const DfaState& lhs = *plhs;
    const DfaState& rhs = *prhs;
    if (lhs.size() < rhs.size())
      result = true;
    if (lhs.size() == rhs.size()) {
      DfaState::iterator lit, rit;
      // uid-lexicographic order
      for (lit = lhs.begin(),rit = rhs.begin(); lit != lhs.end(); ++lit,++rit){
	stateNFA *l = *lit;  stateNFA *r = *rit;
	if (l->uid < r->uid) {
	  result = true;
	  break;
	}else if (l->uid > r->uid)
	  break;
      }
    }
    return result;
  }
};

bool do_they_intersect(const setPtrStateNFA &s1, const setPtrStateNFA &s2) {
  setPtrStateNFA::const_iterator it1 = s1.begin();
  setPtrStateNFA::const_iterator it2 = s2.begin();
  while(it1 != s1.end() && it2 != s2.end()) {
    if ((*it1)->uid < (*it2)->uid)
      ++it1;
    else if ((*it1)->uid > (*it2)->uid)
      ++it2;
    else {
      assert((*it1)->uid == (*it2)->uid);
      assert((*it1) == (*it2));
      return true;
    }
  }
  return false;
}

dfa_tab_t* stateNFA::make_dfa_prev(unsigned int accept_id) {
  static bool dbg = zop.dbgBoolOption("dbg_make_dfa");
  assert(is_root()); //otherwise, the shortcut_links' logic needs to be revised
  DbgTime tm(string("stateNFA::make_dfa.")+(toString(uid)));
  setup_front(FWD);
  if (dbg) {
    ofstream dotsNFA(("dot.NFA-trx.after-setup_front."+ toString(uid) + ".txt").c_str());
    this->to_dot(dotsNFA); dotsNFA.close();
  }
  //setup_frontiers(); //only need FWD, but use all for debugging ...
  
  vector<DfaState* > states;
  map<DfaState*, state_id_t, stateSetCmp> state_index;
  vector<state_id_t* > dfa_state_transitions;
  
  setPtrStateNFA entry;
  entry.insert(this);

  DfaState *start = new DfaState();
  closure_shortcut_links(*start, entry, FWD);
  
  states.push_back(start); state_index[start] = states.size() - 1;
  
  tm.lap("before set construction");
  for (size_t i = 0; i < states.size(); ++i) {//states used as a queue!
    const DfaState& current = *states[i];
    
    state_id_t* current_transitions = new state_id_t[MAX_SYMS];
    for (unsigned int c = 0; c < MAX_SYMS; ++c) {
      state_id_t next_index;
      DfaState *ptr_next = new DfaState(); //just in case
      
      for (setPtrStateNFA::iterator it = current.begin();
	   it != current.end();
	   ++it) {
	stateNFA *s = *it;
	
	if ((  (s) == (s)->ends[FWD])     // proper end for this direction
	    && (s->Type()->matches(c))) { // and it's Type consumes 'b'
	  stateNFA *s_next = s->ends[BCK];
	  std::vector<stateNFA*> &s_frontier = s_next->shortcut_links[FWD];
	  for (std::vector<stateNFA*>::iterator fit = s_frontier.begin();
	       fit != s_frontier.end();
	       ++fit) {
	    stateNFA* sf = *fit;
	    ptr_next->insert(sf);
	  }
	  ptr_next->insert(s_next);
	}
      }
      
      map<DfaState*, state_id_t, stateSetCmp>::iterator itnext;
      if ((itnext = state_index.find(ptr_next)) != state_index.end()) {	
	next_index = itnext->second;
	assert(states[next_index] != ptr_next);
	delete ptr_next; ptr_next = NULL;
      } else {
	states.push_back(ptr_next);
	next_index = state_index[ptr_next] = states.size() - 1;
      }
      assert(next_index < states.size());
      current_transitions[c] = next_index;
    }
    dfa_state_transitions.push_back(current_transitions);
    assert(dfa_state_transitions.size() == i+1);
  }
#if 0
  {
    /*************************************************************************/
    /*              some map access timing/debugging code                    */
    /*************************************************************************/
    MyTime("stateNFA::make_dfa:timing/debugging");
    for(map<DfaState*, state_id_t, stateSetCmp>::iterator it = state_index.begin();
	it != state_index.end(); ++it) {
      const pair<DfaState*, state_id_t> &p = *it;
      const DfaState* pset = p.first;
      DfaState ds = *pset;
      assert(CONTAINS(state_index, &ds));
    }
  }
#endif
  tm.lap("after set construction");
  /***************************************************************************/
  /*                  now populate the dfa_tab_t                             */
  /***************************************************************************/
  assert(states.size() == dfa_state_transitions.size());
  dbgDMPNL(dfa_state_transitions.size());
  dfa_tab_t *dt = new dfa_tab_t();
  dt->num_states = states.size();
  dt->start = state_index[start];
  dbgDMPNL(dt->start);
  dt->tab = new unsigned int [MAX_SYMS*(dt->num_states)];
  for (state_id_t i=0; i < dt->num_states; i++) {
    for (unsigned int j=0; j < MAX_SYMS; j++) {
      assert(dfa_state_transitions[i][j] < dt->num_states);
      ARR(dt->tab, i, j) = dfa_state_transitions[i][j];
      if (dbg) cout << "DBG:STATES: i= " << i << " [ (int)j= " << j << " ] = " << ARR(dt->tab, i, j) << endl;
    }
  }
  dt->stat = NULL;
  dt->acc = new int *[dt->num_states];

  unsigned int dbg_num_accepting = 0;
  for (state_id_t i=0; i < dt->num_states; i++) {
    setPtrStateNFA &current = *states[i];
    //setPtrStateNFA out;
    if (dbg) dumpTContainer(current," current."+toString(i));
    if (CONTAINS(current, this->ends[BCK])) {
      if (dbg) cout << " IS ACCEPTING " << i << endl;
      dt->acc[i] = new int[2];
      dt->acc[i][0] = accept_id;
      dt->acc[i][1] = -1;
      ++dbg_num_accepting;
    } else
      dt->acc[i] = NULL;
  }
  tm.lap("after populate");
  assert(dbg_num_accepting > 0);
  /***************************************************************************/
  for (vector<DfaState*>::iterator it=states.begin(); it!=states.end(); ++it)
    delete *it;
  for (vector<state_id_t * >::iterator it =  dfa_state_transitions.begin();
       it != dfa_state_transitions.end();
       ++it)
    delete [](*it);
  tm.lap("after cleaning");

  dbgDMPNL(dt->num_states);
  if (dbg) dt->dfa2nfa2dotty(("dot.made-before-min."+toString(uid)).c_str());
  dt->minimize();
  tm.lap("after minimization");
  
  dbgDMPNL(dt->num_states);
  return dt;
}

// if(dt == NULL) {dt = new ...}; fill dt; return dt;
dfa_tab_t* stateNFA::make_dfa(unsigned int accept_id, dfa_tab_t *dt) {
  static bool dbg = zop.dbgBoolOption("dbg_make_dfa");
  assert(is_root()); //otherwise, the shortcut_links' logic needs to be revised
  DbgTime tm(string("stateNFA::make_dfa.")+(toString(uid)));
  setup_eps_free_front(FWD);
  if (dbg) {
    ofstream dotsNFA(("dot.NFA-trx.after-setup_front."+ toString(uid) + ".txt").c_str());
    this->to_dot(dotsNFA); dotsNFA.close();
  }
  //setup_frontiers(); //only need FWD, but use all for debugging ...
  
  vector<DfaState* > states;
  map<DfaState*, state_id_t, stateSetCmp> state_index;
  vector<state_id_t* > dfa_state_transitions;
  
  DfaState *start = new DfaState();
  start->insert(this);
  
  states.push_back(start); state_index[start] = states.size() - 1;
  
  tm.lap("before set construction");
  for (size_t i = 0; i < states.size(); ++i) {//states used as a queue!
    if (i == 10000000) {
      cout << "\nstateNFA::make_dfa." << toString(uid) << " ABORT" << endl;fflush(NULL);
      return NULL;
    }
    const DfaState& current = *states[i];
    
    DfaState *transitions[MAX_SYMS];
    for (unsigned int c = 0; c < MAX_SYMS; ++c) {
      transitions[c] = new DfaState(); //just in case
    }
    
    for (setPtrStateNFA::iterator it = current.begin();
	 it != current.end();
	 ++it) {
      stateNFA *s = *it;
      std::vector<stateNFA*> &s_next_set = s->eps_free_links[FWD];
      for (std::vector<stateNFA*>::iterator fit = s_next_set.begin();
	   fit != s_next_set.end();
	   ++fit) {
	stateNFA* sn = *fit;
	const std::vector<unsigned int>& chars = sn->Type()->get_chars();
	for (std::vector<unsigned int>::const_iterator ci = chars.begin();
	     ci != chars.end(); ++ci) {
	  t_term c = *ci;
	  transitions[c]->insert(sn);
	}
      }
    }
    
    state_id_t* current_transitions = new state_id_t[MAX_SYMS];
    for (unsigned int c = 0; c < MAX_SYMS; ++c) {
      state_id_t next_index;
      DfaState *ptr_next = transitions[c];
      
      map<DfaState*, state_id_t, stateSetCmp>::iterator itnext;
      if ((itnext = state_index.find(ptr_next)) != state_index.end()) {	
	next_index = itnext->second;
	assert(states[next_index] != ptr_next);
	delete ptr_next; ptr_next = NULL;
      } else {
	states.push_back(ptr_next);
	next_index = state_index[ptr_next] = states.size() - 1;
      }
      assert(next_index < states.size());
      current_transitions[c] = next_index;
    }
    dfa_state_transitions.push_back(current_transitions);
    assert(dfa_state_transitions.size() == i+1);
  }
#if 0
  {
    /*************************************************************************/
    /*              some map access timing/debugging code                    */
    /*************************************************************************/
    MyTime("stateNFA::make_dfa:timing/debugging");
    for(map<DfaState*, state_id_t, stateSetCmp>::iterator it = state_index.begin();
	it != state_index.end(); ++it) {
      const pair<DfaState*, state_id_t> &p = *it;
      const DfaState* pset = p.first;
      DfaState ds = *pset;
      assert(CONTAINS(state_index, &ds));
    }
  }
#endif
  tm.lap("after set construction");
  /***************************************************************************/
  /*                  now populate the dfa_tab_t                             */
  /***************************************************************************/
  assert(states.size() == dfa_state_transitions.size());
  dbgDMPNL(dfa_state_transitions.size());
  if (dt == NULL)
    dt = new dfa_tab_t();
  dt->num_states = states.size();
  dt->start = state_index[start];
  dbgDMPNL(dt->start);
  dt->tab = new unsigned int [MAX_SYMS*(dt->num_states)];
  for (state_id_t i=0; i < dt->num_states; i++) {
    for (unsigned int j=0; j < MAX_SYMS; j++) {
      assert(dfa_state_transitions[i][j] < dt->num_states);
      ARR(dt->tab, i, j) = dfa_state_transitions[i][j];
      if (dbg) cout << "DBG:STATES: i= " << i << " [ (int)j= " << j << " ] = " << ARR(dt->tab, i, j) << endl;
    }
  }
  dt->stat = NULL;
  dt->acc = new int *[dt->num_states];


  setPtrStateNFA eps_closed_accept;
  this->ends[BCK]->get_closure(eps_closed_accept, NULL, BCK);
  
  unsigned int dbg_num_accepting = 0;
  for (state_id_t i=0; i < dt->num_states; i++) {
    setPtrStateNFA &current = *states[i];
    //setPtrStateNFA out;
    if (dbg) dumpTContainer(current," current."+toString(i));
    if (do_they_intersect(eps_closed_accept, current)) {
      if (dbg) cout << " IS ACCEPTING " << i << endl;
      dt->acc[i] = new int[2];
      dt->acc[i][0] = accept_id;
      dt->acc[i][1] = -1;
      ++dbg_num_accepting;
    } else
      dt->acc[i] = NULL;
  }
  tm.lap("after populate");
  assert(dbg_num_accepting > 0);
  /***************************************************************************/
  for (vector<DfaState*>::iterator it=states.begin(); it!=states.end(); ++it)
    delete *it;
  for (vector<state_id_t * >::iterator it =  dfa_state_transitions.begin();
       it != dfa_state_transitions.end();
       ++it)
    delete [](*it);
  tm.lap("after cleaning");

  dbgDMPNL(dt->num_states);
  if (dbg) dt->dfa2nfa2dotty(("dot.made-before-min."+toString(uid)).c_str());
  dt->minimize();
  tm.lap("after minimization");
  
  dbgDMPNL(dt->num_states);
  return dt;
}

dfa_tab_t* stateNFA::make_dfa_clean(unsigned int accept_id) {
  static bool dbg = zop.dbgBoolOption("dbg_make_dfa");
  assert(is_root()); //otherwise, the shortcut_links' logic needs to be revised
  DbgTime tm(string("stateNFA::make_dfa_clean.")+(toString(uid)));
  setup_front(FWD);
  //setup_frontiers(); //only need FWD, but use all for debugging ...
  
  vector<DfaState* > states;
  map<DfaState*, state_id_t, stateSetCmp> state_index;
  vector<state_id_t* > dfa_state_transitions;
  
  setPtrStateNFA entry;
  entry.insert(this);

  DfaState eps_closed;
  this->get_closure(eps_closed, NULL, FWD);  assert(!eps_closed.empty());
  DfaState *start = new DfaState(eps_closed);
  states.push_back(start); state_index[start] = states.size() - 1;
  
  tm.lap("before set construction");
  for (size_t i = 0; i < states.size(); ++i) {//states used as a queue!
    const DfaState& current = *states[i];
    
    state_id_t* current_transitions = new state_id_t[MAX_SYMS];
    for (unsigned int c = 0; c < MAX_SYMS; ++c) {
      state_id_t next_index;
      DfaState *ptr_next = new DfaState(); //just in case
      
      for (setPtrStateNFA::iterator it = current.begin();
	   it != current.end();
	   ++it) {
	stateNFA *s = *it;
	if ((  (s) == (s)->ends[FWD])     // proper end for this direction
	    && (s->Type()->matches(c))) { // and it's Type consumes 'b'
	  stateNFA *s_next = s->ends[BCK];
	  s_next->get_closure(*ptr_next, NULL, FWD);
	}
      }
      
      map<DfaState*, state_id_t, stateSetCmp>::iterator itnext;
      if ((itnext = state_index.find(ptr_next)) != state_index.end()) {
	next_index = itnext->second;
	assert(states[next_index] != ptr_next);
	delete ptr_next; ptr_next = NULL;
      } else {
	states.push_back(ptr_next);
	next_index = state_index[ptr_next] = states.size() - 1;
      }
      assert(next_index < states.size());
      current_transitions[c] = next_index;
    }
    dfa_state_transitions.push_back(current_transitions);
    assert(dfa_state_transitions.size() == i+1);
  }
#if 0
  {
    /*************************************************************************/
    /*              some map access timing/debugging code                    */
    /*************************************************************************/
    MyTime("stateNFA::make_dfa:timing/debugging");
    for(map<DfaState*, state_id_t, stateSetCmp>::iterator it = state_index.begin();
	it != state_index.end(); ++it) {
      const pair<DfaState*, state_id_t> &p = *it;
      const DfaState* pset = p.first;
      DfaState ds = *pset;
      assert(CONTAINS(state_index, &ds));
    }
  }
#endif
  tm.lap("after set construction");
  /***************************************************************************/
  /*                  now populate the dfa_tab_t                             */
  /***************************************************************************/
  assert(states.size() == dfa_state_transitions.size());
  dbgDMPNL(dfa_state_transitions.size());
  dfa_tab_t *dt = new dfa_tab_t();
  dt->num_states = states.size();
  dt->start = state_index[start];
  dbgDMPNL(dt->start);
  dt->tab = new unsigned int [MAX_SYMS*(dt->num_states)];
  for (state_id_t i=0; i < dt->num_states; i++) {
    for (unsigned int j=0; j < MAX_SYMS; j++) {
      assert(dfa_state_transitions[i][j] < dt->num_states);
      ARR(dt->tab, i, j) = dfa_state_transitions[i][j];
      if (dbg) cout << "DBG:STATES: i= " << i << " [ (int)j= " << j << " ] = " << ARR(dt->tab, i, j) << endl;
    }
  }
  dt->stat = NULL;
  dt->acc = new int *[dt->num_states];

  unsigned int dbg_num_accepting = 0;
  for (state_id_t i=0; i < dt->num_states; i++) {
    setPtrStateNFA &current = *states[i];
    //setPtrStateNFA out;
    if (dbg) dumpTContainer(current," current."+toString(i));
    if (CONTAINS(current, this->ends[BCK])) {
      if (dbg) cout << " IS ACCEPTING " << i << endl;
      dt->acc[i] = new int[2];
      dt->acc[i][0] = accept_id;
      dt->acc[i][1] = -1;
      ++dbg_num_accepting;
    } else
      dt->acc[i] = NULL;
  }
  tm.lap("after populate");
  assert(dbg_num_accepting > 0);
  /***************************************************************************/
  for (vector<DfaState*>::iterator it=states.begin(); it!=states.end(); ++it)
    delete *it;
  for (vector<state_id_t * >::iterator it =  dfa_state_transitions.begin();
       it != dfa_state_transitions.end();
       ++it)
    delete [](*it);
  tm.lap("after cleaning");

  dbgDMPNL(dt->num_states);
  if (dbg) dt->dfa2nfa2dotty(("dot.made-before-min."+toString(uid)).c_str());
  dt->minimize();
  tm.lap("after minimization");
  
  dbgDMPNL(dt->num_states);
  return dt;
}
#endif
///////////////////////////////////////////////////////////////////////////////
void stateNFA::get_closure(setPtrStateNFA&result, setPtrStateNFA*pseen,
			   direction dir) {
  std::vector<stateNFA*> &next = thompson_links[dir];
  setPtrStateNFA local_seen;
  if (!pseen)
    pseen = &local_seen;
  pseen->insert(this);
  result.insert(this);
  if (is_matching(dir))
    return;
  for(std::vector<stateNFA*>::iterator it= next.begin(); it!= next.end(); ++it) {
    if (!CONTAINS(*pseen, *it)) {
      stateNFA *s = *it;
      s->get_closure(result, pseen, dir);
    }
  }  
}

void gather_thompson_nodes(stateNFA* ps, search_scope &ss) {
  if (ss.keep_exploring(ps)) {
    if (ss.fwd()) {
      for(std::vector<stateNFA*>::iterator it= ps->thompson_links[stateNFA::FWD].begin(); it!= ps->thompson_links[stateNFA::FWD].end(); ++it) {
	stateNFA *s = *it;
	gather_thompson_nodes(s, ss);
      }
    }
    if (ss.bck()) {
      for(std::vector<stateNFA*>::iterator it= ps->thompson_links[stateNFA::BCK].begin(); it!= ps->thompson_links[stateNFA::BCK].end(); ++it) {
	stateNFA *s = *it;
	gather_thompson_nodes(s, ss);
      }
    }
  }
}

void gather_eps_free_nodes(stateNFA* ps, search_scope &ss) {
  if (ss.keep_exploring(ps)) {
    if (ss.fwd()) {
      for(std::vector<stateNFA*>::iterator it= ps->eps_free_links[stateNFA::FWD].begin(); it!= ps->eps_free_links[stateNFA::FWD].end(); ++it) {
	stateNFA *s = *it;
	gather_eps_free_nodes(s, ss);
      }
    }
    if (ss.bck()) {
      for(std::vector<stateNFA*>::iterator it= ps->eps_free_links[stateNFA::BCK].begin(); it!= ps->eps_free_links[stateNFA::BCK].end(); ++it) {
	stateNFA *s = *it;
	gather_eps_free_nodes(s, ss);
      }
    }
  }
}

#if 0
void gather_shortcut_nodes(stateNFA* ps, search_scope &ss) {
  if (ss.keep_exploring(ps)) {
    if (ss.fwd()) {
      for(std::vector<stateNFA*>::iterator it= ps->shortcut_links[stateNFA::FWD].begin(); it!= ps->shortcut_links[stateNFA::FWD].end(); ++it) {
	stateNFA *s = *it;
	gather_shortcut_nodes(s, ss);
      }
    }
    if (ss.bck()) {
      for(std::vector<stateNFA*>::iterator it= ps->shortcut_links[stateNFA::BCK].begin(); it!= ps->shortcut_links[stateNFA::BCK].end(); ++it) {
	stateNFA *s = *it;
	gather_shortcut_nodes(s, ss);
      }
    }
  }
}
#endif

void index_nodes(stateNFA* ps, vector<stateNFA*> &indexed) {
  assert(ps->is_root()); // otherwise need to use a different search_scope
  search_scope ss;
  gather_thompson_nodes(ps, ss);
  setPtrStateNFA &all_nodes = ss.get_seen();
  indexed.clear();
  indexed.resize(all_nodes.size());
  unsigned int ix = 0;
  for (setPtrStateNFA:: iterator it =  all_nodes.begin();
       it !=  all_nodes.end();
       ++it, ++ix) {
    stateNFA *s = (*it);
    assert(s->id == -1); // so it is indexed in only one place
    s->id = ix;
    indexed[ix] = s;
  }
  assert(ix == all_nodes.size());
}


// Beware! this destroys the entire graph
void stateNFA::delete_all_reachable(stateNFA* s) {
  search_scope ss;
  gather_thompson_nodes(s, ss);
  setPtrStateNFA &all_nodes = ss.get_seen();
  for (setPtrStateNFA:: iterator it =  all_nodes.begin();
       it !=  all_nodes.end();
       ++it) {
    delete (*it);
  }
}


/* This assumes that the 'target' is in epsilon-closure of 'this' state.
   There may be an infinite number of paths, if there is an epsilon* in between,
   but we return the unique path that doesn't loop through any epsilon*.
   'this' should not be a matching state (unless it is the target), otherwise
   it will be mistaken for a state on the border of the closure, and no path
   is found.
   If the 'source' must be a matching state, pick it's other to start the search
 */
bool stateNFA::find_path(stateNFA* target,
			 std::list<stateNFA*>& path,
			 setPtrStateNFA* pseen,
			 direction dir) {
  std::vector<stateNFA*> &next = thompson_links[dir];
  setPtrStateNFA local_seen;
  if (!pseen)
    pseen = &local_seen;
  pseen->insert(this);
  if (this == target) {
    path.push_front(this);
    return true;
  }
  if (!is_matching(dir)) {
    for(std::vector<stateNFA*>::iterator it= next.begin(); it!= next.end(); ++it) {
      if (!CONTAINS(*pseen, *it)) {
	stateNFA *s = *it;
	if (s->find_path(target, path, pseen, dir)) {
	  path.push_front(this);
	  return true;
	}
      }
    }
  }
  return false;
}

bool stateNFA::is_matching(direction dir) {//holds for either ends of trex_byte
  return (this == ends[dir]) && NULL != (dynamic_cast<trex_byte*>(Type()));
}

/* The automaton is in one of the states in 'state_set', and 'b' is the current
   input byte. Find the next possible set of states after the automaton consumes
   'b' while performing a match in direction 'dir'.
   The transitions are only performed for trex_byte types, which are of the form
   ends[FWD] --- b ---> ends[BCK], where ends[FWD] is equivalent to the START
   node in a Thomson construction, and ends[BCK} is the ACCEPTS state; but only
   if 'dir' is FWD, otherwise the roles are reversed.
*/
void transition(setPtrStateNFA &state_set,
		unsigned char b,
		setPtrStateNFA &eps_closed_result,
		stateNFA::direction dir, type_filter& tf) {
  eps_closed_result.clear();
  stateNFA::direction other_dir = (dir==stateNFA::FWD)? stateNFA::BCK:stateNFA::FWD;
  tf.clear_candidates();
  for (setPtrStateNFA::iterator it = state_set.begin();
       it != state_set.end();  ++it) {
    trex *trx = (*it)->Type();
    if ((   (*it) == (*it)->ends[dir])   // proper end for this direction
	&& (trx->matches(b)))      // and it's Type consumes 'b'
      {
        //DMP((*it)->uid);DMP(dir);DMP((*it)->thompson_links[dir].size());
      /* The following assertion should hold for Thomson NFA. However, for
	 performance reasons, we don't use Thomson for expressions with R{l,h}
	 See: term::to_trex() ...  case quantifier::RANGE:
	 If zop.dbgBoolOption("thompson_expand_range") then the assertion should hold
      */
	//assert((*it)->thompson_links[dir].size() == 1);           // Should use this...
	assert(((*it)->thompson_links[dir])[0]->Type() == (*it)->Type()); // Should use this...
	assert(((*it)->thompson_links[dir])[0] == (*it)->ends[other_dir]); // Should use this...
	tf.add_candidate(*it);
      }
  }
  setPtrStateNFA &candidates =  tf.get_candidates();
  for (setPtrStateNFA::iterator it = candidates.begin();
       it != candidates.end();  ++it) {
    (*it)->ends[other_dir]->get_closure(eps_closed_result, NULL, dir);
  }
}

/*
  This can perform matching both ways: forward, and backward
  (was 'match_all': removed specialized match/match_bck after svn v.734)  
*/
bool stateNFA::match(const char *buf, unsigned int len, direction dir) {
  permisive_filter dummy;
  return match(buf, len, dir, dummy);
}

bool stateNFA::match(const char *buf, unsigned int len, direction dir, type_filter& tf) {
  //MyTime tm(string("match-")+((dir==FWD)? "FWD":"BCK"));
  direction other_dir = (dir==FWD)? BCK:FWD;
  int delta = (dir==FWD)? 1 : -1;
  int index = (dir==FWD)? 0 : len-1;
  assert(is_root());
  setPtrStateNFA eps_closed_current_set;
  ends[dir]->get_closure(eps_closed_current_set, NULL, dir);
#define DBG_MATCH 0
#if DBG_MATCH
    dumpTContainer(eps_closed_current_set," Initial eps_closed_current_set");
#endif
  unsigned int i;
  for (i=0; i < len && !eps_closed_current_set.empty(); ++i, index+=delta) {
    setPtrStateNFA eps_closed_result;
    tf.set_index(index);
    transition(eps_closed_current_set, buf[index], eps_closed_result, dir, tf);
    eps_closed_current_set = eps_closed_result;

#if DBG_MATCH
    std::cout << "\nend of iteration " << i << " ";
    dumpTContainer(eps_closed_current_set," eps_closed_current_set");
#endif
  }
  bool match_res = CONTAINS(eps_closed_current_set, ends[other_dir]);
  //dbgDMP(match_res) << endl;
  return match_res;
}

parseTreeTerminal::parseTreeTerminal(trex* type0, t_term b0) :
  parseTree(type0), b(b0) {
  trex_byte *trx = dynamic_cast<trex_byte*>(_type); assert(trx);
  flat = output_char2string(b0);
  flat_length = 1;
  rule_index = trx->rank(this->b);//not really needed
}
// return the number of parseTree's that precede 'this', with the same flat_length
MPInt parseTreeTerminal::rank() {
  switch(_type->rk) {
  case rk_byte:
  case rk_byte_set:
  {
    trex_byte *trx = dynamic_cast<trex_byte*>(_type); assert(trx);
    return trx->rank(this->b);
  }
  default: assert(0);
  }
  return 0;
}

parseTree* path2parseTree(std::list<stateNFA*> &path,
			  const char *buf, unsigned int len) {
  vector<parseTree*> pt_stack;
  unsigned int buf_index = 0;
  for (std::list<stateNFA*>::iterator it = path.begin();
       it != path.end(); ++it) {
    stateNFA *snfa = *it;
    if (snfa == snfa->Exit()) {
      trex *type = snfa->Type();
      parseTree *current = NULL;
      switch(type->rk) {
      case rk_epsilon: current = new parseTree(type); break;
      case rk_byte:
      case rk_byte_set:
	assert(buf_index < len);
	current = new parseTreeTerminal(type, buf[buf_index++]);
	break;
      case rk_disjunction: {
	current = new parseTree(type);
	assert(!pt_stack.empty());
	parseTree *choice_child = pt_stack.back(); pt_stack.pop_back();
	assert(choice_child);
	int rule_index = type->child_index(choice_child->_type);
	assert(   0 <= rule_index
	       && rule_index <= 1 /* currently:  2-child disjunctions */);
	current->rule_index = rule_index;
	current->add_child(choice_child);
	current->flat = choice_child->flat;
	current->flat_length = choice_child->flat_length;
	break;
      }
      case rk_conjunction: {
	current = new parseTree(type);
	assert(!pt_stack.empty());
	int num_children = type->count_pt_children();
	assert(num_children == 2);  /* currently:  2-child conjunctions */
	current->children.resize(num_children);
	for (int child_index = num_children - 1; child_index >= 0; --child_index) {
	  assert(!pt_stack.empty());
	  parseTree *child = pt_stack.back(); pt_stack.pop_back();
	  assert(child);
	  current->add_child(child, child_index);
	  current->flat = child->flat + current->flat;
	  current->flat_length += child->flat_length;
	}
	break;
      }
      case rk_plus: {
        /* We consider an implicit rule ordering as follows
	   (assuming that !t->generates_epsilon() )
	   (0) t+ -> t
	   (1) t+ -> t t+
	   It is a bad idea to have t* as a type.
	   To understand why, consider the following rule ordering, if we used t*
	   (0) t* -> epsilon
	   (1) t* -> t
	   (2) t* -> t t*
	   TBD: CLARIFY THIS BETTER:
	   Then the trees #1(t,NULL) and #2(t,#0(epsilon)) would produce the same string.
	   To avoid confusion, we would prohibit #2(t,#0(epsilon)), but this messes the cleanness
	   of the type.
	   
	   to_trex translates t* as (epsilon|t+)
	*/
	current = new parseTree(type);
	trex_plus *type_plus = dynamic_cast<trex_plus*>(type); assert(type_plus);
	const trex *tr = type_plus->get_tr(); assert(tr);
	assert(!pt_stack.empty() && pt_stack.back()->_type == tr);
	while (!pt_stack.empty() && pt_stack.back()->_type == tr) {
	  parseTree *child = pt_stack.back(); pt_stack.pop_back();
	  assert(child);
	  if (current->children.empty()) { // Not sure if this case should be different from the following one
   	    current->rule_index = 0;
	    current->add_child(child);
	    current->flat = child->flat;
	    current->flat_length = child->flat_length;
	  } else {
	    parseTree *new_current = new parseTree(type); // (child, current)
	    new_current->add_child(child);
	    new_current->add_child(current);
	    new_current->flat = child->flat + current->flat;
	    new_current->flat_length = child->flat_length + current->flat_length;
   	    new_current->rule_index = 1;
	    current = new_current;
	  }
	}
	break;
      }
	
      default: assert(0);
      }
      assert(current);
      pt_stack.push_back(current);
    }
  }
  assert(path.empty() || (pt_stack.size() == 1));
  assert(path.empty() || buf_index == len);
  if (!pt_stack.empty())
    return pt_stack.front();
  return NULL;
}

// return the number of parseTree's that precede 'this', with the same flat_length
MPInt parseTree::rank() {
  MPInt rank_previous_rules = 0;
  MPInt rank_this_rule = 0;
  switch(_type->rk) {
  case rk_epsilon: return 0; break;
  case rk_byte:
  case rk_byte_set:
    assert(0 && "Should be handled in parseTreeTerminal::rank"); break;
  case rk_disjunction: {
    int num_children = _type->count_children();
    assert(num_children == 2); assert(rule_index < 2); /* currently:  2-child disjunctions */
    for (int child_index = 0;
	 child_index < num_children && child_index < rule_index;
	 ++child_index) {
      trex *child = _type->child(child_index);
      rank_previous_rules += child->num_matches(flat_length);
    }
    assert(children.size() == 1);
    rank_this_rule = children[0]->rank();
    return rank_previous_rules + rank_this_rule;
  }
    break;
  case rk_conjunction: {
    int num_children = _type->count_children();
    assert(num_children == 2);  /* currently:  2-child conjunctions */
    assert(rule_index == 0);
    assert(children.size() == 2);
    MPInt num0_shorter = 0;
    unsigned int l0 = children[0]->flat_length;
    for (unsigned int i = 0; i < l0; ++i) {
      MPInt n0 = children[0]->_type->num_matches(i);
      MPInt n1 = children[1]->_type->num_matches(flat_length-i);
      num0_shorter += n0 * n1;
    }
    
    MPInt rc0 = children[0]->rank();
    MPInt nm_c1 = children[1]->_type->num_matches(children[1]->flat_length);
    MPInt rc1 = children[1]->rank();
    rank_this_rule = rc0 * nm_c1 + rc1;
    //return rank_previous_rules + rank_this_rule;
    return rank_previous_rules + num0_shorter + rank_this_rule;
  }
    break;
  case rk_plus:
    /* We consider an implicit rule ordering as follows
       (0) t* -> epsilon
       (1) t* -> t
       (2) t* -> t t*
       ////////////
       TBD: A better/cleaner way to think about this would be to have
       (assuming that !t->generates_epsilon() )
       t* -> epsilon
       t* -> t t+
       t+ -> t
       t+ -> t t+
       TBD: In fact we should not even have t* as a type!
            to_trex should replace it with (epsilon|t+)
    */
    switch (rule_index) {
    case 0: assert(children.size() == 1); assert(!children[0]->_type->generates_epsilon());
      rank_this_rule = children[0]->rank();
      return rank_this_rule;
      break;
    case 1:
      assert(children.size() == 2);
      
      MPInt num0_shorter = 0;
      unsigned int l0 = children[0]->flat_length;
      assert(!children[0]->_type->generates_epsilon());
      assert(0 == children[0]->_type->num_matches(0));
      for (unsigned int i = 0; i < l0; ++i) {
	MPInt n0 = children[0]->_type->num_matches(i);
	MPInt n1 = children[1]->_type->num_matches(flat_length-i);
	num0_shorter += n0 * n1;
      }
      num0_shorter += children[0]->_type->num_matches(flat_length);// for case 1: T*->T
      
      MPInt rc0 = children[0]->rank();
      MPInt nm_c1 = children[1]->_type->num_matches(children[1]->flat_length);
      MPInt rc1 = children[1]->rank();
      rank_this_rule = rc0 * nm_c1 + rc1;
      //return rank_previous_rules + rank_this_rule;
      return rank_previous_rules + num0_shorter + rank_this_rule;
      break;
    }
    break;
  default: assert(0);
  }
  return 0;
}

void parseTree::get_raw_accept(std::string &res) {
  for(std::vector<parseTree*>::iterator it= children.begin();
      it!= children.end();
      ++it) {
    (*it)->get_raw_accept(res);
  }
}

/*
  A tree type is a conjunction of the form T0T1, where T0/T1's type is c0/c1
  The tree has 'rank' and 'len'. Find the (flatten) lengths len0/len1 of T0/T1
*/
void find_conjunction_division(const MPInt &rank,
			       trex *c0, unsigned int &len0,
			       trex *c1, unsigned int &len1,
			       MPInt &num0_shorter,unsigned int len) {
    // Find (l0,l1)
    num0_shorter = 0;
    unsigned int i;
    for (i = 0; i <= len; ++i) {//TBD:perf: use binary search
      MPInt n0 = c0->num_matches(i);
      MPInt n1 = c1->num_matches(len-i);
      MPInt next_num0_shorter = num0_shorter + n0 * n1;
      /* if next_num0_shorter is unchanged for some 'i', then l0 cannot be that
	 'i' because there are no matches that decompose as (i, flat_length-i)
      */
      if (next_num0_shorter > rank)
	break;
      num0_shorter = next_num0_shorter;
      //if (next_num0_shorter == rank) break;
    }
    assert(i<=len);
    len0 = i;
    len1 = len-i;
}

parseTree* pt_unrank(MPInt& rank, trex *type, unsigned int len) {
  parseTree *current = NULL;
  switch(type->rk) {
  case rk_epsilon: assert(len == 0); return new parseTree(type); break;
  case rk_byte: assert(rank == 0);
  case rk_byte_set: assert(len == 1);
    return new parseTreeTerminal(type, (dynamic_cast<trex_byte*>(type))->unrank(rank));
    break;
  case rk_disjunction: {
    int num_children = type->count_children();
    trex *child_type = NULL;
    assert(num_children == 2); /* currently:  2-child disjunctions */
    int child_index;
    for (child_index = 0; child_index < num_children; ++child_index) {
      trex *child_candidate = type->child(child_index);
      MPInt max_rank_candidate = child_candidate->num_matches(len);
      child_type = child_candidate;
      if (max_rank_candidate > rank)
	break;
      rank -= max_rank_candidate;
      //if (rank == 0) break;
    }
    assert(child_index < num_children);//must have found a path/child
    assert(child_type);
    
    parseTree *child = pt_unrank(rank, child_type, len);
    current = new parseTree(type);
    current->rule_index = child_index;
    current->add_child(child);
    
    current->flat = child->flat;
    current->flat_length = child->flat_length;
    return current;
  }
    break;
  case rk_conjunction: {
    int num_children = type->count_children();
    assert(num_children == 2);  /* currently:  2-child conjunctions */
    trex *c0 = type->child(0);
    trex *c1 = type->child(1);
    MPInt num0_shorter = 0;
    unsigned int len0, len1;
    find_conjunction_division(rank, c0, len0, c1, len1, num0_shorter, len);
    
    MPInt adjusted_rank = rank - num0_shorter;
    MPInt rank0 = adjusted_rank / c1->num_matches(len1);
    MPInt rank1 = adjusted_rank % c1->num_matches(len1);
    
    parseTree *child0 = pt_unrank(rank0, c0, len0);
    parseTree *child1 = pt_unrank(rank1, c1, len1);
    
    current = new parseTree(type);
    current->add_child(child0);
    current->add_child(child1);
    current->flat = child0->flat + child1->flat;
    current->flat_length = child0->flat_length + child1->flat_length;
    return current;
  }
    break;
  case rk_plus: {
    assert(len != 0); // because !c0->generates_epsilon();
    trex_plus* ts = dynamic_cast<trex_plus*>(type);
    trex *c0 = ts->get_tr();
    trex *c1 = type;
    assert(!c0->generates_epsilon());// otherwise the logic must be revised
    // Check if in case 0: T+->T
    if (rank < c0->num_matches(len)) {
      parseTree *child0 = pt_unrank(rank, c0, len);
      current = new parseTree(type);
      current->add_child(child0);
      current->flat = child0->flat;
      current->flat_length = child0->flat_length;
      return current;
    }
    // Definitely case 1: T+->TT+
    // Find (l0,l1)
    rank -= c0->num_matches(len);
    MPInt num0_shorter = 0;
    assert(0 == c0->num_matches(0)); // otherwise the logic must be revised
    unsigned int len0, len1;
    find_conjunction_division(rank, c0, len0, c1, len1, num0_shorter, len);
    
    MPInt adjusted_rank = rank - num0_shorter;
    MPInt rank0 = adjusted_rank / c1->num_matches(len1);
    MPInt rank1 = adjusted_rank % c1->num_matches(len1);
    
    parseTree *child0 = pt_unrank(rank0, c0, len0);
    parseTree *child1 = pt_unrank(rank1, c1, len1);
    
    current = new parseTree(type);
    current->add_child(child0);
    current->add_child(child1);
    current->flat = child0->flat + child1->flat;
    current->flat_length = child0->flat_length + child1->flat_length;
    return current;
  }
  default: assert(0);
  }
}

parseTree* stateNFA::parse(const char *buf, unsigned int len) {
  DbgTime tm("parseTree* stateNFA::parse");
  parseTree* pt=NULL;
  assert(is_root());
  if(DBGFILTER) {cout << "RANKING/PARSING:input[" << len << "]='" << buf <<"' ... " << std::endl;}
  //match_filter mf(len);
  match_single_path_filter mf(this, len);
  bool m_bck =  match(buf, len, BCK, mf);
  //dbgDMPNL(m_bck);
  mf.no_record_mode();
  bool m_fwd =  match(buf, len, FWD, mf);
  //dbgDMPNL(m_fwd);
  //mf.check(buf, len, m_fwd);
  assert(m_fwd == m_bck);
  if (m_fwd) {
    pt = mf.getParseTree(buf, len, m_fwd);
    assert(pt);
    std::string res;
    pt->get_raw_accept(res);
    assert(res.length() == len);
    //assert(0 == strncmp(res.c_str(), buf, len));
    assert(0 == memcmp(res.c_str(), buf, len));
  }
  return pt;
}

#define DOT_PT REX_VERBOSE
bool string_rank(MPInt& rank, const char *buf, unsigned int len, stateNFA* snfa) {
  DbgTime tm("string_rank");
  rank = 0;
  bool success = false;
  rank = -1;
  parseTree* pt = snfa->parse(buf, len);
  if (pt) {
    if(DOT_PT){ std::ofstream of(("dot.pt.txt."+toString(pt->uid)).c_str());  pt->to_dot(of, true); of.close();}

    if (!pt->_type->problematic()) {
      success = true;
      DbgTime tm("ranking-and-unranking");
      rank = pt->rank(); if (DOT_PT) {dbgDMPNL(rank);}
      tm.lap("ranking-done");
#if 0
      MPInt rank2 = pt->rank();
      assert(rank2 == rank);
      tm.lap("2ranking-done");
      if(DOT_PT) {cout << "RANK of " << buf << " is:" << rank << endl;}
      {
        parseTree* upt = pt_unrank(rank2, snfa->Type(), len);
	tm.lap("unranking-done");
      
	if(DOT_PT){ std::ofstream ofu(("dot.upt.txt."+toString(pt->uid)).c_str()); upt->to_dot(ofu, true); ofu.close();
		cout << "UN-RANKed of (rank of)" << buf << " is:" << upt->flat << endl;}
	assert(upt->flat == pt->flat);
	delete upt;
      }
      rank2 = rank;
      {
        parseTree* upt = pt_unrank(rank2, snfa->Type(), len);
	tm.lap("unranking2-done");
      
	if(DOT_PT){ std::ofstream ofu(("dot.upt2.txt."+toString(pt->uid)).c_str());
		upt->to_dot(ofu, true); ofu.close();
		cout << "UN-RANKed2 of (rank of)" << buf << " is:" << upt->flat << endl;}
	assert(upt->flat == pt->flat);
	delete upt;
      }
      rank2 = rank;
      {
        std::string revert;
	bool ok = string_unrank(revert, rank2, snfa->Type(), len);
	assert(ok);
	dbgDMPNL(pt->flat);
	dbgDMPNL(revert);
	assert(revert == pt->flat);
      }
#endif
    } else {
      cout << "skipped (un)ranking for problematic type " << pt->_type->unparse() << endl;
    }
  }
  delete pt;
  return success;
}

bool string_unrank(std::string& s, MPInt& rank, trex *type, unsigned int len) {
  DbgTime tm("string_unrank");
  bool success = false;
  s.clear();
  parseTree* upt = pt_unrank(rank, type, len);
  if(DOT_PT){ std::ofstream of(("dot.upt.txt."+toString(upt->uid)).c_str());  upt->to_dot(of, true); of.close();}
  if (upt) {
    success = true;
    s = upt->flat;//performance issue, string copy needed because upt is discarded
    delete upt;
  }
  return success;
}

/************** rank_nfa_tab_t ******************/
rank_nfa_tab_t::rank_nfa_tab_t(const std::string &rex0,
			       size_t max_len)
  : ranker(rex0) {
  DbgTime tm("rank_nfa_tab_t::rank_nfa_tab_t");
  init();
  update_matches(max_len);
  
  return;
}

rank_nfa_tab_t::rank_nfa_tab_t(const std::string &rex0)
  : ranker(rex0) {
  DbgTime tm("rank_nfa_tab_t::rank_nfa_tab_t");

  init();
  
  return;
}

void rank_nfa_tab_t::init() {
  DbgTime tm("rank_nfa_tab_t::init");

  DbgTime tm_parse("rank_nfa_tab_t:parse", false);
  DbgTime tm_to_trex("rank_nfa_tab_t:to_trex", false);
  trex *trx = regex2trex(rex, &tm_parse, &tm_to_trex);
  
  DbgTime tm_toNFA("rank_nfa_tab_t:toNFA");
  local_snfa_root = snfa_root = trx->toNFA(NULL);
  tm_toNFA.stop();
  
  assert(snfa_root->is_root());
  
  return;
}

rank_nfa_tab_t::rank_nfa_tab_t(const string& rex0,
			       stateNFA* snfa0,
			       size_t max_len)
  : ranker(rex0), snfa_root(snfa0), local_snfa_root(NULL) {
  DbgTime tm("rank_nfa_tab_t::rank_nfa_tab_t");
  assert(snfa_root->is_root());
  update_matches(max_len);
  return;
}

void rank_nfa_tab_t::update_matches(unsigned int len) {
  static DbgTime tm_update_matches("STATIC:rank_nfa_tab_t:update_matches", false);
  DbgTime tm("rank_nfa_tab_t::update_matches."+toString(len));
  if (len < _memoized_size)
    return;
  tm_update_matches.resume();
  unsigned int low;
  unsigned int num_states;
  if (_memoized_size == 0) {
    _memoized_size = len+1;
    snfa_root->setup_eps_free_front(stateNFA::FWD);
    search_scope ss;
    gather_eps_free_nodes(snfa_root, ss);
    setPtrStateNFA &relevant_nodes = ss.get_seen();
    assert(CONTAINS(relevant_nodes, snfa_root));
    //////////////////////////
    setPtrStateNFA eps_closed_accept;
    snfa_root->ends[stateNFA::BCK]->get_closure(eps_closed_accept, NULL, stateNFA::BCK);
    //////////////////////////
    //nfa_nodes.resize(relevant_nodes.size());  unsigned int idx = 0;
    tab.clear(); tab.resize(relevant_nodes.size());
    nfa_nodes.clear();
    unsigned int idx = 0;
    for (setPtrStateNFA::iterator it = relevant_nodes.begin();
	 it != relevant_nodes.end();
	 ++it, ++idx) {
      stateNFA *s = *it;
      assert(idx == nfa_nodes.size());
      s->id = idx;
      nfa_nodes.push_back(s);
      tab[idx].resize(_memoized_size, 0);
      if (CONTAINS(eps_closed_accept, s))
	tab[idx][0] = 1;
    }
    low = 1;
    num_states = nfa_nodes.size();
  } else {
    low = _memoized_size;
    _memoized_size = len+1;
    num_states = nfa_nodes.size();
    for (state_id_t s = 0; s < num_states; ++ s)
      tab[s].resize(_memoized_size, 0);
  }
  
  start = snfa_root->id;
  assert(nfa_nodes[start] = snfa_root);
  
  for (size_t i = low; i < _memoized_size; ++i) {
    for (state_id_t s = 0; s < num_states; s++) {
      stateNFA * cur = nfa_nodes[s];
      for(std::vector<stateNFA*>::iterator it= cur->eps_free_links[stateNFA::FWD].begin(); it!= cur->eps_free_links[stateNFA::FWD].end(); ++it) {
	stateNFA *next = *it;
	state_id_t next_s = next->id;
	trex_byte *tb_next = dynamic_cast<trex_byte*>(next->Type());assert(tb_next);

	const std::vector<unsigned int>& chars = tb_next->get_chars();
#if 1
	tab[s][i] = tab[s][i] + chars.size() * tab[next_s][i-1];
#else	// maybe a few additions are faster than multiplication
	assert(chars.size() > 0);
	for (unsigned int c = 0; c < chars.size(); ++c)
	  tab[s][i] = tab[s][i] + tab[next_s][i-1];
#endif
      }
    }
  }
  tm_update_matches.stop();
}

bool rank_nfa_tab_t::rank(MPInt & pc, const char *str, size_t len) {
  static DbgTime tm_rank("STATIC:rank_nfa_tab_t::rank", false);
  static DbgTime tm_match_fwd("STATIC:rank_nfa_tab_t::match_fwd", false);
  static DbgTime tm_match_bck("STATIC:rank_nfa_tab_t::match_bck", false);
  tm_rank.resume();
  //MyTime tm("rank_nfa_tab_t::rank");
  pc = 0;
  //////////////////////////////////////////////////////////////////////
  match_single_path_filter mf(snfa_root, len);
  tm_match_bck.resume();
  bool m_bck =  snfa_root->match(str, len, stateNFA::BCK, mf);
  tm_match_bck.stop();
  //dbgDMPNL(m_bck);
  tm_match_fwd.resume();
  mf.no_record_mode();
  bool m_fwd =  snfa_root->match(str, len, stateNFA::FWD, mf);
  tm_match_fwd.stop();
  
  assert(m_fwd == m_bck);
  if (!m_fwd) {
    tm_rank.stop();
    return false;
  }
  vector <stateNFA*>& path = mf.get_trace();
  //////////////////////////////////////////////////////////////////////
  state_id_t s = start;
  //assert(len == strlen(str)); Incorrect, because str may contain \x00 
  assert(len < _memoized_size);
  assert(path.size() == len);
  
  for (size_t i = 0;      i < len;      ++i) {
    unsigned char c = str[i];
    stateNFA * cur = nfa_nodes[s];
    stateNFA * next = path[i]->ends[stateNFA::BCK];
    std::vector<stateNFA*>::iterator it;
    for(it = cur->eps_free_links[stateNFA::FWD].begin();
	it!= cur->eps_free_links[stateNFA::FWD].end();
	++it) {
      stateNFA *prec_next = *it;
      state_id_t prec_next_s = prec_next->id;
      assert(prec_next_s < nfa_nodes.size());
      trex_byte *tb_next = dynamic_cast<trex_byte*>(prec_next->Type());assert(tb_next);
      if (prec_next == next) {
	pc = pc + (tb_next->rank(c))* tab[prec_next_s][len - 1 -i];
	s = prec_next_s;
	break;
      }
      pc = pc + (tb_next->get_chars().size())* tab[prec_next_s][len - 1 -i];
    }
    assert(it!= cur->eps_free_links[stateNFA::FWD].end());
  }
  tm_rank.stop();
  return true;
}

bool rank_nfa_tab_t::unrank(MPInt& pc, size_t len, string& res) {
  static DbgTime tm_unrank("STATIC:rank_nfa_tab_t::unrank", false);
  if (pc > tab[start][len])
    return false;
  tm_unrank.resume();
  res.clear();
  
  state_id_t s = start;
  stateNFA * cur = nfa_nodes[s]; assert(cur = snfa_root);
  for (size_t i = 0;      i < len;      ++i) {
    assert(res.length() == i);
    std::vector<stateNFA*>::iterator it;
    unsigned int rest_len = len - 1 - i;
    for(it = cur->eps_free_links[stateNFA::FWD].begin();
	it!= cur->eps_free_links[stateNFA::FWD].end();
	++it) {
      stateNFA *candidate_next = *it;
      state_id_t candidate_next_s = candidate_next->id;
      trex_byte *tb_candidate = dynamic_cast<trex_byte*>(candidate_next->Type());assert(tb_candidate);
      const std::vector<unsigned int>& chars = tb_candidate->get_chars();
      MPInt candidate_paths = (chars.size())* tab[candidate_next_s][rest_len];
      if (pc < candidate_paths) {
	unsigned int c_rank;
	MPInt q = pc / tab[candidate_next_s][rest_len];
	pc = pc % tab[candidate_next_s][rest_len];//weird: mpz_tdiv_qr = no gains
	c_rank = MPInt_to_uint(q);
	assert(c_rank < chars.size());
	t_term c = tb_candidate->unrank(c_rank);/*unrank not needed:see chars*/
	res.push_back(c);
	cur = candidate_next;
	break;
      }
      pc = pc - candidate_paths;
    }
    assert(it!= cur->eps_free_links[stateNFA::FWD].end());
  }
  assert(res.length() == len);
  tm_unrank.stop();
  return true;
}

void rank_nfa_tab_t::dump_tab() {
  dbgDMP(_memoized_size); dbgDMP(start); dbgDMPNL(nfa_nodes.size());

  setPtrStateNFA eps_closed_accept;
  snfa_root->ends[stateNFA::BCK]->get_closure(eps_closed_accept, NULL, stateNFA::BCK);
  for (state_id_t s = 0; s < nfa_nodes.size(); s++) {
    stateNFA * cur = nfa_nodes[s];    
    cout << " state(" << s << " uid=" << (cur->uid)
	 << ", acc?=" << (CONTAINS(eps_closed_accept, cur));
    trex_byte *tb_cur = dynamic_cast<trex_byte*>(cur->Type());
    if (tb_cur)
      cout << ":" << tb_cur->unparse();
    cout << ")";
    for (size_t l = 0; l < _memoized_size; ++l) {
      cout << " [" << l << "]=" << tab[s][l];
    }
    cout << endl;
  }
}

void rank_nfa_tab_t::report_size(const char* msg, int verbose_level) {
  search_scope ss;
  gather_thompson_nodes(snfa_root, ss);
  unsigned int num_thompson_states = ss.get_seen().size();

  unsigned int num_eps_closed_states = nfa_nodes.size();
  unsigned int tab_entries = num_eps_closed_states * _memoized_size;
  unsigned int nz_entries = 0;
  size_t bits = 0;

  cout << "rank_nfa_tab_t::report_size:" << msg
       << " : num_thompson_states= " << num_thompson_states
       << " : num_eps_closed_states= " << num_eps_closed_states
       << " : tab_entries= " << tab_entries;

  if (verbose_level >= 2) {
    bits = get_memoized_bits(&nz_entries);
    cout << " : nz_entries= " << nz_entries
	 << " : bits= " << bits;
  }
  cout << endl;
}

size_t rank_nfa_tab_t::get_memoized_bits(unsigned int *nz_entries) const {
  if (nz_entries) *nz_entries=0;
  unsigned int num_eps_closed_states = nfa_nodes.size();
  size_t bits = 0;
  for (state_id_t s = 0; s < num_eps_closed_states; s++) {
    for (size_t i = 0; i < _memoized_size; ++i) {
      bits += mpz_sizeinbase (tab[s][i].get_mpz_t(), 2);
	  if (nz_entries && tab[s][i] != 0) ++(*nz_entries);
	}
  }
  return bits;
}

void rank_nfa_tab_t::dotty(const char *dot_name) {
  ofstream dotsNFA(dot_name);
  snfa_root->to_dot(dotsNFA);
  dotsNFA.close();
}

//////////////////////////////////////////////////////////////////////
/************************************************/
/*              DEBUGGING STUFF                 */
/************************************************/
void stateNFA::to_dot(std::ostream& dio, setPtrStateNFA*seen) {
  bool first=false;
  if (seen == NULL) {
    dio << "digraph stateNFA {\n";
    first = true;
    seen = new setPtrStateNFA;
  }
  assert(seen && !CONTAINS(*seen, this));
  seen->insert(this);

  ////////////// label
  stringstream label;
  //dio << uid << "[label=\"" << uid << " "; // start label ....
  label << uid << " ";
  if (_dir == FWD) {
    switch(Type()->rk) {
    case rk_epsilon: label << "{epsilon} "; break;
    case rk_byte:
    {
      //trex_byte *tb = dynamic_cast<trex_byte*>(Type());
      label << "!" << Type()->unparse() << " ";
      dio << uid << "[shape=box]\n";
    }
    break;
   case rk_byte_set:
    {
      label << "." << Type()->unparse() << " ";
      dio << uid << "[shape=box]\n";
    }
    break;    
    case rk_disjunction: label << "| " << Type()->unparse() << " "; break;
    case rk_conjunction: label << "& " << Type()->unparse() << " "; break;
    case rk_range: label << "{}" << Type()->unparse() << " "; break;
    case rk_plus: label << "+ " << Type()->unparse() << " "; break;
    default: assert(0);
    }
  } else {
    label << "[" << ends[FWD]->uid << "] ";
  }
  
  label << "t(" << Type()->uid << ") ";
  dio << uid << "[color=red]\n";
  if (this == Exit())
    dio << uid << "[style=dotted]\n";
  if (ends[FWD]->is_root())
    dio << uid << "[shape=doubleoctagon]\n";
  dio << uid << "[label=\"" << label.str() << "\"]";
  ////////////// edges
  int i_link=0;
  for(std::vector<stateNFA*>::iterator it= thompson_links[FWD].begin();
      it!= thompson_links[FWD].end();
      ++it) {
    dio << uid << " -> " << (*it)->uid << "[color=red,style=dotted,label=" << (i_link++) <<"]\n";
    if (!CONTAINS(*seen, *it))
      (*it)->to_dot(dio, seen);
  }
  for(std::vector<stateNFA*>::iterator it= thompson_links[BCK].begin(); it!= thompson_links[BCK].end(); ++it) {
    //if (*it)
    dio << uid << " -> " << (*it)->uid << "[color=blue,style=dotted]\n";
  }
  //////////////////////////////////////////////////////////////////////
  for(std::vector<stateNFA*>::iterator it= shortcut_links[FWD].begin();
      it!= shortcut_links[FWD].end();
      ++it) {
    dio << uid << " -> " << (*it)->uid << "[color=black,style=dotted,label=fwd]\n";
  }
  for(std::vector<stateNFA*>::iterator it= shortcut_links[BCK].begin();
      it!= shortcut_links[BCK].end();
      ++it) {
    dio << uid << " -> " << (*it)->uid << "[color=black,style=dotted,label=bck]\n";
  }
  //////////////////////////////////////////////////////////////////////
  for(std::vector<stateNFA*>::iterator it= eps_free_links[FWD].begin();
      it!= eps_free_links[FWD].end();
      ++it) {
    dio << uid << " -> " << (*it)->uid << "[color=black,style=dotted,label=eFwd]\n";
  }
  for(std::vector<stateNFA*>::iterator it= eps_free_links[BCK].begin();
      it!= eps_free_links[BCK].end();
      ++it) {
    dio << uid << " -> " << (*it)->uid << "[color=black,style=dotted,label=eBck]\n";
  }
  //////////////////////////////////////////////////////////////////////
  
  
  if (first) {
    delete seen;
    dio << "}\n";
  }
}

void dbg_dot(stateNFA *s, const char *file) {
  ofstream dotsNFA (file?file:("dbg"+toString(s->uid)).c_str());
  s->to_dot(dotsNFA); dotsNFA.close();
}

void dotty_path(std::list<stateNFA*> &path, std::ostream& dio) {
  stateNFA* prev = NULL;
  dio << "digraph path {\n";
  unsigned int i = 0;
  for (std::list<stateNFA*>::iterator it = path.begin();
       it != path.end();
       ++it, ++i) {
    if (prev) {
      dio << prev->uid << " -> " << (*it)->uid << " [label=" << (i++) << "]\n";
    }
    prev = *it;
  }
  dio << "}\n";
}

void parseTree::to_dot(std::ostream& dio, bool first) {
  if (first)
    dio << "digraph parseTree {\n";

  ////////////// label
  stringstream label;
  label << uid
	<< " #" << rule_index
	<< " t(" << _type->uid << ")"
	<< _type->unparse() << "\\n"
	<< flat_length << ":'" << flat << "'";
  dio << uid << "[label=\"" << label.str() << "\"]";
  ////////////// edges
  for(std::vector<parseTree*>::iterator it= children.begin();
      it!= children.end();
      ++it) {
    int rule_index = _type->child_index((*it)->_type);
    dio << uid << " -> " << (*it)->uid << "[color=blue, label=" << (rule_index) << "]\n";
    (*it)->to_dot(dio, false);
  }
  if (first)
    dio << "}\n";
}

#if 0
void parseTree::to_path(stateNFA* start, std::list<stateNFA*> &path) {
  assert(0);
  path->push_back(start);
  stateNFA* first = start->thompson_links[FWD][rule_index];
  stateNFA* last = start = start->thompson_links[FWD][rule_index];
  for(std::vector<parseTree*>::iterator it= children.begin();
      it!= children.end();
      ++it) {
    (*it)->to_path(path);
  }
}
#endif

void pMPI(MPInt *pmpi) {
  cout << (*pmpi) << endl;
}
void pMPIv(MPInt mpi) {
  cout << (mpi) << endl;
}

void dmpS(setPtrStateNFA* pc) {
  dumpTContainer(*pc,"");
}

void dmpV(std::vector<stateNFA*>* pc) {
  dumpTContainer(*pc,"");
}
