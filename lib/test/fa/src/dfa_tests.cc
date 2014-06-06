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

void read_string_inputs(vector<string> &string_inputs, const char* ifile) {
  if (VERBOSE_LEVEL > 0) DMP(ifile);
  char * line = NULL;
  size_t len = 0;
  ssize_t read;
  unsigned int lines_processed = 0;
  FILE *file = zop.optFILE(ifile);
  lines_processed = 0;
  string_inputs.clear();
  while ((read = getline(&line, &len, file)) != -1) {
    lines_processed++;
    if (line == NULL || line[0] == '#') {
      if (VERBOSE_LEVEL > 0)
        printf("SKIPPING: on line %d; comment:%s\n",lines_processed, line);fflush(0);
      continue;
    }    
    if (VERBOSE_LEVEL > 0)
      printf(" Adding input:%d:%s\n",lines_processed, line);fflush(0);
    chomp(line);
    string_inputs.push_back(line);
  }
  fclose(file);
  if (line)
    free(line);
}
void read_string_inputs(vector<string> &string_inputs) {
  read_string_inputs(string_inputs, "rexInputFile");
}

static void rex_test(vector<string> &regular_expressions,
		     vector<string> &string_inputs) {
  int rex_id = 0;
  for (vector<string>::iterator rit =  regular_expressions.begin();
       rit !=  regular_expressions.end();
       ++rit) {
    if (stateNFA::dbg_reset_stateNFA_guid) stateNFA::g_uido = 0;
    ++rex_id;
    const std::string rex = *rit;
    if (VERBOSE_LEVEL > 0)  cout << "TESTING: REX=" << rex << endl;
    std::string suffix = toString(rex_id) + ".txt";
    //////////////////
    trex *trx = regex2trex(rex, NULL, NULL);
    std::ofstream dotout (("dot.trx."+suffix).c_str());  trx->to_dot(dotout); dotout.close();
    //////////////
    stateNFA *snfa = trx->toNFA(NULL);
    assert(snfa->is_root());
    ofstream dotsNFA(("dot.NFA-trx."+suffix+".txt").c_str());
    snfa->to_dot(dotsNFA); dotsNFA.close();
    //////////////
    dfa_tab_t * pdt = snfa->make_dfa();
///////////////////////////////////////////////////////////////////////////////
    int input_id=0;
    for (vector<string>::iterator iit =  string_inputs.begin();
	 iit !=  string_inputs.end();
	 ++iit) {
	    ++input_id;
      //MyTime tm(("Match.")+suffix+"."+toString(input_id));
      const std::string input = *iit;
      //DMP(input); (DMP(input.length())).flush();
      bool pdt_match = pdt->strict_match((unsigned char*)input.c_str(), input.length());
      if (pdt_match)
        cout << "MATCH " << rex_id << " " << input_id << endl;
      bool does_match_fwd = snfa->match(input.c_str(), input.length(), stateNFA::FWD);  //DMP(does_match_fwd);
      assert(does_match_fwd == pdt_match);
      bool does_match_bck = snfa->match(input.c_str(), input.length(), stateNFA::BCK); //DMP(does_match_bck);
      assert(does_match_bck == pdt_match);
      cout.flush();
    }
///////////////////////////////////////////////////////////////////////////////
    stateNFA::delete_all_reachable(snfa);
    delete trx;
    delete pdt;
  }
}

void trex_tests() {
  //MyTime tm("run_tests");
  vector<string> regular_expressions;
  read_regular_expressions(regular_expressions);
  vector<string> string_inputs;
  read_string_inputs(string_inputs);
  stateNFA::dbg_reset_stateNFA_guid = true;
  rex_test(regular_expressions,string_inputs);
}

//MAIN
static std::string action;

/*-----------------------------------------------------------------------------
 *get_options
 *---------------------------------------------------------------------------*/
const char*
usage="dfa_tests -zrexExprFile:<REX-in-file> -zrexInputFile:<string-input-file>";

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
  trex_tests();
  return 0;
}
