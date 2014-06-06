/*-----------------------------------------------------------------------------
 * File:    trex_dbg.cc
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
#include "trex.h"
#include "escape_sequences.h" // for output_char
#include "dbg.h"
//#include "misc.h"

/********** trex (base class) *******************/
void trex::to_dot(std::ostream& dio) {
  dio << "digraph trex {\n";
  dot(dio);
  dio << "}\n";
}
void trex::label_dot(std::ostream& dio) {
  dio << uid << "[label=\""; dbg_dot(dio); common_dbg_dot(dio); dio << "\"]\n";
  //if (Exit())  Exit()->dot(dio);
}
void trex::common_dbg_dot(std::ostream& dio) {
  const char * nl = "\\n";
  //dio << nl << has_plus() << "," << generates_epsilon();
#if 0
  dio << nl;
  for (unsigned int i = 0; i < 10; ++i) {
    dio << "#" << i << ":" << num_matches(i) << "; ";
  }
  dio << nl;
#endif
  if (_num_matches.size())
    dio << nl << "nm= " << _num_matches.size();
  //if (Exit()) dio << ";A(" << Exit()->uid << ")";
  //show_frontier(dio);
}

/********** trex_epsilon ************************/
std::string trex_epsilon::unparse() {
  std::string su="";
  return su;
}
void trex_epsilon::dbg_dot(std::ostream& dio) {
  dio << uid << ":/" << unparse() << "/";
}
void trex_epsilon::dot(std::ostream& dio) {
  label_dot(dio);
  //if (Exit()) dio << uid << " -> " << Exit()->uid << "\n";
}

/********** trex_byte ***************************/
std::string trex_byte::unparse() {
  std::string su = output_char2string(b);
  return su;
}
void trex_byte::dbg_dot(std::ostream& dio) {
  dio << uid << ":/" << unparse() << "/";
}
void trex_byte::dot(std::ostream& dio) {
  label_dot(dio);
}

/********** trex_byte_set************************/
std::string trex_byte_set::unparse() {//See char_class::unparse(FILE *out, int indent)
  std::string su;
  if (_chars.count() == MAX_SYMS)
    su = ".";
  else {
    bool test_val = true;
    su = "[";
    if (_chars.count() < MAX_SYMS/2) {
      su = su +"^";
      test_val = false;
    }
    for (unsigned int i=0; i < MAX_SYMS; i++) {
      if (_chars.test(i) == test_val)
	su = su + output_char2string(i);
    }
    su = su + "]";
  }
  return su;
}

/********** trex_disjunction ********************/
std::string trex_disjunction::unparse() {
  std::string sul = left->unparse();
  std::string sur = right->unparse();
  std::string su = sul+"|"+sur;
  return su;
}
void trex_disjunction::dbg_dot(std::ostream& dio) {
  dio << "| " << uid << ":/" << unparse() << "/";
}
void trex_disjunction::dot(std::ostream& dio) {
  label_dot(dio);
  dio << uid << " -> " << left->uid  << " [label=\"0|\"]\n";
  dio << uid << " -> " << right->uid << " [label=\"|1\"]\n";
  left->dot(dio);
  right->dot(dio);
}

/********** trex_conjunction ********************/
std::string trex_conjunction::unparse() {
  std::string sul = left->unparse();
  std::string sur = right->unparse();
  std::string su = "("+sul+")("+sur+")";
  return su;
}
void trex_conjunction::dbg_dot(std::ostream& dio) {
  dio << "& " << uid << ":/" << unparse() << "/";
}
void trex_conjunction::dot(std::ostream& dio) {
  label_dot(dio);
  dio << uid << " -> " << left->uid << " [label=0]\n";
  dio << uid << " -> " << right->uid << " [label=1]\n";
  left->dot(dio);
  right->dot(dio);
}

/********** trex_range **************************/
std::string trex_range::unparse() {
  std::string sut = tr->unparse();
  std::string su = sut+"{"+toString(low) + ","+ toString(high)+"}";
  return su;
}
void trex_range::dbg_dot(std::ostream& dio) {
  dio << "{..}" << uid << ":/" << unparse() << "/";
}
void trex_range::dot(std::ostream& dio) {
  label_dot(dio);
  dio << uid << " -> " << tr->uid << "\n";
  tr->dot(dio);
}

/********** trex_plus ***************************/
std::string trex_plus::unparse() {
  std::string sut = tr->unparse();
  std::string su = "("+sut+")+";
  return su;
}
void trex_plus::dbg_dot(std::ostream& dio) {
  dio << "+ " << uid << ":/" << unparse() << "/";
}
void trex_plus::dot(std::ostream& dio) {
  label_dot(dio);
  dio << uid << " -> " << tr->uid << "\n";
  tr->dot(dio);
}
