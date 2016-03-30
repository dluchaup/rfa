/*-----------------------------------------------------------------------------
 * File:    gram.cc
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
#include <iostream>
#include "gram.h"
#include "MyTime.h"
#include "dbg.h"

#define CHECKSIZES ((children.size() == 0 && \
		     c_memoized_size.size() == 0 && \
		     c_num_matches.size() == 0) ||	    \
		    (children.size() != 0 &&\
		     c_memoized_size.size() == children.size() &&\
		     c_num_matches.size() == children.size() -1))

#define C_NUM_MATCHES(start, len)					\
((start < c_num_matches.size()) ?					\
 (c_num_matches[start][len]) :						\
 (assert(children.size() > 0),						\
  assert(start == c_num_matches.size() && start == children.size() -1),	\
  children[start]->num_matches(len)))
  
/* how many strings of length 'len' can be derived from the
 * sub-word word[start]word[start+1]word[start+2]..word[word.size()-1]
 *
 * TBD:perf: memoize all this in an array inside the 'rule' class.
 */
//only used in rank/unrank
const MPInt& GRule::num_w_matches(unsigned int start, unsigned int len) const {
  MPInt res = 0;
  assert (start <= children.size());
  if (start == children.size()) {
    if (len == 0)
      return MPInt_one;//one way to match the empty string
    return MPInt_zero;
  }
  assert(CHECKSIZES);
  return C_NUM_MATCHES(start, len);
}

/* pt_children is an array of parse trees, and children are their types
 * Assume that:
 *   pt_children is T0 T1 T2 T3 T4
 *      children is N0 N1 N2 N3 N4
 * and that start==2, then this function
 * returns the rank of the [T2 T3 T4] among the strings derived from N2N3N4
 * i.e. the number of the strings derived from N2N3N4 that precede [T2 T3 T4]
 * ([T2 T3 T4] represents the concatenation of the yield of the trees T2 T3 T4)
 */
MPInt GRule::rank(const std::vector<ParseTree*>& pt_children,
		 unsigned int start,
		 unsigned int rest_length) const {
  assert (start <= pt_children.size());
  if (start == pt_children.size()) {
    assert(rest_length == 0);
    return 0;
  }
  MPInt res = 0;
  unsigned int len = pt_children[start]->yield_length;
  Nonterminal& N = *children[start];
  MPInt starts_shorter = 0;               //TBD: check if memoization helps
  for (unsigned int l = 0; l < len; ++l) {//TBD: check if memoization helps
    MPInt count_at_l = N.num_matches(l);
    if (count_at_l > 0) {//if possible avoid bignum operations
      MPInt rest_matches_l = num_w_matches(start+1, rest_length-l);
      starts_shorter += count_at_l * rest_matches_l;
    }
  }
  MPInt rest_matches = num_w_matches(start+1,rest_length - len);
  MPInt rank_start = children[start]->rank(pt_children[start]);
  MPInt starts_same_len = rank_start * rest_matches;
  MPInt starts_equal = rank(pt_children, start+1, rest_length - len);
  return starts_shorter + starts_same_len + starts_equal;
}

MPInt GRule::rank(const ParseTree* pt) const {
  const GParseTree* gpt = dynamic_cast<const GParseTree*>(pt);
  assert(gpt->yield_length >= terminal_length);
  return rank(gpt->children, 0, gpt->yield_length - terminal_length);
}

