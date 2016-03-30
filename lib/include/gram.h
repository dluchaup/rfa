/*-----------------------------------------------------------------------------
 * File:    gram.h
 * Author:  Daniel Luchaup
 * Date:    December 2013
 * Copyright 2013 Daniel Luchaup luchaup@cs.wisc.edu
 *
 * This file contains unpublished confidential proprietary work of
 * Daniel Luchaup, Department of Computer Sciences, University of
 * Wisconsin--Madison.  No use of any sort, including execution, modification,
 * copying, storage, distribution, or reverse engineering is permitted without
 * the express written consent (for each kind of use) of Daniel Luchaup.
 *
 *---------------------------------------------------------------------------*/
#ifndef _META_GRAMMAR_H
#define _META_GRAMMAR_H
#include <assert.h>
#include <string>
#include <vector>
#include <map>
#include <stack>
#include "MPInt.h"
#include "ranker.h"
#include <climits>

class Rule;
class ParseTree;
char* byte_copy(const void *src, size_t n); //TBD: move to some utility file

/********** ParseTree ***************************/
class PTVisitor {
  virtual void accept(class ParseTreePrePostVisitor *visitor) const = 0;
};
/********** ParseTree ***************************/
class ParseTree : public PTVisitor {
public:
  unsigned int yield_length;
  const Rule *r; // grammar rule which derives this ParseTree
  ParseTree(const Rule *r0) : r(r0) {}
  virtual ~ParseTree() {}
  virtual std::string flatten() const = 0;
  virtual void accept(class ParseTreePrePostVisitor *visitor) const = 0;
};

/********** Symbol ******************************/
class Symbol {//TBD: inherit from rankerinterface
public:
  std::string name;
  bool is_terminal;
  Symbol(std::string name0) :  name(name0), is_terminal(false) {}
  virtual ~Symbol() {/*std::cout << "deleting " << name << std::endl;*/}
};

/********** Terminal ****************************/
class Terminal: public Symbol, /* a bit of a hack: */ PTVisitor {
public:
  Terminal(std::string name0) : Symbol(name0) { is_terminal = true; }
  size_t length() const { return name.length(); }
  virtual void accept(class ParseTreePrePostVisitor *visitor) const;
};

/********** Nonterminal *************************/
class Nonterminal: public Symbol {
  friend class dbgG;
  // stuf for memoization ...
  unsigned int memoized_size;
  // ... stuf for memoization
  std::vector<MPInt> _num_matches;
  std::vector<Rule*> rules;
public:
  Nonterminal(std::string name0) : Symbol(name0) { memoized_size = 0; };
  virtual ~Nonterminal();
  void add_rule(Rule* r);
  
  inline const MPInt& num_matches(unsigned int len) const {
    assert(len < _num_matches.size());
    return _num_matches[len];
  }
  virtual const MPInt& memoize_num_matches(unsigned int len,
					   unsigned int max_len) {
    if (len >= memoized_size)
      memoize(len, max_len);
    return _num_matches[len];
  }
  virtual MPInt rank(const ParseTree* pt) const;
  virtual ParseTree* unrank(MPInt& r, unsigned int len) const;
  void memoize(unsigned int len, unsigned int max_len);
  unsigned int get_memoized_size() { return memoized_size; }
};

/********** Rule ********************************/
/* A Rule abstractly describes how strings are derived from a Nonterminal.
 * For ranking/unranking purposes we only care which Nonterminal is expanded
 * and what is the rule index in the rule ordering for that Nonterminal.
 * It should hold be able to rank/unrank strings derived from the rule.
 */
#include "uidObject.h"
class Rule : public uidObject
{
  friend class dbgG;
protected:
  int id;
public:
  /* Assume that A is a nonterminal whose rules
   * are A->rules == [r0,r1,r2,r3], where
   *   r0: A -> ...
   *   r1: A -> ...
   *   r2: A -> ...
   *   r3: A -> ...
   * and assume that 'this' represents rule r2: A -> ... Then:
   *   this->nr    == 2 (i.e. the index of r2)
   *   this->left == A
   */
  unsigned int nr; // index in left->rules: 'this' == left->rules[nr];
  Nonterminal* left;

  Rule(Nonterminal *l, const char* rname0) : left(l), name(rname0) {
    left->add_rule(this);
  }
  virtual ~Rule() { /*std::cout << "deleting " << name << std::endl;*/}
  virtual const MPInt& num_matches(unsigned int len) const = 0;
  virtual const MPInt& memoize_num_matches(unsigned int len,
					   unsigned int max_len) = 0;
  virtual MPInt rank(const ParseTree* pt) const = 0;
  virtual ParseTree* unrank(MPInt& rnk, unsigned int len) const = 0;
  virtual void memoize(unsigned int len, unsigned int max_len) = 0;
  /****************** debugging ********************/
  const char* name;
  virtual std::string toString() const = 0;
};

