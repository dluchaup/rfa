/*-----------------------------------------------------------------------------
 * File:    dfa_tests.cc
 * Author:  Daniel Luchaup
 * Date:    28 May 2014
 * Copyright 2014 Daniel Luchaup luchaup@cs.wisc.edu
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
#include "tnfa.h"
#include "ranker.h"
#include "ffa.h"

#include "zoptions.h"
#include "dbg.h"
#include "MemoryInfo.h"

using namespace std;
#define VERBOSE_LEVEL 0
void read_regular_expressions(vector<string> &regular_expressions) {
  FILE *file = zop.optFILE("rexExprFile");
  char * line = NULL;
  size_t len = 0;
  ssize_t read;
  unsigned int lines_processed = 0;
  regular_expressions.clear();
  
  while ((read = getline(&line, &len, file)) != -1) {
    lines_processed++;
    if (line == NULL || line[0] != '/') {
      if (VERBOSE_LEVEL > 0)
        printf(" SKIPPING line or regexp:%d:%s\n",lines_processed, line);fflush(0);
      continue;
    }
    if (VERBOSE_LEVEL > 0)
      printf(" Adding regexp:%d:%s\n",lines_processed, line);fflush(0);
    chomp(line);
    regular_expressions.push_back(line);
  }
  
  fclose(file);
  if (line)
    free(line);
}

void rank_tests() {
  MyTime tm("rank_tests");
  vector<string> regular_expressions;
  read_regular_expressions(regular_expressions);

  int rex_id = 0;

  bool do_trx = zop.dbgBoolOption("do_trx");

  unsigned int repeat = zop.dbgNumOption("repeat", 100);
  MemoryInfo mi; mi.read();  mi.dump("initial-memory");
  
  for (vector<string>::iterator rit =  regular_expressions.begin();
       rit !=  regular_expressions.end();
       ++rit) {
    ++rex_id;
    const std::string rex = *rit;
    cout << endl << "TESTING: REX=" << rex << endl;
    
    if (unsigned int forced_len = zop.dbgNumOption("forced_len", 0)) {
        // DFA: old GS
	rank_dfa_tab_t rdt(rex, forced_len);
	rdt.ranker_report_size("constructed-udated-rdt", 1);
	MPInt dfa_num_matches = rdt.num_matches(forced_len);
	DMPNL(dfa_num_matches);
	rdt.test_unrank_rank(forced_len, repeat);
	rdt.ranker_report_size("used-rdt", 1);

	// FFA: new GS-based
	rank_ffa_tab_t rft(rex, forced_len);
	rft.ranker_report_size("constructed-udated-rft", 1);
	MPInt ffa_num_matches = rft.num_matches(forced_len);
	DMPNL(ffa_num_matches);
	assert(ffa_num_matches == dfa_num_matches);
	rft.test_unrank_rank(forced_len, repeat);
	rft.ranker_report_size("used-rft", 1);
      
        // NFA
	rank_nfa_tab_t rnf(rex, forced_len);
        rnf.ranker_report_size("constructed-udated-rnf", 1);
	MPInt nfa_num_matches = rnf.num_matches(forced_len);
	DMPNL(nfa_num_matches);
	assert(nfa_num_matches >= dfa_num_matches);
	if (nfa_num_matches > dfa_num_matches) {
          float nfa_ambig = MPdiv2float(nfa_num_matches, dfa_num_matches);
	  DMPNL(nfa_ambig);
	  zop.dbgOptionSet.insert(std::string("allow_ambiguous"));
	}
	rnf.test_unrank_rank(forced_len, repeat);
	rnf.ranker_report_size("used-rnf", 1);
      
	if (do_trx) {
	  rank_trx rtrx(rex, forced_len);
	  rtrx.ranker_report_size("constructed-udated-rtrx", 1);
	  rtrx.test_unrank_rank(forced_len, repeat);
	  rtrx.ranker_report_size("used-rtrx", 1);
	}
    }
  }
}

//MAIN
static std::string action;

/*-----------------------------------------------------------------------------
 *get_options
 *---------------------------------------------------------------------------*/
const char*
usage="fa_rank_tests -zrexExprFile:<REX-file> -zforced_len:<MaxLen> [-zallow_ambiguous] [-zrepeat:<Count>]";

void get_options(int argc, char *argv[])
{
  int c;
  extern char *optarg;
  
  while ((c = getopt(argc, argv, "z:")) != -1) {
    switch (c)
      {
      case 'z':
	zop.dbgOptionSet.insert(std::string(optarg));
	break;
      default:
	zop.usage_and_die("unknown option");
	break;
      }
  }
  
  return;
}
///////////////////////////////////////////////////////////////////////////////
void init(int argc, char *argv[]) {
  // Debug: dump the arguments
  std::cout << "Command line: ";
  for (int i = 0; i < argc; ++i) {
    std::cout << argv[i] << " ";
  }
  std::cout << std::endl;
  zop.dumpOptions();
}

int main(int argc, char *argv[])
{
  //MyTime tm("main");
  zop.usage = usage;
  get_options(argc, argv);
  rank_tests();
  return 0;
}