ParseTree* GRule::unrank(MPInt& rnk, unsigned int len) const {
  assert(len >= terminal_length);
  len -= terminal_length;
  
  GParseTree *pt = new GParseTree(this);
  size_t num_children = this->children.size();
  pt->children.resize(num_children); 
  // Now find the children..
  unsigned int num_nonterminals = this->children.size();
  if (num_nonterminals == 0) {
    assert(rnk == 0 && len == 0);
    return pt;
  }

  // fill in those children that are not the last one.
  for (unsigned int n = 0; n < num_nonterminals - 1; ++n) {
    // note that n+1 < num_nonterminals <==> at least two more children left
    const Nonterminal *N = this->children[n];
    // Find out how long is the string derived from the n-th child, N.
    MPInt n_shorter = 0;
    unsigned int i;
    
    for (i = 0; i <= len; ++i) {//TBD:perf: use binary search <-cannot. Memoize
      MPInt n0 = N->num_matches(i);
      MPInt n1 = num_w_matches(n+1, len-i);
      MPInt next_n_shorter = n_shorter + n0 * n1;
      /* if next_num0_shorter is unchanged for some 'i', then l0 cannot be that
	 'i' because there are no matches that decompose as (i, yield_length-i)
      */
      if (next_n_shorter > rnk)
	break;
      n_shorter = next_n_shorter;
    }
    assert(i<=len);
    
    unsigned int l_n = i; // length of string derived from N
    rnk -= n_shorter;

    // recursively fill in the n-th child
    MPInt rank_n = rnk / num_w_matches(n+1, len - l_n); //TBD:perf merge division and mod
    pt->add_child(N->unrank(rank_n, l_n), n);
    
    // ... and prepare for the rest of the children
    rnk = rnk % num_w_matches(n+1, len - l_n);          //TBD:perf merge division and mod
    len = len - l_n;  // length of string derived from nonterminals "after" N.
  }
  // recursively fill in the last child
  pt->add_child(this->children[num_nonterminals-1]->unrank(rnk, len),
		num_nonterminals -1);
  return pt;
 }

Nonterminal::~Nonterminal() {
  for(std::vector<Rule*>::iterator it = rules.begin(); it != rules.end(); ++it)
    delete (*it);
}

void Nonterminal::add_rule(Rule* r) {
  r->nr = rules.size();
  rules.push_back(r);
}

MPInt Nonterminal::rank(const ParseTree* pt) const {
  MPInt precede = 0;                    //TBD: check if memoization helps
  unsigned int num_preceding_rules = pt->r->nr;
  
  for (unsigned int ir = 0; ir < num_preceding_rules; ++ir) {
    precede += this->rules[ir]->num_matches(pt->yield_length);
  }
  MPInt rule_rank = pt->r->rank(pt);
  return precede + rule_rank;
}

ParseTree* Nonterminal::unrank(MPInt& rnk, unsigned int len) const {
  unsigned int num_rules = this->rules.size();
  unsigned int ir;
  // find what rule to apply
  for (ir = 0; ir < num_rules; ++ir) {
    MPInt max_rank_candidate = this->rules[ir]->num_matches(len);
    if (max_rank_candidate > rnk)
      break;
    rnk -= max_rank_candidate;
  }
  assert(ir < num_rules);
  const Rule* rule = this->rules[ir];
  return rule->unrank(rnk, len);
}

