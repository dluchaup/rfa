/*-----------------------------------------------------------------------------
 * File:    trex.h
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
#ifndef TREX_H
#define TREX_H
#include <assert.h>
#include <string>
#include <iostream>
#include <vector>
#include <set>
#include <bitset>
#include "uidObject.h"

#include "alphabet_byte.h"
#include "MPInt.h"

class stateNFA;

class trex;
class trex_epsilon;
class trex_byte;
class trex_byte_set;
class trex_disjunction;
class trex_conjunction;
class trex_range;
class trex_plus;


/********** Visitor *****************************/
class TrexVisitor {
  virtual void accept(class TrexPrePostVisitor *visitor) const = 0;
};

class TrexPrePostVisitor {
  public:
  /*pre-order */
  virtual void pre_visit(const trex_epsilon* ) = 0;
  virtual void pre_visit(const trex_byte* ) = 0;
  virtual void pre_visit(const trex_byte_set* ) = 0;
  virtual void pre_visit(const trex_disjunction* ) = 0;
  virtual void pre_visit(const trex_conjunction* ) = 0;
  virtual void pre_visit(const trex_range* ) = 0;
  virtual void pre_visit(const trex_plus* ) = 0;
  
  /*post-order*/
  virtual void post_visit(const trex_epsilon* ) = 0;
  virtual void post_visit(const trex_byte* ) = 0;
  virtual void post_visit(const trex_byte_set* ) = 0;
  virtual void post_visit(const trex_disjunction* ) = 0;
  virtual void post_visit(const trex_conjunction* ) = 0;
  virtual void post_visit(const trex_range* ) = 0;
  virtual void post_visit(const trex_plus* ) = 0;
  
};


/********** trex kind ***************************/
enum rex_kind {
  rk_none,
  rk_epsilon,
  rk_byte,
  rk_byte_set,
  rk_disjunction,//includes sets
  rk_conjunction,//includes plus, and ranges
  rk_range,
  rk_plus
};

/********** trex (base class) *******************/
class trex: public uidObject, public TrexVisitor {
public:
  unsigned int length_max; //max. length of an element of this type
  bool bounded() { return length_max <	_INFINITY; }

protected:
  static const unsigned int _INFINITY = (unsigned int ) -1;  
  std::vector<MPInt> _num_matches;
public:
  rex_kind rk;
  bool known_ambiguous;
  
  trex(rex_kind k): _num_matches(0), known_ambiguous(false) { rk = k;}
  trex(void): _num_matches(0), known_ambiguous(false)       { rk=rk_none;}
  const std::vector<MPInt>& get_num_matches() { return _num_matches; }
  virtual ~trex()  {};
  virtual trex* clone() = 0;
  virtual stateNFA* toNFA(stateNFA* succ);
  
  virtual std::string unparse() = 0;
  
  virtual void dot(std::ostream& dio) = 0;
  void to_dot(std::ostream& dio);
  virtual void dbg_dot(std::ostream& dio) = 0;
  void label_dot(std::ostream& dio);

  bool is_infinite() { return has_plus();}
  
  virtual bool has_plus() {return false;}
  virtual bool generates_epsilon() = 0;
  virtual bool problematic() = 0;
  
  // parse tree stuff:
  virtual int count_pt_children() {return 0;}
  virtual int count_children() {return 0;}
  virtual trex* child(unsigned int ) { assert(0);}
  virtual int child_index(trex*) {return -1;}
  virtual bool is_alternaive() {return false;}
  
  virtual bool matches(t_term) { return false; }
  virtual const MPInt& num_matches(unsigned int len) {
     if (_num_matches.size() < len+1)
       update_matches(len);
     assert(_num_matches.size() >= len+1);
     return _num_matches[len];
  }
  
  // fills in [0 .. len] entries in _num_matches
  virtual void update_matches(unsigned int) = 0;