/********** GRule *******************************/
/* general grammar rule of the form
 * r2: A -> x B y C D (with x,y terminals and B,C,D nonterminals)
 */
class GRule : public Rule {
  MPInt rank(const std::vector<ParseTree*>& pt_children,
	     unsigned int start, unsigned int rest_length) const;
  const MPInt& num_w_matches(unsigned int start, unsigned int len) const;
  // stuf for memoization ...
  unsigned int memoized_size;
  // ... stuf for memoization
  std::vector<MPInt> _num_matches;
  /////////////////////////
  const MPInt& memoize_num_w_matches(unsigned int start,
			    unsigned int len, unsigned int max_len);
  std::vector<unsigned int> c_memoized_size;
  // ... stuf for memoization
  std::vector<std::vector<MPInt> > c_num_matches;
public:
  /* Assume that A is a nonterminal whose rules
   * are A->rules == [r0,r1,r2,r3], where
   *   r0: A -> ...
   *   r1: A -> ...
   *   r2: A -> x B y C D (with x,y terminals and B,C,D nonterminals)
   *   r3: A -> ...
   * and assume that 'this' represents rule r2: A -> x B y C D. Then:
   *                     we view rule r2 as r2: left -> right, where
   *   this->nr    == 2 (i.e. the index of r2)
   *   this->left == A
   *   this->right = [x,B,y,C,D] (store the entire right side)
   *   this->children = [B,C,D]  (just the non-terminals on the right side)
   *   this->terminal_length = length(x)+length(y)
   */
  std::vector<Symbol*> right;
  std::vector<Nonterminal*> children;
  unsigned int terminal_length;
  GRule(Nonterminal* l, const char* rname0)
    : Rule(l, rname0) { memoized_size = 0; terminal_length = 0;};
  virtual MPInt rank(const ParseTree* pt) const;
  virtual ParseTree* unrank(MPInt& rnk, unsigned int len) const;

  
  virtual const MPInt& num_matches(unsigned int len) const {//TBD:perf this should be inline not virtual! Must fix LRule
    assert(len < _num_matches.size());
    return _num_matches[len];
  }
  virtual const MPInt& memoize_num_matches(unsigned int len, unsigned int max_len) {
    if (len >= memoized_size)
      memoize(len, max_len);
    return _num_matches[len];
  }
  void memoize(unsigned int len, unsigned int max_len);
  void append_symbol(Symbol* s) {
    right.push_back(s);
    if (s->is_terminal)
      terminal_length += (static_cast<Terminal*>(s))->length();
    else
      children.push_back(static_cast<Nonterminal*>(s));
  }
  /********** GParseTree **************************/
  class GParseTree : public ParseTree {
    public:
    const GRule *gr;//TBD: this is the same as this->r. Unpleasant...
  /* Consider the example from Rule's comment, and that this is a derivation of
   * r2: A -> x B y C D (with x, y terminals and B,C,D nonterminals)
   * 
   */
    std::vector<ParseTree*> children;
    GParseTree(const GRule *r0) : ParseTree(r0), gr(r0) {
      yield_length = gr->terminal_length;
    }
    void add_child(ParseTree* child, int n) {
      assert(children[n] == NULL);
      children[n] = child;
      yield_length += child->yield_length;
    }
    
    void push_child(ParseTree* child) {
      children.push_back(child);
      yield_length += child->yield_length;
    }
    
    ~GParseTree() {
      for (std::vector<ParseTree*>::iterator it = children.begin();
	   it != children.end();
	   ++it) {
	ParseTree *pt_child = *it;
	delete pt_child;
      }
    }
    std::string flatten() const;
    void accept(class ParseTreePrePostVisitor *visitor) const;
  };
  /****************** debugging ********************/
  std::string toString() const;
};
#if 1
/********** LRule *******************************/
/* Lexical rule of the form
 * r2: A -> REX , where REX is a regular expression
 */