void Nonterminal::memoize(unsigned int len, unsigned int max_len) {
  // ensure that _num_matches[i] is valid for i in [0,1,...,len]
  if (_num_matches.size() == 0)
    _num_matches.resize(max_len + 1, 0);
  assert(max_len + 1 == _num_matches.size());
  assert(len <= max_len);
  assert(memoized_size <= len);
  
  unsigned int old_size = memoized_size;
  memoized_size = len + 1;
  for (unsigned int new_l = old_size; new_l < memoized_size; ++new_l) {
    /* NOTE: It is possible that the grammar has cycles/recursion,
     * and that while recursing from this loop we end up calling again
     * this->memoize_num_matches(x, max_len). In that case x must be x <= new_l
     * and if x < new_l then memoize_num_matches(x, max_len) properly returns
     * _num_matches[x] which holds a  stable and correct value.
     * However, if x == new_l then _num_matches[x] is 0, which is an unstable,
     * and a pending update value. Nevertheless, returning 0 in this case
     * (i.e. the case we recurse for x == new_l) is CORRECT under the following
     * interpretation: Assume that 'this' represents nonterminal N.
     * DEFINITION: A "minimal" parse tree of yield L derived from N is a tree
     * that does not have a strict subtree that is also derived from N, and
     * which also has yield of length L.
     * When we count the number of trees derived from N that can produce a
     * yield of length L, we are interested only in those trees that are
     * "minimal". We cannot count arbirtary (not just "minimal") trees because
     * in that case it would be possible that certain strings would have an
     * infinite number of parse trees, which is undesirable for counting. 
     * The reason for this is the following:
     * FACT: if a string has a parse tree which is not minimal, then that
     * string has infinite number of parse trees.
     * Proof: Assume s is a string derived from N that has a non-minimal
     * pasrse tree. Then there is a derivation N ->* w1 N w2 ->* s, where
     * both w1 and w2 can derive the empty string. Then we can derive s in an
     * infinite number of ways:
     * N ->* s; N ->* w1 N w2 ->* s; N ->* w1 N w2 ->* w1 w1 N w2 w2 ->* s; ...
     * IMPORTANT: this assumes that the parser produces "minimal" trees.
     */
    assert(_num_matches[new_l] == 0);
    MPInt num_new_l = 0;
    for(std::vector<Rule*>::iterator it = rules.begin();
	it != rules.end(); ++it) {
      Rule *r = *it;
      num_new_l += r->memoize_num_matches(new_l, max_len);
    }
    _num_matches[new_l] = num_new_l;
  }
}

//called only from GRule::memoize
const MPInt& GRule::memoize_num_w_matches(unsigned int start,
				 unsigned int len, unsigned int max_len)
{
  MPInt res = 0;
  assert(len <= max_len);
  assert (start <= children.size());
  if (start == children.size()) {
    if (len == 0)
      return MPInt_one;//one way to match the empty string
    return MPInt_zero;
  }
  /*********************/
  assert (start < children.size());
  assert (children.size() != 0);
  {
    assert(CHECKSIZES ||
	   (c_num_matches.size() == 0 && c_memoized_size.size() == 0));
    if (c_memoized_size.size() == 0) {
      c_memoized_size.resize(children.size(), 0);
      c_num_matches.resize(children.size() - 1);
      for (unsigned int c = 0; c < c_num_matches.size(); ++c)
	c_num_matches[c].resize(max_len + 1 , 0);
    }
  }
  assert(CHECKSIZES);
  assert(start >= c_num_matches.size() ||
	 c_num_matches[start].size() == max_len + 1);
  if(len >= c_memoized_size[start]) {
    unsigned int old_size = c_memoized_size[start];
    unsigned int memoized_size = c_memoized_size[start] = len + 1;
    
    for (unsigned int new_l = old_size; new_l < memoized_size; ++new_l) {
      /*********************/
      Nonterminal *N = children[start]; assert(N);  
      for (unsigned int i = 0; i <= new_l; ++i) {
	MPInt n0 = N->memoize_num_matches(i, max_len);
	// TBD:check this: I am unsure if it is OK to skip
	// the recursive memoize_num_w_matches(children, start+1, new_l - i);
	if (n0 != 0) { //avoid useless bignum operations
	  MPInt n1 = memoize_num_w_matches(start+1, new_l - i, max_len);
	  res += n0*n1;
	}
      }
      if (start < c_num_matches.size())
	c_num_matches[start][new_l] = res;
      // else use the info in children[start] ...
    }
  }
  assert(len < c_memoized_size[start]);
  return C_NUM_MATCHES(start, len);
}

void GRule::memoize(unsigned int len, unsigned int max_len) {
  // ensure that _num_matches[i] is valid for i in [0,1,...,len]
  if (_num_matches.size() == 0)
    _num_matches.resize(max_len + 1, 0);
  assert(max_len + 1 == _num_matches.size());
  assert(len <= max_len);
  assert(memoized_size <= len);
  
  unsigned int old_size = memoized_size;
  memoized_size = len + 1;
  
  for (unsigned int new_l = old_size; new_l < memoized_size; ++new_l) {
    assert(_num_matches[new_l] == 0);
    if (new_l >= terminal_length) {
      /* See the NOTE/comment regarding recursion from Nonterminal::memoize */
      MPInt num_new_l = memoize_num_w_matches(0,
					    new_l - terminal_length,max_len);
      _num_matches[new_l] = num_new_l;
    }
  }
}