  virtual const std::vector<unsigned int>& get_chars()
  { assert(0); static std::vector<unsigned int> dummy; return dummy; }
  void accept(class TrexPrePostVisitor *visitor) const = 0;
private:
  trex(trex&);
  trex& operator=(const trex & T);
  void common_dbg_dot(std::ostream& dio);
};

/********** trex_epsilon ************************/
class trex_epsilon : public trex {
public:
  trex_epsilon(void): trex(rk_epsilon) {length_max = 0;};
  virtual trex* clone() { return new trex_epsilon();}
  virtual stateNFA* toNFA(stateNFA* succ);
  std::string virtual unparse();
  
  void dot(std::ostream& dio);
  void dbg_dot(std::ostream& dio);
  bool generates_epsilon() {return true;}
  virtual bool problematic() {return false;}

  void accept(class TrexPrePostVisitor *visitor) const {
    visitor->pre_visit(this);
    visitor->post_visit(this);
  }
private:
  virtual const MPInt& num_matches(unsigned int len) {
    if (len == 0)
      return MPInt_one;
    return MPInt_zero;
  }
  virtual void update_matches(unsigned int) {};
};

/********** trex_byte ***************************/
class trex_byte : public trex {
  t_term b;
protected:
  std::vector<unsigned int> v_chars;
  trex_byte(rex_kind rk): trex(rk), b(0) {length_max = 1;};// called fron trex_byte_set
public:
  trex_byte(t_term b0): trex(rk_byte), b(b0) {v_chars.push_back(b);length_max = 1;};
/*
  BEWARE: If we translate (trx)+ using trx = new trex_conjunction(trx, new trex_star(trx));
  (this also holds for similar translations for {ranges}, etc)
  then the trex structure is no longer a tree; it is a DAG!
  Also, the types in the stateNFA may look weird.
  May need to add a trex_light_clone, that simply stores a pointer to the
  original and forwards the calls.
  The clone would be 'light', i.e. not contain _num_matches tables of its own.
  Thus, for (trx)+, instead of
  trx = new trex_conjunction(trx, new trex_star(trx));
  we may want to use
  trx = new trex_conjunction(light_clone(trx), new trex_star(trx));
  For now, we just use aplain clone.
  OPTIMIZATIONS TBD:
  (1) use a light_clone, by forwarding the foo() calls in trex to clone->foo()
*/
  virtual trex* clone() { return new trex_byte(b);}
  virtual stateNFA* toNFA(stateNFA* succ);
  std::string virtual unparse();
  virtual bool matches(t_term c) { return c == b; }
  
  void dot(std::ostream& dio);
  void dbg_dot(std::ostream& dio);
  bool generates_epsilon() {return false;}
  virtual bool problematic() {return false;}
  virtual unsigned int rank(t_term c) {assert(c==b); return 0;}
  virtual t_term unrank(const MPInt& rank) {assert(rank==0);return b;}
  virtual const std::vector<unsigned int>& get_chars() { return v_chars; }
  void accept(class TrexPrePostVisitor *visitor) const {
    visitor->pre_visit(this);
    visitor->post_visit(this);
  }
private:
  virtual const MPInt& num_matches(unsigned int len) {
    if (len == 1)
     return MPInt_one;
    return MPInt_zero;
  }
  virtual void update_matches(unsigned int) {};
};

/********** trex_byte_set ***********************/
class trex_byte_set : public trex_byte {
  std::bitset<MAX_SYMS> _chars;
  unsigned char _rank[MAX_SYMS];
  unsigned char _unrank[MAX_SYMS];
  MPInt chars_count;
public:
  trex_byte_set(const std::bitset<MAX_SYMS>& chars);
  virtual trex* clone() { return new trex_byte_set(_chars);}
  std::string virtual unparse();
  virtual bool matches(t_term c) { return _chars.test(c); }
  
