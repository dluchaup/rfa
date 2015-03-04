/*-----------------------------------------------------------------------------
 * File:    rex-intersect.cc
 * Author:  Daniel Luchaup
 * Date:    April 2014
 * Copyright 2014 Daniel Luchaup luchaup@cs.wisc.edu
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
#include "dfa_tab_t.h"
#include "utils.h"
#include "ranker.h"
#include "ffa.h"

using namespace std;
static std::string action;

/*-----------------------------------------------------------------------------
 *get_options
 *---------------------------------------------------------------------------*/
const char* usage= //The following is the usage message
"Usage:\n\
  rex-intersect <rex-file>\n\
";

void read_string_inputs(FILE *file, vector<string> &string_inputs) {
  char * line = NULL;
  size_t len = 0;
  ssize_t read;
  unsigned int lines_processed = 0;
  lines_processed = 0;
  string_inputs.clear();
  while ((read = getline(&line, &len, file)) != -1) {
    lines_processed++;
    if (line == NULL || line[0] == '#') {
      //printf("SKIPPING: on line %d; comment:%s\n",lines_processed, line);fflush(0);
      continue;
    }    
    //printf(" Adding input:%d:%s\n",lines_processed, line);fflush(0);
    chomp(line);
    string_inputs.push_back(line);
  }
  fclose(file);
  if (line)
    free(line);
}

static bool do_intersect(vector<string> &regular_expressions) {
  int rex_id = 0;
  dfa_tab_t *pdt_intersect = NULL;
  for (vector<string>::iterator rit =  regular_expressions.begin();
       rit !=  regular_expressions.end();
       ++rit) {
    ++rex_id;
    const std::string rex = *rit;
    if (pdt_intersect == NULL)
      pdt_intersect = new dfa_tab_t(rex);
    else {
      dfa_tab_t dt(rex);
      dfa_tab_t *old_intersect = pdt_intersect;
      pdt_intersect = dt_intersection(pdt_intersect, &dt);
      delete old_intersect;
      if (pdt_intersect->num_states == 0)
	break;
    }
  }
  bool result = (pdt_intersect && pdt_intersect->num_states != 0);
  if (pdt_intersect)
    delete pdt_intersect;
  return result;
}

int main(int argc, char *argv[])
{
  DbgTime tm("main");
  if (argc != 2)
    zop.usage_and_die("Missing file argument.");

  const char* fname = argv[1];
  FILE *f = fopen(fname, "r");
  if (f == NULL) {
    perror("Cannot open file ");
    zop.usage_and_die(((string(" specified a bad  file:") + fname)).c_str());
  }

  vector<string> regular_expressions;
  read_string_inputs(f, regular_expressions);

  bool intersect = do_intersect(regular_expressions);
  std::cout << intersect << std::endl;
  return intersect? 0 : 1;
}