std::string GRule::toString() const {
  std::string res = left->name + " ::";
  for(std::vector<Symbol*>::const_iterator is = right.begin();
      is != right.end(); ++is) {
    Symbol *s = *is;
    res = res + " " + s->name;
  }
  return res;
}

std::string GRule::GParseTree::flatten() const {
  std::string res(""); //TBD:perf
  unsigned int c = 0;
  for(std::vector<Symbol*>::const_iterator is = gr->right.begin();
      is != gr->right.end(); ++is) {
    Symbol *s = *is;
    if (s->is_terminal)
      res = res + s->name;
    else
      res = res + children[c++]->flatten();
  }
  assert(c == children.size());
  return res;
}

void GRule::GParseTree::accept(class ParseTreePrePostVisitor *visitor) const {
  visitor->pre_visit(this);

  unsigned int c = 0;
  for(std::vector<Symbol*>::const_iterator is = gr->right.begin();
      is != gr->right.end(); ++is) {
    Symbol *s = *is;
    if (s->is_terminal) {
      Terminal* t = static_cast<Terminal*> (s);
      t->accept(visitor);
    }
    else
      children[c++]->accept(visitor);
  }
  assert(c == children.size());
#if 0  
  for(std::vector<ParseTree*>::const_iterator ic = children.begin();
      ic != children.end(); ++ic) {
    const ParseTree *child = *ic;
    child->accept(visitor);
  }
#endif
  visitor->post_visit(this);
}

std::string LRule::LParseTree::flatten() const {
  return std::string(yield, yield_length);
}
std::string L1Rule::LParseTree::flatten() const {
  //std::cout << "L1Rule::LParseTree::flatten's yield=" << yield << std::endl;;
  std::string res = std::string(yield, yield_string_length);
  //std::cout << "L1Rule::LParseTree::flatten's res=" << res << std::endl;;
  return res;
}

void LRule::LParseTree::accept(class ParseTreePrePostVisitor *visitor) const {
  visitor->pre_visit(this);
  visitor->post_visit(this);
}
void L1Rule::LParseTree::accept(class ParseTreePrePostVisitor *visitor) const {
  visitor->pre_visit(this);
  visitor->post_visit(this);
}

void Terminal::accept(class ParseTreePrePostVisitor *visitor) const {
  visitor->pre_visit(this);
  visitor->post_visit(this);
}

//TBD: move to some utility file
char* byte_copy(const void *src, size_t n) {
  char *res = (char*)malloc(n*sizeof(*res));
  return (char*) memcpy(res, src, n);
}

void PTYieldVisitor::post_visit(const Terminal *t) {
  stk.push(t->name);
}

bool should_separate(std::string& s1, std::string& s2) {
  return !(s1.empty() || s2.empty())
    && !isspace(s1[s1.length() - 1]) && !isspace(s2[0]);
    //!isspace(s1.back()) &&!isspace(s2.front());
}

void PTYieldVisitor::post_visit(const GRule::GParseTree *pt) {
  std::string res(""); //TBD:perf
  //results on stack, rightmost on top
  int size_right = pt->gr->right.size();
  for (int i = 0; i < size_right; ++i) {
    if (should_separate(stk.top(), res) )
      res = stk.top() + separator + res;
    else
      res = stk.top() + res;
    stk.pop();
  }
  stk.push(res);
}

void PTYieldVisitor::post_visit(const LRule::LParseTree *pt) {
  stk.push(std::string(pt->yield, pt->yield_length));
}
void PTYieldVisitor::post_visit(const L1Rule::LParseTree *pt) {
  stk.push(std::string(pt->yield, pt->yield_string_length));
}