  t_term byte() {assert(0); return 0;}
  virtual unsigned int rank(t_term c) {
    //assert(c < MAX_SYMS);
    assert(_chars.test(c));
    return _rank[c];
  }
  virtual t_term unrank(const MPInt& rank) {
    assert(rank<_chars.count());
    unsigned int r = MPInt_to_uint(rank);
    assert(r < MAX_SYMS);
    t_term res = _unrank[r];
    assert(_chars.test(res));
    return res;
  }
  const std::vector<unsigned int>& get_chars() {
    if (v_chars.empty()) {
      for (unsigned int i=0; i < MAX_SYMS; i++) {
	if (_chars.test(i)) {
	  v_chars.push_back(i);
	}
      }
      assert(!v_chars.empty());
    }
    return v_chars;
  }
  void accept(class TrexPrePostVisitor *visitor) const {
    visitor->pre_visit(this);
    visitor->post_visit(this);
  }
private:
  virtual const MPInt& num_matches(unsigned int len) {
    if (len == 1)
     return chars_count;
    return MPInt_zero;
  }
};

/********** trex_disjunction ********************/
class trex_disjunction : public trex {
  trex *left;
  trex *right;
public:
  trex_disjunction(trex* l, trex* r): trex(rk_disjunction), left(l),right(r)
  { assert(l); assert(r); length_max = std::max(l->length_max, r->length_max);};
  ~trex_disjunction() {delete left; delete right;}
  virtual trex* clone() { return new trex_disjunction(left->clone(), right->clone());}
  virtual stateNFA* toNFA(stateNFA* succ);
  std::string virtual unparse();

  // parse tree stuff:
  int count_pt_children() {return 1; /* not 2. Only 1 child will match */}
  int count_children() {return 2;}
  int child_index(trex* t) {
    if (t == left) return 0;
    if (t == right) return 1;
    return -1;
  }
  virtual trex* child(unsigned int i)
  {assert(i<=1); return (i==0)? left: right;}
  
  void dot(std::ostream& dio);
  void dbg_dot(std::ostream& dio);
  virtual bool has_plus() {return left->has_plus()||right->has_plus();}
  bool generates_epsilon() {return left->generates_epsilon() || right->generates_epsilon();}
  bool problematic() {return left->problematic() || right->problematic();}
  void accept(class TrexPrePostVisitor *visitor) const {
    visitor->pre_visit(this);
    left->accept(visitor);
    right->accept(visitor);
    visitor->post_visit(this);
  }
private:
  virtual void update_matches(unsigned int len);
};

/********** trex_conjunction ********************/
class trex_conjunction : public trex {
  trex *left;
  trex *right;
public:
  trex_conjunction(trex* l, trex* r): trex(rk_conjunction), left(l),right(r)
    { assert(l); assert(r);
      length_max = (l->length_max == _INFINITY || r->length_max == _INFINITY)?
	    _INFINITY :  l->length_max+r->length_max;
    };
  ~trex_conjunction() {delete left; delete right;}
  virtual trex* clone() { return new trex_conjunction(left->clone(), right->clone());}
  virtual stateNFA* toNFA(stateNFA* succ);
  std::string virtual unparse();

  // parse tree stuff:
  int count_pt_children() {return count_children();}
  int count_children() {return 2;}
  int child_index(trex* t) {
    if (t == left) return 0;
    if (t == right) return 1;
    return -1;
  }
  virtual trex* child(unsigned int i)
  {assert(i<=1); return (i==0)? left: right;}
  
  void dot(std::ostream& dio);
  void dbg_dot(std::ostream& dio);
  virtual bool has_plus() {return left->has_plus()||right->has_plus();}
  bool generates_epsilon() {return left->generates_epsilon() && right->generates_epsilon();}
  bool problematic() {return left->problematic() || right->problematic();}
  void accept(class TrexPrePostVisitor *visitor) const {
    visitor->pre_visit(this);
    left->accept(visitor);
    right->accept(visitor);
    visitor->post_visit(this);
  }
private:
  virtual void update_matches(unsigned int len);
};

