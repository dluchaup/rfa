/*-----------------------------------------------------------------------------
 * File:    tnfa.h
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
#ifndef TNFA_H
#define TNFA_H
#include <string>
#include <iostream>
#include <vector>
#include <set>
#include <list>

#include "trex.h"
#include "utils.h"
#include "MyTime.h"

class trex;
class trex_byte;
class type_filter;
class parseTree;
class dfa_tab_t;
class search_scope;
typedef std::set<class stateNFA*,uidObject::cmpUidObject> setPtrStateNFA;

/********** stateNFA  ***************************/
class stateNFA: public uidObject {
private:
  virtual ~stateNFA();
  public:
  // Beware! this destroys the entire graph
  static void delete_all_reachable(stateNFA*);
public:// TBD: MAKE THIS PRIVATE
  trex* _type;
  enum direction {FWD=0, BCK=1, NUM_DIRECTIONS=2} _dir;
  std::vector<stateNFA*> thompson_links[NUM_DIRECTIONS];
  std::vector<stateNFA*> shortcut_links[NUM_DIRECTIONS];
  std::vector<stateNFA*> eps_free_links[NUM_DIRECTIONS];
  stateNFA *ends[NUM_DIRECTIONS];
  
  stateNFA(trex* type0, direction dir0):_type(type0), _dir(dir0) {
    assert(_type);
    assert(_dir == FWD || _dir == BCK);
    ends[FWD] = ends[BCK] = NULL;
    ends[_dir] = this;
  }
    
  void add_fwd_transition(stateNFA* sn) {
    //assert(!CONTAINS(thompson_links[FWD], sn));//paranoia
    thompson_links[FWD].push_back(sn);
    sn->add_bck_transition(this);
  }
  void add_bck_transition(stateNFA* sn) {
    //assert(!CONTAINS(thompson_links[FWD], sn));//paranoia
    thompson_links[BCK].push_back(sn);
  }
  
  inline void add_shortcut(direction dir, stateNFA* target) {
    assert(!SEQCONTAINS(shortcut_links[dir], target));
    shortcut_links[dir].push_back(target);
  }
  
  inline void add_eps_free_shortcut(direction dir, stateNFA* target) {
    assert(!SEQCONTAINS(eps_free_links[dir], target));
    eps_free_links[dir].push_back(target);
  }
  
  trex* Type()     {  return _type;}
  stateNFA* Exit() {  return ends[BCK];}

  stateNFA* other_end() {return ends[(_dir==FWD)? BCK:FWD];}

  bool match(const char *buf, unsigned int len, direction dir);
  parseTree* parse(const char *buf, unsigned int len);

  void to_dot(std::ostream& dio, setPtrStateNFA*seen=NULL);
  void show_frontier(std::ostream& dio);
  
  //TBD: get rid of the following !!!!!!!!!
  bool is_root() { return (this == ends[FWD]) && Exit()->thompson_links[FWD].empty(); }
  bool find_path(stateNFA* target,
		 std::list<stateNFA*>& path,
		 setPtrStateNFA* pseen,
		 direction dir);
  void setup_frontiers() {
    MyTime tm("setup_frontiers");
    setup_front(stateNFA::FWD);
    setup_front(stateNFA::BCK);
  }
  void get_closure(setPtrStateNFA& result, setPtrStateNFA* pseen,direction dir);

  dfa_tab_t* make_dfa_prev(unsigned int accept_id = 1);
  // if(dt == NULL) {dt = new ...}; fill dt; return dt;
  dfa_tab_t* make_dfa(unsigned int accept_id = 1, dfa_tab_t*pdt=NULL);
  dfa_tab_t* make_dfa_clean(unsigned int accept_id = 1);
  virtual bool is_matching(direction dir);
  void setup_eps_free_front(direction dir);
  bool match(const char *buf, unsigned int len, direction dir, type_filter& tf);

  static bool dbg_reset_stateNFA_guid;

protected:
  void link_shortcuts(direction dir);
  void link_eps_free_shortcuts(direction dir);
  void setup_front(direction dir);
};
/************************************************/
/********** parseTree ***************************/

class parseTree : public uidObject {
  parseTree();
public:
  std::string flat; //shows human readable version of the string at the leaves
  unsigned int flat_length;
  virtual void get_raw_accept(std::string &res); //similar to flat, but 'raw'
public:
  trex* _type;
  int rule_index;
  std::vector<parseTree*> children;
  parseTree(trex* type0) :
    flat(""), flat_length(0),
    _type(type0), rule_index(0) { assert(_type); };
  virtual ~parseTree() {
    for (std::vector<parseTree*>::iterator it = children.begin();
	 it != children.end(); ++it)
      delete *it;
  }
  void add_child(parseTree* child, int child_index)  {
    assert(0 <= child_index && (unsigned int)child_index < children.size());
    assert(children[child_index] == NULL);
    children[child_index] = child;
  }
  void add_child(parseTree* pt)  { assert(pt);
    children.push_back(pt);
  }

  void to_dot(std::ostream& dio, bool first);
  void to_path(stateNFA* start, std::list<stateNFA*> &path);

  virtual MPInt rank();
};

/********** parseTreeTerminal *******************/
class parseTreeTerminal : public parseTree {
  t_term b;
  public:
  parseTreeTerminal(trex* type0, t_term b0);
  virtual MPInt rank();
  virtual void get_raw_accept(std::string &res) { res.push_back(b); }
};

parseTree* path2parseTree(std::list<stateNFA*> &path, const char *buf, unsigned int len);
parseTree* pt_unrank(MPInt& rank, trex *type, unsigned int len);
void dotty_path(std::list<stateNFA*> &path, std::ostream& dio);
bool string_rank(MPInt& rank, const char *buf, unsigned int len, stateNFA* snfa);
bool string_unrank(std::string& s, MPInt& rank, trex *type, unsigned int len);
void transition(setPtrStateNFA &state_set,
		unsigned char b,
		setPtrStateNFA &eps_closed_result,
		stateNFA::direction dir,
		type_filter& tf);
void gather_thompson_nodes(stateNFA* ps, search_scope &ss);
void index_nodes(stateNFA* ps, std::vector<stateNFA*> &indexed);
void get_matching(setPtrStateNFA& result,
		  setPtrStateNFA& state_set,
		  stateNFA::direction dir);

/************************************************/
/* filters are used to gather reachable sets    */
/********** type_filter *************************/
class search_scope {
protected:
  std::set<stateNFA*,uidObject::cmpUidObject> _seen;
public:
  virtual bool keep_exploring(stateNFA* snfa) {
    if (!CONTAINS(_seen, snfa)) {
      _seen.insert(snfa);
      return true;
    }
    return false;
  }
  std::set<stateNFA*,uidObject::cmpUidObject>& get_seen() {
    return _seen;
  }
  bool fwd() { return true; }
  bool bck() { return true; }  
};

void gather_eps_free_nodes(stateNFA* ps, search_scope &ss);
#endif //TNFA_H
