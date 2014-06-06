/*-----------------------------------------------------------------------------
 * File:    trex.cc
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
#include <fstream>
#include <bitset>
#include "trex.h"
#include "tnfa.h"

//#include "utils.h"


/********** trex (base class) *******************/
stateNFA* trex::toNFA(stateNFA* succ) {
  stateNFA* in_snfa = new stateNFA(this, stateNFA::FWD);
  stateNFA* out_snfa = new stateNFA(this, stateNFA::BCK);
  { assert(in_snfa == in_snfa->ends[stateNFA::FWD]);
    assert(in_snfa->_type == this);
    assert(in_snfa->_dir == stateNFA::FWD);
    assert(!in_snfa->ends[stateNFA::BCK]);
    
    assert(out_snfa == out_snfa->ends[stateNFA::BCK]);
    assert(out_snfa->_type == this);
    assert(out_snfa->_dir == stateNFA::BCK);
    assert(!out_snfa->ends[stateNFA::FWD]);
  }
  
  in_snfa->ends[stateNFA::BCK] = out_snfa;
  out_snfa->ends[stateNFA::FWD] = in_snfa;
  
  if (succ)
    out_snfa->add_fwd_transition(succ);
  return in_snfa;
}

/********** trex_epsilon ************************/
stateNFA* trex_epsilon::toNFA(stateNFA* succ) {
  stateNFA* in_snfa = trex::toNFA(succ);
  in_snfa->add_fwd_transition(in_snfa->Exit());
  return in_snfa;
}

/********** trex_byte ***************************/
stateNFA* trex_byte::toNFA(stateNFA* succ) {
  stateNFA* in_snfa = trex::toNFA(succ);
  in_snfa->add_fwd_transition(in_snfa->Exit());
  return in_snfa;
}

/********** trex_byte_set ***********************/
trex_byte_set::trex_byte_set(const std::bitset<MAX_SYMS>& chars):
  trex_byte(rk_byte_set), _chars(chars) {
  length_max = 1;	
  assert(((unsigned long)0x1)<<sizeof(*_rank) <= MAX_SYMS);
  unsigned int current_rank = 0;
  for (unsigned int i=0; i < MAX_SYMS; i++) {
    if (_chars.test(i)) {
      _rank[i] = current_rank;
      _unrank[current_rank] = i;
      ++current_rank;
    }
  }
  assert(current_rank && current_rank == _chars.count());
  chars_count = current_rank;
};
/********** trex_disjunction ********************/
stateNFA* trex_disjunction::toNFA(stateNFA* succ) {
  stateNFA* in_snfa = trex::toNFA(succ);
  in_snfa->add_fwd_transition(left->toNFA(in_snfa->Exit()));
  in_snfa->add_fwd_transition(right->toNFA(in_snfa->Exit()));
  return in_snfa;
}

void trex_disjunction::update_matches(unsigned int len) {
  unsigned int old_size = _num_matches.size();
  if (old_size < len+1) {
    _num_matches.resize(len+1);
    left->update_matches(len);
    right->update_matches(len);
    for (unsigned int i = old_size; i <= len; ++i) {
      _num_matches[i] = left->num_matches(i) + right->num_matches(i);
    }
  }
}
/********** trex_conjunction ********************/
stateNFA* trex_conjunction::toNFA(stateNFA* succ) {
  stateNFA* in_snfa = trex::toNFA(succ);
  stateNFA* l_snfa = left->toNFA(NULL);
  stateNFA* r_snfa = right->toNFA(in_snfa->Exit());

  in_snfa->add_fwd_transition(l_snfa);
  l_snfa->ends[stateNFA::BCK]->add_fwd_transition(r_snfa);
  // i.e. in_snfa->add_fwd_transition(left->toNFA(right->toNFA(in_snfa->Exit())));

  return in_snfa;
}
void trex_conjunction::update_matches(unsigned int len) {
  unsigned int old_size = _num_matches.size();
  if (old_size < len+1) {
    _num_matches.resize(len+1);
    left->update_matches(len);
    right->update_matches(len);
    for (unsigned int i = old_size; i <= len; ++i) {
      _num_matches[i] = 0;
      for (unsigned int k = 0; k <= i; ++k) {
        MPInt l = left->num_matches(k);
	MPInt r = right->num_matches(i-k);
        _num_matches[i] += (l * r);//left->num_matches(k) * right->num_matches(i-k);
      }
    }
  }
}

/********** trex_range **************************/
#include <iostream>
stateNFA* trex_range::toNFA(stateNFA* succ) {
  stateNFA* in_snfa = trex::toNFA(succ);
  stateNFA* tmp = in_snfa->Exit();
  for (int i = high; i >= low; --i) {
    tmp = tr->toNFA(tmp);
    if (i < high)
      tmp->Exit()->add_fwd_transition(in_snfa->Exit());
  }
  for (int i = 1; i < low; ++i) {
    tmp = tr->toNFA(tmp);
  }
  in_snfa->add_fwd_transition(tmp);
  if (0) {
	  std::ofstream dotsNFA("dot.trex_range");
	  in_snfa->to_dot(dotsNFA); dotsNFA.close();
  }
  return in_snfa;
}

/********** trex_plus ***************************/
stateNFA* trex_plus::toNFA(stateNFA* succ) {
  stateNFA* in_snfa = trex::toNFA(succ);
  stateNFA* tr_snfa = tr->toNFA(in_snfa->Exit());
  in_snfa->add_fwd_transition(tr_snfa);
  tr_snfa->Exit()->add_fwd_transition(tr_snfa);

  return in_snfa;
}
void trex_plus::update_matches(unsigned int len) {
  unsigned int old_size = _num_matches.size();
  if (old_size < len+1) {
    _num_matches.resize(len+1, 0);
    assert(!tr->generates_epsilon());// to avoid issues for len=0
    tr->update_matches(len);
    //for trex_star: if (old_size == 0) {_num_matches[0] = 1;++old_size;}// convention: case epsilon
    for (unsigned int i = old_size; i <= len; ++i) {
      _num_matches[i] = tr->num_matches(i);
      //if (i==0) _num_matches[i] += 1;
      for (unsigned int k = 1; k < i; ++k) {
	_num_matches[i] += tr->num_matches(k) * _num_matches[i-k];
      }
    }
  }
}
//////////////////////////////////////////////////
void gather_trex_nodes(trex* type, std::set<trex*>& result) {
  if (!CONTAINS(result, type)) {
    result.insert(type);
    int num_children = type->count_children();
    for (int c = 0; c < num_children; ++c) {
      trex *child = type->child(c);
      gather_trex_nodes(child, result);
    }
  }
}