/********** trex_range ********************/
class trex_range : public trex {
  trex *tr;
  int low, high;
public:
  trex_range(trex* tr0, int low0, int high0):
  trex(rk_range), tr(tr0), low(low0), high(high0)
    { assert(tr);
      assert(low >=0 && high >= low);
      length_max = (tr->length_max == _INFINITY)?
	    _INFINITY :  high*tr->length_max;
    };
  ~trex_range() {delete tr;}
  virtual trex* clone() { return new trex_range(tr->clone(), low, high);}
  virtual stateNFA* toNFA(stateNFA* succ);
  std::string virtual unparse();

  // parse tree stuff:
  int count_pt_children() {assert(0&&"NOT IMPLEMENTED");}
  int count_children() {assert(0&&"NOT IMPLEMENTED");}
  int child_index(trex* ) {assert(0&&"NOT IMPLEMENTED");}
  virtual trex* child(unsigned int ) {assert(0&&"NOT IMPLEMENTED");}
  
  void dot(std::ostream& );
  void dbg_dot(std::ostream& );
  virtual bool has_plus() {return tr->has_plus();}
  bool generates_epsilon() {return tr->generates_epsilon();}
  bool problematic() {return tr->problematic(); }
  void accept(class TrexPrePostVisitor *visitor) const {
    visitor->pre_visit(this);
    tr->accept(visitor);
    visitor->post_visit(this);
  }
private:
  virtual void update_matches(unsigned int ) {assert(0&&"NOT IMPLEMENTED");}
};

/********** trex_plus ***************************///trex_star delete after v808
class trex_plus: public trex {
  trex *tr;
public:
  trex_plus(trex *tr0): trex(rk_plus), tr(tr0)
  { length_max = ((tr->length_max>0)?_INFINITY:0);};
  ~trex_plus() {delete tr;}
  virtual trex* clone() { return new trex_plus(tr->clone());}
  virtual stateNFA* toNFA(stateNFA* succ);
  
  // parse tree stuff:
  int count_pt_children()  {assert(0); return 1;}
  int count_children() {return 1;}//{assert(0); return 1;}
  virtual trex* child(unsigned int i)
  {assert(i==0); return tr;}
  int child_index(trex* t) {
    if (t == tr) return 0;
    return -1;
  }
public:
  trex *get_tr() {return tr;}
  
  std::string virtual unparse();
  void dot(std::ostream& dio);
  void dbg_dot(std::ostream& dio);
  virtual bool has_plus() {return true;}
  bool generates_epsilon() {return tr->generates_epsilon();}
  bool problematic() {return tr->generates_epsilon() || tr->problematic();}
  void accept(class TrexPrePostVisitor *visitor) const {
    visitor->pre_visit(this);
    tr->accept(visitor);
    visitor->post_visit(this);
  }
private:
  virtual void update_matches(unsigned int len);
};
/************************************************/
void gather_trex_nodes(trex* type, std::set<trex*>& result);
/************************************************/
/*
void MPIntDiv(MPFloat& res, const MPInt&i1, const MPInt&i2) {
  MPFloat f1(i1);
  MPFloat f2(i2);
  res = f1/f2;
}
*/
/************************************************/

template <class TContainer> void dumpTContainer(const TContainer c, const std::string &msg ) {
  std::cout << msg << "...:[";
  for (typename TContainer::const_iterator it = c.begin(); it != c.end(); ++it) {
    std::cout << (*it)->uid;
    if (dynamic_cast<trex*>(*it))
      std::cout << "(" << (dynamic_cast<trex*>(*it))->unparse() << ")";
    else
      std::cout << ",";
  }
  std::cout << "]" << std::endl;
}

/************************************************************************/
/* TBD: temporary place ; move this stuff elsewhere                     */
/************************************************************************/
extern trex* regex2trex(const std::string& regexp,
			void* dbg_tm_parse, void* dbg_tm_convert);
#endif //TREX_H