class LRule : public Rule {
  ranker *rkr;
  unsigned int artificial_bound;// Use if we want tokens no longer than a limit
  // stuf for memoization ...
  unsigned int memoized_size;
public:
  LRule(Nonterminal *l,
	ranker *rkr0,
	const char* rname0,
	unsigned int artificial_bound0 = UINT_MAX)
    : Rule(l, rname0), rkr(rkr0), artificial_bound(artificial_bound0)
    { memoized_size = 0;};
  ~LRule() { delete rkr; }
  virtual MPInt rank(const ParseTree* pt) const {
    const LParseTree* lpt = dynamic_cast<const LParseTree*>(pt);
    MPInt res = 0;
    bool ok = rkr->rank(res, lpt->yield, lpt->yield_length);
    assert(ok);
    return res;
  }
  virtual ParseTree* unrank(MPInt& rnk, unsigned int len) const {//TBD:perf
    std::string yield;
    rkr->unrank(rnk, len, yield);
    LParseTree *lpt = new LParseTree(this, byte_copy(yield.c_str(),yield.length()), yield.length());
    return lpt;
  }

  const MPInt& num_matches(unsigned int len) const {
    assert(len < rkr->_memoized_size);
    if (len > artificial_bound)
      return MPInt_zero;
    return rkr->num_matches(len);
  }
  virtual const MPInt& memoize_num_matches(unsigned int len, unsigned int max_len) {
    assert(len <= max_len);
    if (len > artificial_bound) //TBD:perf:get rid of this.Use rkr->num_matches
      return MPInt_zero;
    if (len >= rkr->_memoized_size)
      rkr->update_matches(max_len);
    return rkr->num_matches(len);
  }
  
  void memoize(unsigned int len, unsigned int max_len) {
    assert(len <= max_len);
    rkr->update_matches(max_len);
    memoized_size = max_len+1;
  }
  /********** LParseTree **************************/
  class LParseTree : public ParseTree {
    public:
    const LRule *lr;//TBD: this is the same as this->r. Unpleasant...
    const char *yield;
    LParseTree(const LRule *r0, const char* yield0, unsigned int yield_length0)
      : ParseTree(r0), yield(yield0) {yield_length=yield_length0;}
    LParseTree(const LRule *r0)
      : ParseTree(r0), lr(r0), yield(NULL) {}
    virtual ~LParseTree() {
      if (yield)
	free(const_cast<char*>(yield)); //malloc-ed from lexer
    }
    std::string flatten() const;
    void accept(class ParseTreePrePostVisitor *visitor) const;
  };
  /****************** debugging ********************/
  std::string toString() const {
    std::string res = left->name + " ::" + rkr->rex;
    return res;
  }
};
/********** L1Rule *******************************/
/* Lexical rule of the form
 * r2: A -> str , where str is a string
 * Similar to LRule, but used to count the number of *words* not bytes.
 * That is, each token is condidered of length 1.
 */
class L1Rule : public Rule {
  const std::string keyword;
  protected:
public:
  L1Rule(Nonterminal *l,
	 const char* keyword0,
	 const char* rname0)
    : Rule(l, rname0), keyword(std::string(keyword0))
    {};
  virtual ~L1Rule() {}
  virtual MPInt rank(const ParseTree* ) const {
    MPInt res = 0;
    return res;
  }
  virtual ParseTree* unrank(MPInt& rnk, unsigned int len) const {//TBD:perf
    std::string yield = keyword;
    assert(len == 1);
    assert(rnk == MPInt_zero);
    LParseTree *lpt = new LParseTree(this, byte_copy(yield.c_str(),yield.length()), yield.length());
    return lpt;
  }

  const MPInt& num_matches(unsigned int len) const {
    if (len != 1)
      return MPInt_zero;
    else
      return MPInt_one;
  }
  virtual const MPInt& memoize_num_matches(unsigned int len, unsigned int ) {
    return num_matches(len);
  }
  
  void memoize(unsigned int len, unsigned int max_len) {
    assert(len <= max_len);
  }
  /********** LParseTree **************************/
  class LParseTree : public ParseTree {
    public:
    const L1Rule *lr;//TBD: this is the same as this->r. Unpleasant...
    const char *yield;
    unsigned int yield_string_length;
    LParseTree(const L1Rule *r0, const char* yield0, unsigned int yield_length0)
      : ParseTree(r0),
      yield(yield0), yield_string_length(yield_length0) {
      //std::cout << "L1Rule::LParseTree::LParseTree's yield=" << yield << std::endl;;
      //std::cout << "L1Rule::LParseTree::LParseTree's yield_string_length=" << yield_string_length << std::endl;;
      yield_length=1;
    }
    LParseTree(const L1Rule *r0)
      : ParseTree(r0), lr(r0), yield(NULL) {}
    virtual ~LParseTree() {
      if (yield)
	free(const_cast<char*>(yield)); //malloc-ed from lexer
    }
    std::string flatten() const;
    void accept(class ParseTreePrePostVisitor *visitor) const;
  };
  /****************** debugging ********************/
  std::string toString() const {
    return keyword;
  }
};
#endif

