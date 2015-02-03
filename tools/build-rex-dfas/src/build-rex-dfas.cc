/*-----------------------------------------------------------------------------
 * File:    build-rex-dfas.cc
 * Author:  Daniel Luchaup
 * Date:    June 2014
 * Copyright 2010- 2014 Daniel Luchaup luchaup@cs.wisc.edu
 *
 * This file contains unpublished confidential proprietary work of
 * Daniel Luchaup, Department of Computer Sciences, University of
 * Wisconsin--Madison.  No use of any sort, including execution, modification,
 * copying, storage, distribution, or reverse engineering is permitted without
 * the express written consent (for each kind of use) of Daniel Luchaup.
 *
 *---------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <vector>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <assert.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <errno.h>
#include <assert.h>
#include <pthread.h>

#include "zoptions.h"
#include "dbg.h"
#include "MyTime.h"
#include "dfa_tab_t.h" //for minimMDFA : TBD: may want to get rid of it...
#include "utils.h"
#include "ranker.h"
#include "ffa.h"

static std::string action;

/*-----------------------------------------------------------------------------
 *get_options
 *---------------------------------------------------------------------------*/
const char* usage= //The following is the usage message
"Usage:\n\
  build-rex-dfas -zsigFile:<RegexFile> -z dumpSigMdfa:smdfa\
 where:\
  RegexFile is a file with regular expressions; each a line of the form /.../\
";

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

using namespace std;

///////////////////////////////////////////////////////////////////////////////
// build one DFA per signature
void build_signature_dfas() {
  MyTime tm("build_signature_dfas");
  /* It shouldn't matter whether we do Moore minimization or not, because
     we convert a simple *single* regular expression to a DFA.
  */
  
  string dumpSigMdfa = zop.dbgStringOption("dumpSigMdfa");
  FILE *file = zop.optFILE("sigFile");
  dfa_tab_t *sig_dfa = NULL;
  char * line = NULL;
  size_t len = 0;
  ssize_t read;
  unsigned int lines_processed = 0;

  vector<unsigned int> failed;
  
  while ((read = getline(&line, &len, file)) != -1) {
    lines_processed++;
    MyTime tm(lines_processed, "line");
    std::stringstream out;
    out << "line_" << lines_processed << " sig: " << line;
    if (line == NULL || line[0] != '/') {
      printf(" SKIPPING line or regexp:%d:%s\n",lines_processed, line);fflush(0);
      continue;
    }
    printf(" Adding regexp:%d:%s\n",lines_processed, line);fflush(0);
    {
      MyTime tml("reading-"+out.str());
      sig_dfa = new dfa_tab_t(line, lines_processed);
      if (sig_dfa == NULL) {
	printf(" FAILED, SKIPPING regexp:%d:%s\n",lines_processed, line);fflush(0);
	failed.push_back(lines_processed);
	continue;
      }
      sig_dfa->machine_id = lines_processed;
    }
    std::stringstream fname;
    fname << "sfa_" << dumpSigMdfa << "_" << lines_processed;
    sig_dfa->txt_dfa_dump(fname.str().c_str());
    printf(" DONE regexp:%d:[states:%d]:%s\n",
	   lines_processed, sig_dfa->num_states, line);fflush(0);
    delete sig_dfa;
  }
  if (line)
    free(line);

  if (!failed.empty()) {
    cout << " FAILED: ";
    while(!failed.empty()) {
      cout << failed.back() << " ";
      failed.pop_back();
    }
    cout << endl;
  }
}
int main(int argc, char *argv[])
{
  //pin_processors();
  DbgTime tm("main");
  zop.usage = usage;
  get_options(argc, argv);
  build_signature_dfas();
  return 0;
}