/********** Visitor *****************************/
class ParseTreePrePostVisitor {
  public:
  /*pre-order */
  virtual void pre_visit(const Terminal *t) = 0;
  virtual void pre_visit(const GRule::GParseTree *pt) = 0;
  virtual void pre_visit(const LRule::LParseTree *pt) = 0;
  virtual void pre_visit(const L1Rule::LParseTree *pt) = 0;
  /*post-order*/
  virtual void post_visit(const Terminal *t) = 0;
  virtual void post_visit(const GRule::GParseTree *pt) = 0;
  virtual void post_visit(const LRule::LParseTree *pt) = 0;
  virtual void post_visit(const L1Rule::LParseTree *pt) = 0;
};
/********** Grammar *****************************/
class Grammar {
public:
  Nonterminal* S0;
  
  std::vector<Nonterminal*> nonterminals;
  std::vector<Terminal*> terminals;
  
  std::map<std::string, Terminal*> stringTerminals;
  std::map<std::string, Nonterminal*> stringNonterminals;

  Grammar() : S0(NULL) {}
  virtual ~Grammar() {
     for(std::vector<Nonterminal*>::iterator nit = nonterminals.begin();
	 nit != nonterminals.end(); ++nit)
	   delete (*nit);
     for(std::vector<Terminal*>::iterator tit = terminals.begin();
	 tit != terminals.end(); ++tit)
	   delete (*tit);
  }
  void set_S(Nonterminal* start) { S0 = start; }
  const Nonterminal* S() const {
    assert(S0 && SEQCONTAINS(nonterminals, S0));
    return S0;
  }
  
  Terminal* get_terminal(std::string name) {
     if (CONTAINS(stringTerminals, name))
       return stringTerminals[name];
     else {
       Terminal *t = stringTerminals[name] = new Terminal(name);
       terminals.push_back(t);
       return t;
     }
  }

  Nonterminal* get_nonterminal(std::string name) {
     if (CONTAINS(stringNonterminals, name))
       return stringNonterminals[name];
     else {
       Nonterminal *nt = stringNonterminals[name] = new Nonterminal(name);
       nonterminals.push_back(nt);
       return nt;
     }
  }
};
///////////// grammar parsing stuff ///////////////////
extern "C" GRule::GParseTree* mkgpt(GRule *gr, ... );
extern "C" GRule::GParseTree* mkgptn(GRule *gr, int num, ... );
extern "C" GRule::GParseTree* mkgptr(GRule *gr, ... );
ParseTree *pt_from_file(FILE *fp);
ParseTree *pt_from_mem(const char *data, int len, int line);

// Debugging: show derivation rules
class PTDerivateVisitor : public ParseTreePrePostVisitor {
  std::string result;
  public:
  /*pre-order */
  virtual void pre_visit(const Terminal *) {}
  
  virtual void pre_visit(const GRule::GParseTree *pt) {
    result = result + "[" + pt->r->name;
  }
  
  virtual void pre_visit(const LRule::LParseTree *pt) {
    result = result + "[" + pt->r->name + "]";
  }
  virtual void pre_visit(const L1Rule::LParseTree *pt) {
    result = result + "[" + pt->r->name + "]";
  }
  /*post-order*/
  virtual void post_visit(const Terminal *) {}
  
  virtual void post_visit(const GRule::GParseTree *) {
    result = result + "]";
  }
  virtual void post_visit(const LRule::LParseTree *) {}
  virtual void post_visit(const L1Rule::LParseTree *) {}

  std::string derivation(ParseTree* pt) {
    pt->accept(this);
    return result;
  }
};

class PTYieldVisitor : public ParseTreePrePostVisitor {
  std::stack<std::string> stk;
  const std::string separator;
public:
  /*pre-order */
  virtual void pre_visit(const Terminal *) {};
  virtual void pre_visit(const GRule::GParseTree *) {}
  virtual void pre_visit(const LRule::LParseTree *) {}
  virtual void pre_visit(const L1Rule::LParseTree *) {}
  /*post-order*/
  virtual void post_visit(const Terminal *t);
  virtual void post_visit(const GRule::GParseTree *pt);
  virtual void post_visit(const LRule::LParseTree *pt);
  virtual void post_visit(const L1Rule::LParseTree *pt);

public:
  PTYieldVisitor(const char* sep=" ") : separator(sep) {}

  std::string yield(ParseTree* pt) {
    assert(stk.empty());
    pt->accept(this);
    assert(stk.size() == 1);
    std::string result = stk.top();
    stk.pop();
    return result;
  }
};
#endif
