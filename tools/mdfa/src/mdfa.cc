/*-----------------------------------------------------------------------------
 * mdfa.cc
 * author:  Daniel Luchaup
 * date:    2011 - 2014
 * 
 * Implementation of the signature grouping algorithm in the paper
 *      "Fast and Memory-Efficient Regular Expression Matching for
 *       Deep Packet Inspection", by Fang Yu, Zhifeng Chen, Yanlei Diao
 *
 *    Copyright 2011 Author: Daniel Luchaup
 *
 *    This file contains unpublished confidential proprietary
 *    work of the author, Department of Computer Sciences, 
 *    University of Wisconsin--Madison.  No use of any sort, including 
 *    execution, modification, copying, storage, distribution, or reverse 
 *    engineering is permitted without the express written consent of 
 *    the authors.
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
#include <map>
#include <iostream>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <errno.h>
#include <assert.h>
#include <pthread.h>

#include "dfa_tab_t.h"
#include "dbg.h"
#include "zoptions.h"
#include "dout.h"
#include "trex.h"
#include "utils.h"
#include "MyTime.h"

#include "mm.h"
#include "misc.h" //Hack: after mm.h

Dout dout; //possibly split output: debug logging, and to certain files

#define MDFASTATES 150000
#define DEFAULT_FALSE_POSITIVES 0.015

using namespace std;

static string action;
static unsigned int numThreads=0;
static unsigned int condorProcess=1;
static unsigned int numCondorProcesses=1;
static bool readDfaTree(FILE *logf);

static bool enable_Moore_minimization_unless_option_overwritten() {
  bool old_minimMDFA = minimMDFA;
  if (!zop.dbgDefinedOption("minimMDFA")) {
    cout << " Undefined minimMDFA. Set it to True!" << endl;
    minimMDFA = true;
    DMP(minimMDFA) << endl;
  } else if (!minimMDFA)
    cout << "WARNING! minimMDFA is set to FALSE! Minimization will not respect Moore DFAs!";
  return old_minimMDFA;
}

static bool disable_Moore_minimization_unless_option_overwritten() {
  bool old_minimMDFA = minimMDFA;
  if (!zop.dbgDefinedOption("minimMDFA")) {
    cout << " Undefined minimMDFA. Set it to False!" << endl;
    minimMDFA = false;
    DMP(minimMDFA) << endl;
  } else if (minimMDFA)
    cout << "WARNING! minimMDFA is set to TRUE! Minimization will respect Moore DFAs!";
  return old_minimMDFA;
}

static string get_name_of_bdfa_tree_file() {
  string files2botch = zop.dbgStringOption("BOTCH");
  string res = "bdfa-tree."+files2botch;
  return res;
}

struct BotchInfo {
  map<string, string> botches;         // botches[BotchedFile] = File
  map<string, vector<string> > groups;
} botchInfo;

bool readConflictGraph(vector<vector<int> > &conflicts) {
  //if (zop.dbgStringOption("CG").empty()) return false;
  assert(conflicts.empty());
  conflicts.clear(); //paranoic
  FILE *cgf = zop.optFILE("CG");
  
  char reason[20];
  int r;
  
  unsigned int sig1,sig2;//signatures
  state_id_t n12,n1,n2;
  map<unsigned int, state_id_t> sizes;
  state_id_t maxState = 0;
  while((r = fscanf(cgf,"%s %d with %d : %d %d %d",
		    reason, &sig1, &sig2, &n12, &n1, &n2)) == 6) {
    //cout << " got " << reason << " "  << sig1 << " "  << sig2 << " "  << n12 << " "  << n1 << " "  << n2 << " "  << endl;
    //assert(sig1 >= 0 && sig2 >= 0);
    assert(n12 > 0 && n1 > 0 && n2 > 0);
    
    maxState = max(sig1, maxState);
    maxState = max(sig2, maxState);
    if (!CONTAINS(sizes, sig1)) sizes[sig1] = n1;
    if (!CONTAINS(sizes, sig2)) sizes[sig2] = n2;
    assert(sizes[sig1] == n1);
    assert(sizes[sig2] == n2);
  }
  DMP(maxState);DMP(sizes.size());
  assert(r == EOF);
  rewind(cgf);
  
  conflicts.resize(maxState+1);
  DMP(conflicts.size()) << endl;
  for (sig1 = 0; sig1 <= maxState; ++sig1) {
    conflicts[sig1].resize(maxState+1, 0); //... and initialized to 0
  }
  
  while((r = fscanf(cgf,"%s %d with %d : %d %d %d",
		    reason, &sig1, &sig2, &n12, &n1, &n2)) == 6) {
    if(!(conflicts[sig1][sig2] == 0 && conflicts[sig2][sig1] == 0)) {
      cout << endl;
      DMP(sig1);DMP(sig2);DMP(n1);DMP(n2);DMP(n12);
      DMP(conflicts[sig1][sig2]);DMP(conflicts[sig2][sig1]);
      cout.flush();
    }
    
    assert(conflicts[sig1][sig2] == 0 && conflicts[sig2][sig1] == 0);
    if (string(reason) == "COMPOSE")
      conflicts[sig1][sig2] = -n12;
    else
      conflicts[sig1][sig2] = n12;
    conflicts[sig2][sig1] = conflicts[sig1][sig2];
  }
  
  if (0) {//debugging
    cout << endl;
    for (sig1 = 0; sig1 <= maxState; ++sig1) {
      cout << sig1 << ":\t";
      for (sig2 = 0; sig2 <= maxState; ++sig2)
	cout << (conflicts[sig1][sig2] > 0) ;//<< " ";
      cout << endl;
    }
    if (0) {
      for (sig1 = 0; sig1 <= maxState; ++sig1) {
	for (sig2 = 0; sig2 <= maxState; ++sig2)
	  cout << sig1 << ":" << sig2 << ":" << (conflicts[sig1][sig2] > 0) << " ";
	cout << endl;
      }
    }
  }
  
  return true;
}

/*  The group file structure is:
    <group1>
    <group2>
    ...
    where each <groupK> is a line containing the list of signature identifiers:
    id1  id2 ....
*/
bool maybeReadGroups(vector<set<int> > &groups) {
  if (zop.dbgStringOption("GROUP").empty())
    return false;
  groups.clear();
  FILE *gf = zop.optFILE("GROUP");
  
  char * line = NULL;
  size_t len = 0;
  ssize_t read;
  while ((read = getline(&line, &len, gf)) != -1) {
    chomp(line);
    std::stringstream ss;
    ss << line;
    cout <<  line << " ..." << endl;
    set<int> grp;
    grp.clear();
    while(!ss.eof()) {
      int i;
      if ((ss >> i).fail())
	break;
      assert(!CONTAINS(grp, i));
      grp.insert(i);
      //cout << " inserted i= " << i << endl;
    }
    assert(!grp.empty());
    if (1) {
      cout << "group " << " is: ";
      for (set<int>:: const_iterator it = grp.begin(); it != grp.end(); ++it) {
	cout << *it << " ";
      }
      cout << endl;
    }
    
    groups.push_back(grp);
  }
  if (line)
    free(line);

  if (1) {//dbg
    cout << "#groups= " << groups.size() << endl;
    for (unsigned int g = 0;  g < groups.size(); ++g) {
      cout << "group " << g <<" : ";
      const set<int> & grp = groups[g];
      for (set<int>:: const_iterator it = grp.begin(); it != grp.end(); ++it) {
	cout << *it << " ";
      }
      cout << endl;
    }
  }
  
  return true;
}

bool maybeReadSkipSigs(set<int> &skip) {
  if (zop.dbgStringOption("skip").empty())
    return false;
  skip.clear();
  FILE *gf = zop.optFILE("skip");
  
  char * line = NULL;
  size_t len = 0;
  ssize_t read;
  while ((read = getline(&line, &len, gf)) != -1) {
    chomp(line);
    std::stringstream ss;
    ss << line;
    cout <<  line << " ..." << endl;
    set<int> grp;
    grp.clear();
    while(!ss.eof()) {
      int i;
      if ((ss >> i).fail())
	break;
      skip.insert(i);
    }
  }
  if (line)
    free(line);

  if (1) {//dbg
    cout << "#skip= " << skip.size() << ":: ";
    for (set<int>::iterator it = skip.begin(); it != skip.end(); ++it) {
      cout << * it <<" , ";
    }
    (cout << endl).flush();
  }
  return true;
}

class dfaTree {
public:

  struct scanStats{
    unsigned long long input_size;
    unsigned long long total_scanned_length;
    unsigned long long total_scanned_time;
    unsigned long long total_matches;
    unsigned long long leaf_total_matches;
    unsigned long long int total_memory_steps;
    unsigned long long int total_memory_accesses;
    unsigned int roots;
    unsigned int leaves;
    unsigned long long int nodes;
    unsigned long long int totalStates;
    unsigned long long int leavesStates;

#if PARANOIA > 0
    unsigned long long dbg_real_total_matches;
#endif
    
    void reset() {
      input_size = 0;
      total_scanned_length = 0;
      total_scanned_time = 0;
      total_matches = 0;
      leaf_total_matches = 0;
      total_memory_steps = 0;
      total_memory_accesses = 0;
      roots = 0;
      leaves = 0;
      nodes = 0;
      totalStates = 0;
      leavesStates = 0;
#if PARANOIA > 0
      dbg_real_total_matches = 0;
#endif
    }
    scanStats() {reset();}
    void dumpStats() {
      cout << endl;
      DMP(input_size);
      DMP(total_scanned_length);
      DMP(total_scanned_time);
      DMP(total_matches);
      DMP(leaf_total_matches);
      DMP(total_memory_steps);
      DMP(total_memory_accesses);
      DMP(roots);
      DMP(leaves);
      DMP(nodes);
      cout << "internal-nodes " << nodes - leaves << " ";
      DMP(totalStates);
      DMP(leavesStates);
      cout << " overheadStates= " << totalStates - leavesStates
	   << " i.e. overhead= " << (totalStates - leavesStates)*100.0/leavesStates << " %";
#if PARANOIA > 0
      DMP(dbg_real_total_matches);
#endif    
      cout << endl;
      cout.flush();
    }
  };

  struct treeStats{
    unsigned int roots;
    unsigned int leaves;
    unsigned int nodes;
    vector<int> nodesByDepth;//0 at root
    void reset() {
      roots=0;
      leaves=0;
      nodes=0;
      nodesByDepth.clear();
    }
    treeStats() {reset();}
    void dumpStats() {
      cout << endl;
      DMP(roots);DMP(leaves);DMP(nodes);DMP(nodesByDepth.size());
      DMPC(nodesByDepth);
      cout << endl;
      cout.flush();
    }
  };
  
  void getTreeStats(struct treeStats &ts,
		    int &height, unsigned int depth, Dout &sout) {
    cout << " getTreeStats: " << fileName << endl;
    //dt.txt_dfa_load(fileName.c_str());
    if (depth == 0)
      ts.roots++;
    if (children.empty())
      ts.leaves++;
    if (ts.nodesByDepth.size() < depth + 1) {
      ts.nodesByDepth.push_back(0);
    }
    ts.nodesByDepth[depth]++;
    ts.nodes++;
    sout << fileName << ";" << ENDL;
    for (vector<dfaTree*>::iterator it = children.begin();
	 it != children.end();
	 ++it) {
      sout << fileName << " -> " << (*it)->fileName << ";" << ENDL;
      (*it)->getTreeStats(ts, height, depth+1, sout);
    }
  }

  static void getTreeStatsAll(bool dotDump) {
    readDfaTree(zop.optFILE("logf"));
    Dout sout((zop.dbgStringOption("logf") + ".dot").c_str());
    struct treeStats ts;
    MyTime tm("getTreeStatsAll");
    
    if (dotDump) {
      sout << "digraph logf {" << ENDL;
      for (vector<dfaTree*>::iterator it = roots.begin();
	   it != roots.end(); ++it) {
	dfaTree *t = *it;
	int height=0;
	t->getTreeStats(ts, height, 0, sout);
      }
      sout << " }" << ENDL;
    }
    ts.dumpStats();
  }
  
  string fileName;
  dfaTree *parent;
  dfa_tab_t dt;
  MMStandard *mm;
  vector<dfaTree*> children;
  
  static vector<dfaTree*> roots;
  static vector<dfaTree*> leaves;
  
  dfaTree(string name, dfaTree *parent0) : fileName(name), parent(parent0), mm(NULL) {
    //dt.txt_dfa_load(fileName.c_str());
    if (!parent)
      roots.push_back(this);
  }
  
  ~dfaTree() {
    if (mm)
      delete mm;
  }

  void do_false_matches_stats() {
    unsigned char buf[MAX_PACKET_SIZE];
    unsigned long packets_extracted = 0;
    int size = 0;
    int fd = zop.optFileDescriptor("PAYLOAD");
    // avoid using using this->mm, in order to preserve mm's statistics
    MMStandard mm0(&dt);
    
    unsigned long long int this_matches = 0;
    unsigned long long int some_child_matches = 0;
    unsigned long long int cumulative_child_matches = 0;

    input_size = 0;
    while(read_packet(fd, buf, size)) {
      input_size += size;
      packets_extracted ++;
      if (mm0.match(buf,size))
	++this_matches;
      bool a_child_matches = false;
      for (vector<dfaTree*>::iterator it = children.begin();
	   it != children.end();
	   ++it) {
	MMStandard mmchild(&(*it)->dt);
	if (mmchild.match(buf, size)) {
	  ++cumulative_child_matches;
	  a_child_matches = true;
	}
      }
      if (a_child_matches)
	++some_child_matches;
    }
    close(fd);
    cout << " :STATS: "
	 << " this: " << ((float)this_matches/packets_extracted)
	 << " some_child: " << ((float)some_child_matches/packets_extracted)
	 << " sum_children: " << ((float)cumulative_child_matches/packets_extracted)
	 << " .";
  }
  
  void dbg_print(string msg) {
    cout << msg << fileName;
    do_false_matches_stats();
    cout << endl;
    for (vector<dfaTree*>::iterator it = children.begin();
	 it != children.end();
	 ++it) {
      (*it)->dbg_print(msg+"     ");
    }
  }
  
  void load() {
    cout << " Loading: " << fileName << endl;
    dt.txt_dfa_load(fileName.c_str());
    mm = new MMStandard(&dt);
    for (vector<dfaTree*>::iterator it = children.begin();
	 it != children.end();
	 ++it) {
      (*it)->load();
    }
  }
  
  void get_stats(struct scanStats &stats) {
    stats.total_scanned_length += mm->total_scanned_length;
    stats.total_scanned_time += mm->total_scanned_time;
    stats.total_matches += mm->total_matches;
    stats.total_memory_steps += mm->total_memory_steps;
    stats.total_memory_accesses += mm->total_memory_accesses;
    stats.nodes +=1;
    stats.totalStates += dt.num_states;
    if (children.empty()) {//leaf
      stats.leaves += 1;
      stats.leavesStates += dt.num_states;
      stats.leaf_total_matches += mm->total_matches;
    }
    
    for (vector<dfaTree*>::iterator it = children.begin();
	 it != children.end();
	 ++it) {
      (*it)->get_stats(stats);
    }
  }
  
  void reset_stats() {
    mm->reset_stats();
    for (vector<dfaTree*>::iterator it = children.begin();
	 it != children.end();
	 ++it) {
      (*it)->reset_stats();
    }
  }
  
  void get_leaves() {
    if (children.empty()) {
      //cout << "(leafe)" << fileName << " ";
      leaves.push_back(this);
    }
    for (vector<dfaTree*>::iterator it = children.begin();
	 it != children.end();
	 ++it) {
      (*it)->get_leaves();
    }
  }
  
  static void getAllStats(struct scanStats &stats) {
    stats.input_size = input_size;
#if PARANOIA > 0
    stats.dbg_real_total_matches = dbg_real_total_matches;
#endif
    stats.roots = roots.size();
    
    for (vector<dfaTree*>::iterator it = roots.begin();
	 it != roots.end(); ++it) {
      dfaTree *t = *it;
      t->get_stats(stats);
    }
  }

  static void showAllStats() {
    struct scanStats stats;
    getAllStats(stats);
    stats.dumpStats();
  }
  
  
  static void dbg_printAll() {
    MyTime tm("dbg_printAll");
    for (vector<dfaTree*>::iterator it = roots.begin();
	 it != roots.end(); ++it) {
      dfaTree *t = *it;
      t->dbg_print("###");
    }
  }

  static void getLeaves() {
    if (!leaves.empty())
      return;
    for (vector<dfaTree*>::iterator it = roots.begin();
	 it != roots.end(); ++it) {
      dfaTree *t = *it;
      t->get_leaves();
    }
  }

  static void loadAll() {
    MyTime tm("loadAll");
    for (vector<dfaTree*>::iterator it = roots.begin();
	 it != roots.end(); ++it) {
      dfaTree *t = *it;
      t->load();
    }
  }
  
  static void resetAll() {
    dbg_real_total_matches = 0;

    MyTime tm("resetAll");
    for (vector<dfaTree*>::iterator it = roots.begin();
	 it != roots.end(); ++it) {
      dfaTree *t = *it;
      t->reset_stats();
    }
  }



  static unsigned long long dbg_real_total_matches; //cannot obtain without performance loss
  static unsigned long long input_size; //cannot obtain without performance loss
  unsigned long long total_apparent_matches;
  unsigned long long total_match_calls;
  
  bool match(const unsigned char *buf, unsigned int len) {
    ++total_match_calls;
  
    bool is_match  = mm->match(buf, len);
    if (is_match) {
      ++total_apparent_matches;
      
#if 1
      if (!children.empty()) {//TBD: speed: save children.empty()
	int rec_num_matches = 0;
	for (vector<dfaTree*>::iterator it = children.begin();
	     it != children.end();
	     ++it) {
	  if((*it)->match(buf, len))
	    ++rec_num_matches;
	}
	is_match =  rec_num_matches != 0;
      }
#endif
    }
    return is_match;
  }

#if 1
  TimerType static scanTree(int fd) {
    MyTime tm("scanTree");
    unsigned char buf[MAX_PACKET_SIZE];
    unsigned long packets_extracted = 0;
    int size = 0;
    unsigned long long matches = 0;
    dbg_real_total_matches = 0;
    
    while(read_packet(fd, buf, size)) {
      packets_extracted ++;
#if PARANOIA > 0
      unsigned long long old_matches = matches;
#endif    

      for (vector<dfaTree*>::iterator it = roots.begin();
	   it != roots.end(); ++it) {
	dfaTree *t = *it;
	if (t->match(buf, size))
	  matches++;
      }
#if PARANOIA > 0
      if (matches != old_matches)
	++dbg_real_total_matches;
#endif
    }
    tm.stop();
    DMP(matches);
#if PARANOIA > 0
    DMP(dbg_real_total_matches);
#endif    
    return tm.get_total();
  }
  TimerType static scanLeaves(int fd) {
    MyTime tm("scanLeaves");
    getLeaves();
    unsigned char buf[MAX_PACKET_SIZE];
    unsigned long packets_extracted = 0;
    int size = 0;
    unsigned long long matches = 0;
    dbg_real_total_matches = 0;
    
    while(read_packet(fd, buf, size)) {
#if PARANOIA > 0
      unsigned long long old_matches = matches;
#endif    
      packets_extracted ++;
      for (vector<dfaTree*>::iterator it = leaves.begin();
	   it != leaves.end(); ++it) {
	dfaTree *t = *it;
	if (t->match(buf, size))
	  matches++;
      }
#if PARANOIA > 0
      if (matches != old_matches)
	++dbg_real_total_matches;
#endif
    }
    tm.stop();
    DMP(matches);
#if PARANOIA > 0
    DMP(dbg_real_total_matches);
#endif    
    return tm.get_total();
  }
#endif
  ////////////////////////////////////////////////
  bool dbgCheckTreeNode(const unsigned char *buf, unsigned int len) {
    bool is_match  = mm->match(buf, len);
    if (!children.empty()) {//TBD: speed: save children.empty()
      for (vector<dfaTree*>::iterator it = children.begin();
	   it != children.end();
	   ++it) {
	bool child_is_match = (*it)->dbgCheckTreeNode(buf, len);
	assert(!child_is_match || is_match);
      }
    }
    return is_match;
  }
public:
  void static dbgCheckTree(int fd) {
    MyTime tm("dbg_check_overapprox");
    unsigned char buf[MAX_PACKET_SIZE];
    int size = 0;
    
    while(read_packet(fd, buf, size)) {
      bool some_apparent_match = false;
      for (vector<dfaTree*>::iterator it = roots.begin();
	   it != roots.end(); ++it) {
	dfaTree *t = *it;
	if (t->dbgCheckTreeNode(buf, size))
	  some_apparent_match = true;
      }
      bool some_real_match = false;
      for (vector<dfaTree*>::iterator it = leaves.begin();
	   it != leaves.end(); ++it) {
	dfaTree *t = *it;
	if (t->match(buf, size)) {
	  some_real_match = true;
	  break;
	}
      }
      assert(!some_real_match || some_apparent_match);
    }
  }
};
unsigned long long dfaTree::dbg_real_total_matches; //cannot obtain without performance loss
unsigned long long dfaTree::input_size; //cannot obtain without performance loss

vector<dfaTree*> dfaTree::roots;
vector<dfaTree*> dfaTree::leaves;

static bool readDfaTree(FILE *logf) {
  MyTime("readDfaTree");
  cout << "readDfaTree ..." << endl;
  char * line = NULL;
  size_t len = 0;
  ssize_t read;
  map<string, dfaTree*> name2tree;
  while ((read = getline(&line, &len, logf)) != -1) {
    chomp(line);
    std::stringstream ss;
    ss << line;
    string first;
    ss >> first;
    //cout << "first=" << first << endl;
    if (first == "LOG@SKIPBOTCH:") {
      cout << "GOT:" << line << endl;
      string fileName; ss >> fileName;
      string tmp;    ss >> tmp;
      assert(   tmp == "contains:"
	     || tmp == "contains-no-children");
      
      cout << "INSIDE:" << fileName << endl;
      dfaTree *t;
      if (CONTAINS(name2tree, fileName)) {
	t = name2tree[fileName];
	if (t->parent == NULL) {
	  cout << " ERROR parent == NULL: fileName: "
	       << fileName << " " << t->fileName << endl;
	}
	assert(t->parent != NULL); //for now ...
      } else {
	t = new dfaTree(fileName, NULL);
	name2tree[fileName] = t;
      }
      while(!ss.eof()) {
	if ((ss >> tmp).fail())
	  break;
	cout << "[file]" << tmp << ":";
	assert(!CONTAINS(name2tree, tmp));
	dfaTree *child = new dfaTree(tmp, t);
	name2tree[tmp] = child;
	t->children.push_back(child);
      }
      cout << endl;
    }
  }
  if (line)
    free(line);
  cout << ".... readDfaTree" << endl;
  
  //dfaTree::dbg_printAll();
  return true;
}

void get_tree_stats() { // dump tree statistics
  dfaTree::getTreeStatsAll(true);
}

void scan_bdfa_tree() {// TBD: maybe pin_processors() ...
  MyTime("scan_bdfa_tree");
  readDfaTree(zop.optFILE("logf"));
  
  dfaTree::loadAll();
  dfaTree::dbg_printAll();
  
  pin_processors();
  cout << ".........................................\n";
  TimerType tt1 = dfaTree::scanTree(zop.optFileDescriptor("PAYLOAD"));
  struct dfaTree::scanStats statsTree1; dfaTree::getAllStats(statsTree1);  statsTree1.dumpStats(); dfaTree::resetAll();
  //dfaTree::showAllStats(); 
  cout << ".........................................\n";
  TimerType tl = dfaTree::scanLeaves(zop.optFileDescriptor("PAYLOAD"));
  struct dfaTree::scanStats statsLeaves; dfaTree::getAllStats(statsLeaves);  statsLeaves.dumpStats(); dfaTree::resetAll();
  //dfaTree::showAllStats();
  cout << ".........................................\n";
  TimerType tt2 = dfaTree::scanTree(zop.optFileDescriptor("PAYLOAD"));
  struct dfaTree::scanStats statsTree2; dfaTree::getAllStats(statsTree2);  statsTree2.dumpStats(); dfaTree::resetAll();
  //dfaTree::showAllStats();
  float speedup1 = tl/(float)tt1;
  float speedup2 = tl/(float)tt2;
  assert(statsTree1.total_memory_steps == statsTree2.total_memory_steps);
  float thSpeedup = statsLeaves.total_memory_steps/(float)statsTree1.total_memory_steps;
  float efficiency1 = speedup1/statsTree1.leaves;
  float efficiency2 = speedup2/statsTree1.leaves;
  float thEfficiency = thSpeedup/statsTree1.leaves;
  DMP(speedup1); DMP(speedup2); DMP(thSpeedup);
  DMP(efficiency1); DMP(efficiency2); DMP(thEfficiency) << endl ;
}

///////////////////////////////////////////////////////////////////////////////
int count_sigs_in_file(FILE *file) {
  char * line = NULL;
  size_t len = 0;
  unsigned int lines_processed = 0;
  while (getline(&line, &len, file) != -1) {
    lines_processed++;
  }
  if (line)
    free(line);
  return lines_processed;
}

struct threadData {
  vector<vector<int> > *conflicts;
  vector<dfa_tab_t*> *dfas;
  vector<vector<const char*> > *msgs;
  int level;
  int startIndexThread;
  int strideThread;//numThreads
  threadData(vector<vector<int> > *conflicts0,
	     vector<dfa_tab_t*> *dfas0,
	     vector<vector<const char*> > *msgs0,
	     int level0,
	     int startIndexThread0,
	     int strideThread0):
    conflicts(conflicts0),
    dfas(dfas0),
    msgs(msgs0),
    level(level0),
    startIndexThread(startIndexThread0),
    strideThread(strideThread0)
    {}
};

void* build_CG_thread(void *ptr) {
  threadData * td = (threadData*) ptr;
  vector<vector<int> > &conflicts = *(td->conflicts);
  vector<dfa_tab_t*> &dfas = *(td->dfas);
  vector<vector<const char*> > &msgs = *(td->msgs);
  const int level = td->level;
  const int startIndexThread = td->startIndexThread;
  const int strideThread = td->strideThread;
  std::stringstream out;
  out << "build_CG_thread: " << startIndexThread << "/" << numThreads << "/" << strideThread;
  MyTime tm(out.str());
  
  assert (!dfas.empty());
  size_t size = dfas.size();

  for (unsigned int s1 = startIndexThread; s1 < size; s1+=strideThread) {
    dfa_tab_t *pdt1 = dfas[s1];
    //assert(pdt1); This fails if we skip some signatures as 951,1192 in http-2612
    if (pdt1 == NULL)
      continue;
    //(cout << "in build_CG_thread ...").flush(); int i; cin >> i;
    state_id_t num1 = num_states(pdt1);
    for (unsigned int s2 = s1+1; s2 < size; ++s2) {
      const char * msg="";
      dfa_tab_t *pdt2 = dfas[s2];
      
      //assert(pdt2); This fails if we skip some signatures as 951,1192 in http-2612
      if (pdt2 == NULL)
	continue;
      //cout << " TRY: " << s1 << " with " << s2 << endl;
      
      state_id_t num2 = num_states(pdt2);
      state_id_t tmp_states = 4*(num1+num2)*level;
      dfa_tab_t *tmp = NULL;
      if (level >= 2) {
	tmp_states = zop.dbgNumOption("MDFASTATES", MDFASTATES);
      }
      try {
	tmp = dt_union(pdt1, pdt2, tmp_states, false);//should be 'true'...
      } catch (bad_alloc) {
	tmp = NULL;
	cout << "BAD-ALLOC-";
      }
      bool compose = false;
      if (tmp) {
	tmp->minimize();
      	if (num_states(tmp) > num_states(pdt1) + num_states(pdt2))
	  msg = "CONFLICTS ";
	else {
	  msg = "COMPOSE ";
	  compose = true;
	}
	tmp_states = num_states(tmp);
      } else
	msg = "CONFLICT4TRUNC ";


#if 0
      if (strideThread == 1)
	cout << "dbg:" << msg << s1 << " with " << s2 << " : "
	     << tmp_states << " " << num1 << " " << num2
	     << endl;
#endif
      
      msgs[s1][s2] = msg;
      
      if (!compose)
	conflicts[s1][s2] = tmp_states;
      else
	conflicts[s1][s2] = -tmp_states;
      
      delete tmp;
    }
  }
  delete td;
  pthread_exit(NULL);
  return NULL;
}

int numCPUs(void) {
  int numCPU = sysconf( _SC_NPROCESSORS_ONLN );
  //printf("numCPU=%d\n", numCPU);
  return numCPU;
}

void buildConflictGraph(vector<vector<int> > &conflicts,
			vector<dfa_tab_t*> &dfas,
			int level) {
  MyTime tm("buildConflictGraph");
  cout << "buildConflictGraph\n";
  
  assert (!dfas.empty());
  assert(conflicts.empty());
  size_t size = dfas.size();
  //cout << "DBG: dfas.size= " << size << endl;
  
  //assert(0&&"simplify debugging of the mem bug!");
  vector<vector<const char*> > msgs;
  conflicts.clear();
  conflicts.resize(size+1); msgs.resize(size+1);
  assert(dfas[0]==NULL);
  for (unsigned int sig1 = 0+1; sig1 < 1 + size; ++sig1) {
    conflicts[sig1].resize(size+1, 0);
    msgs[sig1].resize(size+1,0);
  }
  
  assert(dfas[0] == NULL);
  
  vector<pthread_t> threads;
  
  int startIndexThread = condorProcess;
  int strideThread = numCondorProcesses*numThreads;
  
  for (unsigned int t = 1; t <= numThreads; ++t) {
    pthread_t thread;
    threadData *td = new threadData(&conflicts, &dfas, &msgs, level,
				    startIndexThread, strideThread);
    startIndexThread += numCondorProcesses;
    if (pthread_create( &thread, NULL, build_CG_thread, (void*) td)) {
      perror("Thread creation failed!\n");
      exit(1);
    }
    threads.push_back(thread);
  }
  assert (threads.size() == numThreads);
  for (unsigned int t = 0; t < numThreads; ++t) {
    if (pthread_join(threads[t], NULL)) {
      perror("Thread join failed!\n");
    }
  }

  //////////////////
  //cout << "DBG: Start to look at " << size << " elements " << endl;
  for (unsigned int s1 = 1; s1 < size; s1+=1) {
    //cout << "DBG: Look at " << s1 << endl;
    dfa_tab_t *pdt1 = dfas[s1];
    //assert(pdt1);
    if (pdt1 == NULL)
      continue;
    state_id_t num1 = num_states(pdt1);
    for (unsigned int s2 = s1+1; s2 < size; ++s2) {
      //cout << "DBG: And also Look at " << s2 << endl;
      //cout << "DBG: msgs[s1].size()== " << msgs[s1].size() << endl;
      const char * msg = msgs[s1][s2];
      //cout << "DBG: msgs[s1][s2] == NULL? " << (msg==NULL) << endl;
      if (msg == NULL) continue;//for condor processes
      dfa_tab_t *pdt2 = dfas[s2];
      //assert(pdt2);
      if (pdt2 == NULL)
	continue;
      state_id_t num2 = num_states(pdt2);
      
      int tmp_states = conflicts[s1][s2];
      if (tmp_states < 0)
	tmp_states = -tmp_states;

      dout << msg << s1 << " with " << s2 << " : "
	   << tmp_states << " " << num1 << " " << num2
	   << ENDL;
    }
  }
  cout << "DONE";
}


void delete_dfas(vector<dfa_tab_t*> &dfas) {
  unsigned int size = dfas.size();
  for (unsigned int d = 0; d < size; ++d) {
    if (dfas[d])
      delete dfas[d];
    dfas[d]=NULL;
  }
}

/* The file containing the regular expression/signatures is specified with
   the required option -zsigFile:FILE
   The regular expressions were pre-processed and saved in DFA files named
   XXX1,XXX2,XXX3,...,XXXk(has the k-th signature in FILE), ...
   where XXX is specified with the required option -zreadSigs:XXX  (TBD: use dumpSigMdfa!)
 */
void read_saved_signature_dfas(vector<dfa_tab_t*> &dfas) {
  MyTime tm("read_saved_signature_dfas");
  assert(dfas.empty());
  if (zop.dbgStringOption("readSigs").empty()) {
    //sigs_from_file(zop.optFILE("sigFile"), dfas);
    zop.usage_and_die("Option -z readSigs:??? missing, but required for building the conflict graph");
  }
    
  string readSigs = zop.dbgStringOption("readSigs");
  dfas.clear();
  dfas.push_back(NULL);
  unsigned int numSigs = count_sigs_in_file(zop.optFILE("sigFile"));
  cout << " LOADING signatures from "
       << zop.dbgStringOption("sigFile")
       << " saved as " << numSigs << " '" << readSigs << "'* files! \n"; cout.flush();
  for (unsigned int i=1; i<= numSigs; ++i) {
    std::stringstream ss;
    ss << readSigs << i;
    dfa_tab_t *pdt = new dfa_tab_t();
    assert(pdt);
    if (!pdt->txt_dfa_load(ss.str().c_str())) {
      delete pdt;
      pdt = NULL;
    }
    dfas.push_back(pdt);
  }
}

bool addSig2Mdfa(dfa_tab_t *&mdfa,
		 unsigned int j,// the signature to add
		 vector<dfa_tab_t*> &dfas,
		 set<int> &sigsInMdfa,
		 state_id_t &lastMinim,
		 int max_mdfa_states) {
  if (dfas[j] == NULL)
    return false;
  dfa_tab_t *tmp = dt_union(mdfa, dfas[j], max_mdfa_states, false);
  if (tmp) {
    delete mdfa;
    delete dfas[j];
    dfas[j] = NULL;
    mdfa = tmp;
    if (mdfa->num_states > lastMinim + 8000)
      guarded_minimize(mdfa, lastMinim);
    
    sigsInMdfa.insert(j);
    cout << " -IN" << endl;
    return true;
  }
  else cout << " -OUT" << endl;
  return false;
}

static int conflictWeight = -1; //default
int conflict_weight(int confl_ij) {
  if (confl_ij <= 0)
    return 0;
  if (conflictWeight == -1) { // unspecified
    return confl_ij; // default
  } else if (conflictWeight == 1) {
    return 1; // as in Yu's paper
  }
  assert(0 && "bad conflictWeight value");
  return 0;
  // May want to also try confl_ij/(states_in_i+states_in_j)
}
void augmentMdfa(dfa_tab_t *mdfa,
		 vector<dfa_tab_t*> &dfas,
		 const vector<vector<int> > &conflicts,
		 set<int> &sigsInMdfa,
		 const set<int> * pignoreSet,
		 int max_mdfa_states,
		 const string& mdfaName) {
  assert(mdfa);
  MyTime tm("completeMdfa");
  set<pair<int, int> > reserves;
  state_id_t lastMinim = mdfa->num_states;

  int num_added = 0;
  
  for (unsigned int j = 0; j < dfas.size(); ++j) {
    //assert(dfas[j] == NULL || j>=i);
    if (dfas[j] == NULL)
      continue;
    if (pignoreSet && CONTAINS(*pignoreSet, j)) {
      cout << mdfaName << " #sigs= " << sigsInMdfa.size() << " #states= " << mdfa->num_states << " j= " << j << " -ignored" << endl;
      continue;
    }
    int numConflicts = 0;
    cout << mdfaName << " #sigs= " << sigsInMdfa.size() << " #states= " << mdfa->num_states << " j= " << j;
    cout.flush();
    for (set<int>::iterator it = sigsInMdfa.begin();
	 it != sigsInMdfa.end(); ++it) {
      int i = *it;
      int confl_ij = conflicts[i][j];
      
      numConflicts += conflict_weight(confl_ij);
    }
    if (numConflicts == 0) {
      bool added = addSig2Mdfa(mdfa, j, dfas, sigsInMdfa, lastMinim, max_mdfa_states);
      if (added)
	++num_added;
    }
    else {
      cout << " j-reserve" << endl;
      reserves.insert(make_pair(numConflicts, j));
    }
  }
  guarded_minimize(mdfa, lastMinim);
  DMP(num_added);
  //mdfa->txt_dfa_dump((mdfaName+"_tmp").c_str());
  //TBD:need this? tm.stop(" before reserves "); tm.resume();
  
  for(set<pair<int, int> >::iterator it = reserves.begin();
      it != reserves.end(); ++it) {
    //int numConflicts = it->first;
    int j = it->second;
    cout << " reserves " << mdfaName << " #sigs= " << sigsInMdfa.size() << " #states= " << mdfa->num_states << " j= " << j;
    cout.flush();
    if (dfas[j] == NULL) {
      cout << " j=" << j << endl;
    }
    assert(dfas[j]);
    bool added = addSig2Mdfa(mdfa, j, dfas, sigsInMdfa, lastMinim, max_mdfa_states);
    if (added)
      ++num_added;
  }
  guarded_minimize(mdfa, lastMinim);
  DMP(0+num_added);
  mdfa->txt_dfa_dump(mdfaName.c_str());
  delete mdfa; mdfa = NULL;
}

string makeNdfaName(unsigned int num_mdfa,
		    const string &from_what,
		    unsigned int i) {
  std::stringstream mdfaName;
  mdfaName << "mdfa_" << num_mdfa << from_what << i;
  return mdfaName.str();
}

void showBotchInfo(string &fileName, int level) {
  dout.append("DBG-showBotchInfo."+get_name_of_bdfa_tree_file());
  assert(!CONTAINS(botchInfo.botches, fileName) || !CONTAINS(botchInfo.groups, fileName));
  for (int i =0; i < level; ++i ) dout << " ";
  if (CONTAINS(botchInfo.botches, fileName)) {
    dout << fileName << " botches: " << botchInfo.botches[fileName] << ENDL;
    showBotchInfo(botchInfo.botches[fileName], level+4);
  } else if (CONTAINS(botchInfo.groups, fileName)) {
    dout << fileName << " contains: " << ENDL;
    for (vector<string>::iterator fin = botchInfo.groups[fileName].begin();
	 fin != botchInfo.groups[fileName].end();
	 ++fin)
      showBotchInfo(*fin, level+4);
  } else
    dout << fileName << ENDL;
  dout.close();
}

// Dump the complete BotchInfo-tree, including (botches ->) and (contains ->) edges
void logBotchInfo(string &fileName) {
  dout.append("DBG-logBotchInfo."+get_name_of_bdfa_tree_file());
  assert(!CONTAINS(botchInfo.botches, fileName) || !CONTAINS(botchInfo.groups, fileName));
  if (CONTAINS(botchInfo.botches, fileName)) {
    dout << "LOG@BOTCH: " << fileName << " botches: " << botchInfo.botches[fileName] << ENDL;
    logBotchInfo(botchInfo.botches[fileName]);
  } else if (CONTAINS(botchInfo.groups, fileName)) {
    dout << "LOG@BOTCH: " << fileName << " contains: ";
    for (vector<string>::iterator fin = botchInfo.groups[fileName].begin();
	 fin != botchInfo.groups[fileName].end();
	 ++fin)
      dout << *fin << " ";
    dout << ENDL;
    
    for (vector<string>::iterator fin = botchInfo.groups[fileName].begin();
	 fin != botchInfo.groups[fileName].end();
	 ++fin)
      logBotchInfo(*fin);
  }
  dout.close();
}

/* Dump just the bdfa-tree, with (contains ->) edges, but skipping over
   (botches ->) edges.
   That is, if the complete BotchInfo tree is
   R -contains-> (B1,B2)
   B1-botches-> D1
   B2-botches-> D2
   Then here we dump the tree
   R -children-> (D1,D2)
   
   This is the format consumed by scan_bdfa_tree...,
   and it has lines that start with LOG@SKIPBOTCH
 */
void logSkippedBotchInfo(string &fileName, bool is_root = false) {
  string bdfa_tree_file_name = get_name_of_bdfa_tree_file();
  dout.append(bdfa_tree_file_name);
  
  assert(!CONTAINS(botchInfo.botches, fileName));
  if (CONTAINS(botchInfo.groups, fileName)) {
    vector<string> indirectContent;
    dout << "LOG@SKIPBOTCH: " << fileName << " contains: ";
    for (vector<string>::iterator fin = botchInfo.groups[fileName].begin();
	 fin != botchInfo.groups[fileName].end();
	 ++fin) {
      string indirectFile = *fin;
      if (CONTAINS(botchInfo.botches, indirectFile))
	indirectFile = botchInfo.botches[indirectFile];
      assert(!(CONTAINS(botchInfo.botches, indirectFile)));//DL:Oct 25: I think this should hold ....
      dout << indirectFile << " ";
      indirectContent.push_back(indirectFile);
    }
    dout << ENDL;
    
    for (vector<string>::iterator fin = indirectContent.begin();
	 fin != indirectContent.end();
	 ++fin)
      logSkippedBotchInfo(*fin);
  } else {
    if (is_root)
      dout << "LOG@SKIPBOTCH: " << fileName << " contains-no-children " << ENDL;
  }
  dout.close();
}

dfa_tab_t* do_botch(string & fileName,
		    double falsePositives) {
  MyTime tm("MMBotcher for "+fileName+" with "+toString(falsePositives));
  MMBotcher mm(fileName);
  dfa_tab_t *pbdt = mm.botch(falsePositives);
  return pbdt;
}

dfa_tab_t* do_size_botch(string & fileName,
			 state_id_t max_mdfa_states,
			 double &falsePositives) {
  MyTime tm("MMBotcher for "+fileName+" with size="+toString(max_mdfa_states));
  MMBotcher mm(fileName);
  dfa_tab_t *pbdt = mm.botch(max_mdfa_states, falsePositives);
  return pbdt;
}

void dbg_botch() {
  dfa_tab_t *pbdt = NULL;
  string mdfa2botch = zop.dbgStringOption("MDFA2BOTCH");
  if (zop.dbgDefinedOption("MDFASTATES")) {
    double falsePositives=0;
    state_id_t max_mdfa_states = zop.dbgNumOption("MDFASTATES", MDFASTATES);
    cout << " dbg_botch: max_mdfa_states= " << max_mdfa_states << endl;
    pbdt = do_size_botch(mdfa2botch, max_mdfa_states, falsePositives);
    DMP(falsePositives);
  } else {
    double falsePositives = zop.dbgDoubleOption("falsePositives", DEFAULT_FALSE_POSITIVES);
    cout << " dbg_botch: falsePositives= " << falsePositives << endl;
    pbdt = do_botch(mdfa2botch, falsePositives);
  }
  delete pbdt;
}

dfa_tab_t* cache_botch(string & fileName,
		       double falsePositives) {
  dfa_tab_t *pbdt_res = NULL;
  MyTime tm(("botching-"+fileName).c_str());
  cout << "botch(" << fileName << ", " << falsePositives << ")" << endl;
  
  string BDFA_cached_name = MMBotcher::BDFA_name(fileName, falsePositives);//expected Botch file name
  
  // Check "cache" if this was botched before.
  if (CONTAINS(botchInfo.botches, BDFA_cached_name)) {//Cache hit. Load saved file
    cout << BDFA_cached_name << " cache hit " << endl;
    pbdt_res = new dfa_tab_t();
    pbdt_res->txt_dfa_load(BDFA_cached_name.c_str());
  } else {
    pbdt_res = do_botch(fileName, falsePositives); 
    botchInfo.botches[BDFA_cached_name] = fileName;
  }
  assert(pbdt_res);
  return pbdt_res;
}


void SHRINK(vector<string>& inFileNames,
	    vector<dfa_tab_t*>& bdfas,
	    vector<string> &botchedInFileNames,
	    double falsePositives) {
  MyTime tm("SHRINK");
  bdfas.clear();
  botchedInFileNames.clear();
  bdfas.push_back(NULL);//start from 0
  for (vector<string>::iterator fin = inFileNames.begin();
       fin != inFileNames.end();
       ++fin) {
    string fileName = *fin;
    dfa_tab_t *pdt = cache_botch(fileName, falsePositives);// TBD: store BDFA's name in pdt
    assert(pdt);
    bdfas.push_back(pdt);
    string dbg_tmp = MMBotcher::BDFA_name(fileName, falsePositives);
    //DMP(dbg_tmp); DMP(pdt->filename) << ENDL;
    assert(pdt->filename == dbg_tmp);
    botchedInFileNames.push_back(pdt->filename);
  }
}

void GROUP(vector<string>& inFileNames,
	   vector<string>& outFileNames,
	   vector<dfa_tab_t*> bdfas,
	   int level,
	   vector<string> botchedInFileNames) {
  MyTime tm("GROUP");
  int max_mdfa_states = zop.dbgNumOption("MDFASTATES", MDFASTATES);
  vector<vector<int> > conflicts;
  buildConflictGraph(conflicts, bdfas, level);
  unsigned int num_mdfa = 0;
  
  for (unsigned int i = 0; i < bdfas.size(); ++i) {
    if (bdfas[i]) {
      cout.flush();
      MyTime tm("mdfa-i");
      string mdfaName = makeNdfaName(num_mdfa, "_lvl_", level);
      
      ++num_mdfa;
      
      cout << " num_mdfa: " << num_mdfa << " i= " << i << endl;
      dfa_tab_t *mdfa = bdfas[i];
      bdfas[i] = NULL;
      set<int> sigsInMdfa;
      sigsInMdfa.insert(i);
      augmentMdfa(mdfa, bdfas, conflicts, sigsInMdfa, NULL,
		  max_mdfa_states, mdfaName);
      if (sigsInMdfa.size() == 1) {
	mdfaName = inFileNames[i-1];// not the same as mdfa->filename (mdfa_2_lvl_1 vs. BDFA.from.mdfa_2_lvl_1.fp.0.008)
	DMP(mdfaName);DMP(mdfa->filename);
	outFileNames.push_back(mdfaName); // Inefficient. Will botch again ...
	cout << "CONTENT of " << mdfaName << " is unchanged..." << endl;
      } else {
	outFileNames.push_back(mdfaName);
	assert(!CONTAINS(botchInfo.botches, mdfaName));
	cout << "CONTENT of " << mdfaName << " = ";
	for (set<int>::iterator it = sigsInMdfa.begin();
	     it != sigsInMdfa.end();
	     ++it) {
	  if (!(1 <= *it && *it <= (int)botchedInFileNames.size())) {
	    DMP(i);DMP(*it) << endl;
	    cout.flush();
	    assert(1 <= *it && *it <= (int)botchedInFileNames.size());
	  }
	  cout << botchedInFileNames[(*it) -1] << " ";
	  botchInfo.groups[mdfaName].push_back(botchedInFileNames[(*it) -1]);
	}
	cout << endl;
	cout.flush();
      }
    }
  }
  delete_dfas(bdfas);//keep valgrind happy
}

void build_bdfa_tree(vector<string>& inFileNames) {
  int level = 0;
  int depth = 0;
  double falsePositives = zop.dbgDoubleOption("falsePositives", DEFAULT_FALSE_POSITIVES);
  int tweaks = 0;
  while (inFileNames.size() > 1) {
    
    //reverse(inFileNames.begin(), inFileNames.end());
    
    ++level; ++ depth;
    MyTime tm("botchMdfas-level-"+toString(level));
    
    cout << "build_bdfa_tree level= " << level << " depth=" << depth << " #files: " << inFileNames.size() << endl;
    vector<string> outFileNames;
    //botchMdfas(inFileNames, outFileNames, falsePositives, level);
    {
      vector<dfa_tab_t*> bdfas;
      vector<string> botchedInFileNames;//TBD: get rid of this
      SHRINK(inFileNames, bdfas, botchedInFileNames, falsePositives);
      GROUP(inFileNames, outFileNames, bdfas, level, botchedInFileNames);
    }
    
    if (inFileNames.size() == outFileNames.size()) {
      cout << "build_bdfa_tree CANNOT reduce file group of file size= "
	   << outFileNames.size()
	   << " fp= " << falsePositives
	   << " level=" << level
	   << " depth=" << depth;
      
      double new_falsePositives = falsePositives * 2;
      if (new_falsePositives * inFileNames.size() > 0.33) { // 0.33 Tweak this...

	new_falsePositives = 0.33/inFileNames.size();
	if (tweaks > 0 || new_falsePositives <= falsePositives) {
	  cout << " FAILED: Give up " << " new.fp= " << new_falsePositives << " too large. tweaks= " << tweaks << endl;
	  std::stringstream lastin;
	  std::stringstream lastout;
	  for (size_t i = 0; i < inFileNames.size(); ++i) {
	    lastin << inFileNames[i] << " ";
	    lastout << outFileNames[i] << " ";
	  }
	  cout << "LAST-GOOD-SET: " << lastin.str() << endl;
	  cout << "LAST-ATTEMPT-OUT: " << lastout.str() << endl;
	  break;
	}
	cout << " TWEAKED: new.fp= " << new_falsePositives << endl;
	tweaks++;
      } else {
	cout << " CONTINUE: Doubled fp new.fp= " << new_falsePositives << endl;
      }
      falsePositives = new_falsePositives;
      cout << " IGNORE-PRECEDING-FILES: " ;
      for (vector<string>::iterator it = outFileNames.begin(); it != outFileNames.end(); ++it) {
	string fname = *it;
	cout << fname << " ";
      }
      cout << endl;
      depth --;
      continue;
    }
    
    inFileNames = outFileNames;
    {
      Dout level_dout((zop.dbgStringOption("BOTCH") + ".level-"+toString(level)).c_str());
      for (vector<string>::iterator it = inFileNames.begin(); it != inFileNames.end(); ++it) {
        string fname = *it;
	level_dout << fname << ENDL;
      }
    }
  }
  if (inFileNames.size() == 1)
    cout << "build_bdfa_tree SUCCEEDED to reduce file group to a single file."
	 << "Done at level: " << level << " depth=" << depth << endl;
  for (vector<string>::iterator it = inFileNames.begin(); it != inFileNames.end(); ++it) {
    logBotchInfo(*it);
  }
  cout << endl;
  for (vector<string>::iterator it = inFileNames.begin(); it != inFileNames.end(); ++it) {
    logSkippedBotchInfo(*it, true);
  }
  cout << endl;
  for (vector<string>::iterator it = inFileNames.begin(); it != inFileNames.end(); ++it) {
    showBotchInfo(*it, 0);
  }
  cout << endl;
}

void read_file_names(vector<string> & fileNames, const string &containerName) {
  // TBD: use a TREE.files2botch file instead of parsing some log.botch
  MyTime tm("read_file_names:"+containerName);
  FILE *cf = fopen(containerName.c_str(), "r");
  if (cf == NULL)
    perror(("Cannot open/read file " + containerName).c_str());
  char * line = NULL;
  size_t len = 0;
  ssize_t read;
  while ((read = getline(&line, &len, cf)) != -1) {
    chomp(line);
    fileNames.push_back(string(line));
  }
  if (line)
    free(line);
  
  if (1) {//dbg
    cout << "#read= " << fileNames.size() << " file names read from "
	 << containerName << " : ";
    for (vector<string>::iterator it = fileNames.begin();
	 it != fileNames.end(); ++it) {
      cout << * it <<" , ";
    }
    (cout << endl).flush();
  }
  fclose(cf);
  return;
}

/* Build a dfa-tree with leaves the DFAs specified by the required argument
   -zBOTCH:<files2botch>
   For statistics, scan the payload specified by the required argument
   -zPAYLOAD:<payload-file>
   OUTPUT: there is no actual tree build, just the DFAs corresponding to the
   nodes (and the intermediate BDFAs). There is a tree structure that relates
   these DFAs, and that is dumped both at the standard output (part of the
   debugging messages), as well as in a file named 'bdfa-tree.<files2botch>'
*/
void build_bdfa_tree() {
  // TBD: use a TREE.files2botch file instead of parsing some log.botch
  vector<string> fileNames;
  string files2botch = zop.dbgStringOption("BOTCH");
  MyTime tm("Botching-files-in:"+files2botch);
  bool old_minimMDFA = disable_Moore_minimization_unless_option_overwritten();
  read_file_names(fileNames, files2botch);
  
  build_bdfa_tree(fileNames);
  minimMDFA = old_minimMDFA;
}

void check_predefined_groups(vector<dfa_tab_t*> &dfas,
			     const vector<vector<int> > &conflicts,
			     set<int> &skip,
			     int max_mdfa_states,
			     unsigned int& num_mdfa) {
  MyTime tm("check_predefined_groups");
  vector<set<int> > groups;
  
  if (maybeReadGroups(groups)) {
    /* TBD: fix this. The problem is that for multiple groups,
       members from later groups get inserted in the mdfa's for earlier groups
    */
    cout << "#groups= " << groups.size() << endl;
    for (unsigned int g = 0;  g < groups.size(); ++g) {
      MyTime tm("mdfa-grp");
      cout << "group " << g <<" : ";
      const set<int> & grp = groups[g];
      
      string mdfaName = makeNdfaName(num_mdfa, "_from_group_", g);
      
      num_mdfa++;
      
      dfa_tab_t *mdfa = NULL;
      set<int> sigsInMdfa;
      
      DMP(dfas.size()) << endl;
      
      state_id_t lastMinim = 0;
      for (set<int>:: const_iterator it = grp.begin(); it != grp.end(); ++it) {
	int i = *it;
	if (dfas[i] == NULL) {
	  cout << "WARNING: group member " << *it << " has NULL mdfa!" << endl;
	  continue;
	}
	if (mdfa == NULL) {
	  mdfa = dfas[i]; dfas[i]=NULL;
	  lastMinim = mdfa->num_states;
	  cout << " init-group " << g << " " << mdfaName << " #sigs= " << sigsInMdfa.size() << " #states= " << mdfa->num_states << " j= " << *it << " -IN" << endl;
	  sigsInMdfa.insert(i);
	  continue;
	}
	cout << " init-group " << g << " in " << mdfaName << " #sigs= " << sigsInMdfa.size() << " #states= " << mdfa->num_states << " j= " << *it;
	if (!addSig2Mdfa(mdfa, *it, dfas, sigsInMdfa, lastMinim, max_mdfa_states)) {
	  cout << "WARNING: group member " << *it << " did not fit in!" << endl;
	}
      }
      //TBD:need this? tm.stop(" before augmenting");tm.resume();
      mdfa->txt_dfa_dump((mdfaName + "_group-only_tmp").c_str());
      set<int> ignoreSet = skip;
      cout << "ignore set for group g= " << g << " : ";
      for (unsigned int ig = 0;  ig < groups.size(); ++ig) {
	if (ig == g)
	  continue;
	const set<int> & grp = groups[ig];
	for (set<int>:: const_iterator it = grp.begin(); it != grp.end(); ++it) {
	  cout << *it << " ";
	  ignoreSet.insert(*it);
	}
      }
      cout << endl;
      
      if (mdfa) {
	augmentMdfa(mdfa, dfas, conflicts, sigsInMdfa, &ignoreSet,
		    max_mdfa_states, mdfaName);
      }
      cout << endl;
    }
  }
  
  assert(dfas[0]==0);//count from 1
  for (unsigned int i = 1; i < dfas.size(); ++i) {
    if (dfas[i] && !CONTAINS(skip, i)) {
      cout.flush();
      MyTime tm("mdfa-i");
      string mdfaName = makeNdfaName(num_mdfa, "_from_", i);
      
      ++num_mdfa;
      
      cout << " num_mdfa: " << num_mdfa << " i= " << i << endl;
      dfa_tab_t *mdfa = dfas[i];
      dfas[i] = NULL;
      set<int> sigsInMdfa;
      sigsInMdfa.insert(i);
      augmentMdfa(mdfa, dfas, conflicts, sigsInMdfa, &skip,
		  max_mdfa_states, mdfaName);
    }
  }
  
  tm.lap("DONE grouping");
  delete_dfas(dfas);//keep valgrind happy
}

// Greedily group signatures DFAs into a smaller set of Moore-DFAs
void group_mdfa() {
  MyTime tm("group_mdfa");
  
  bool old_minimMDFA = enable_Moore_minimization_unless_option_overwritten();
  
  int max_mdfa_states = zop.dbgNumOption("MDFASTATES", MDFASTATES);
  vector<vector<int> > conflicts;
  vector<dfa_tab_t*> dfas;

  read_saved_signature_dfas(dfas);
  readConflictGraph(conflicts);
  unsigned int num_mdfa = 0;
  
  set<int> skip;
  maybeReadSkipSigs(skip);
  
  check_predefined_groups(dfas, conflicts, skip, max_mdfa_states, num_mdfa);
  
  assert(dfas[0]==0);//count from 1
  for (unsigned int i = 1; i < dfas.size(); ++i) {
    if (dfas[i] && !CONTAINS(skip, i)) {
      cout.flush();
      MyTime tm("mdfa-i");
      string mdfaName = makeNdfaName(num_mdfa, "_from_", i);
      
      ++num_mdfa;
      
      cout << " num_mdfa: " << num_mdfa << " i= " << i << endl;
      dfa_tab_t *mdfa = dfas[i];
      dfas[i] = NULL;
      set<int> sigsInMdfa;
      sigsInMdfa.insert(i);
      augmentMdfa(mdfa, dfas, conflicts, sigsInMdfa, &skip,
		  max_mdfa_states, mdfaName);
    }
  }
  
  tm.lap("DONE grouping");
  delete_dfas(dfas);//keep valgrind happy
  minimMDFA = old_minimMDFA;
}

///////////////////////////////////////////////////////////////////////////////
/*-----------------------------------------------------------------------------
 *get_options
 *---------------------------------------------------------------------------*/
const char* usage =
"(NOTE!: this is mostly obsolete... TBD: update it!)\n\
  -a <action> : action to perform\n\
\t Must specify exactly one action from\n\
\t build-signature-dfas: builds one minimized DFA for each signature in the file \n\
\t specified with -z sigFile:FILE . The file for the K-th signature is named sfa_INFIX_K, where \n\
\t INFIX is specified by -zdumpSigMdfa:INFIX \n\
\t \t Example: \
       mdfa -a build-signature-dfas -zsigFile:http-1400.SKIPPED-no-Duplicates.txt -z dumpSigMdfa:smdfa \n\
\t build-conflict-graph: Builds the conflict graph for the signatures in the signature file\n\
\t specified with -z sigFile:FILE, and whose DFAs are *redundantly* specified with -z readSigs:<prefix>\n\
\t \t Example: \
       mdfa -a build-conflict-graph -zsigFile:DBG-http -z readSigs:SMDFAs/sfa_smdfa_\n\
\t group-mdfa: Perform the DFA grouping algorithm from Fang Yu's paper \n\
\t \t Example: \
       mdfa -a group-mdfa -zCG:CG.DBG-http -zsigFile:DBG-http -z readSigs:SMDFAs.DBG-http/sfa_smdfa_  -zMDFASTATES=1000 \n\
\t build-bdfa-tree\n\
\t \t Example: \
       mdfa -a build-bdfa-tree -zBOTCH:files2botch -zPAYLOAD:../cs-trace.merged23.http.payload -zMDFASTATES=1000 2>&1 | tee log.tree\n\
\t scan-dfa-tree\n\
\t \t Example: \
       mdfa -a scan-bdfa-tree -zlogf:log.tree -zPAYLOAD:../cs-trace.merged23.http.payload \n\
  -T <num> : specify the number of threads to be used for build-conflict-graph.\n\
  -z : pass extra options\n\
  Ex.:\n\
    -z sigFile:FILE read the signatures mdfa from FILE \n\
    -z MDFASTATES:MAX Use at most MAX number of states in the mdfa.\n\
    -z GROUP:FILE : group signatures mentioned in FILE first\n\
    -z dumpSigMdfa:FILE : dump the signatures in files names sfa_FILE_<sig.number>. sig.number is index/line number in FILE\n\
    -z GROUP:FILE : group signatures mentioned in FILE first\n\
    -z skip:FILE : signatures to skip\n\
    -z minimMDFA:[0|1] if 0 don't use accept_id. if 1 use accept_id as in a Moore machine (MDFA)\n\
    -z justConflictGraph : quit after building the conflict graph\n\
    -z CG:FILE : read the conflict graph from FILE\n\
       the expected format is dumped by mdfa.c'buildConflictGraph:\n\
       (1) CONFLICTS 202 with 820 : 122 32 61\n\
       (2) COMPOSE 203 with 370 : 56 38 19\n\
       (3) CONFLICT4TRUNC 204 with 1277 : 1360 33 307\n\
    -z readSigs:PrefixFILE: don't scan the signatures. Insetead load them directly from files named PrefixFILE#NumberSignature\n\
    -z conflictWeight:K specify how to count conflicts between I and J in the conflict graph:\n\
       if N(I),N(J), N(I,J) are the number of states in mdfas for I,J and respectively I|J, then\n\
     if (N(I,J) <= N(I)+N(J)) thrn result = 0; // regardless of K\n\
     else if K is:\n\
       -1  (this used to be the default before Jul28th!... And it is again after Oct26) result = N(I,J)\n\
       1  (default) result = 1\n\
     NOTE the default conflictWeight is -1 (used to be 1 between [Jul28,Oct26], and -1 before)\n\
\n\
Examples:\n\
 mdfa -zsigFile:SIGFILE > CG : Dumps in CG the conflict graph for signatures in SIGFILE\n\
\n\
Example:\n\
Given a file containing signatures (one per line), say http.merged0.srt, the command:\n\
 mdfa -zsigFile:http.merged0.srt -zdumpSigMdfa:xxx | tee LOG\n\
does the following:\n\
 (1) reads the signatures from http.merged0.srt and builds the (Moore)DFA for each\n\
 (2) dumps (each) N-th signature in a file named sfa_xxx_N\n\
     After this, it will no longer needed to specify the signature file (http.merged0.srt).\n\
     The signatures can be read directly specifying the prefix for such signatures with\n\
     '-z readSigs:sfa_xxx_'. All such signatures from 1 to N are read, but N itself is\n\
     infered by reading the conflict graph (see bellow)\n\
 (3) computes the conflict graph and dumps it in text format (see buildConflictGraph) on the standard output,\n\
     together with other debugging messages. In this case, the output is captured in the file named LOG.\n\
     Now, the conflict graph can be extracted from LOG to a new file CG with the following command:\n\
       grep 'with' LOG > CG\n\
     and CG can be used in the future to avoid the expensive cost of computing the graph.\n\
 (4) after the conflict graph is built, Fang Yu's grouping algorithm kicks in and starts grouping signatures\n\
     into larger Moore-DFAs (mdfa's) named mdfa_K_from_S (where K ranges from 0 to the number of such mdfa's,\n\
     and S is the smallest signature part of the group -- a silly choice, I should remove this part).\n\
     It is possible to force grouping of certain signatures together, by using the '-z GROUP:GroupFile' option.\n\
     For each desired group, GroupFile must contain a line with the identifiers for the signatures part of that group.\n\
     The identifier for a signature is its line number in the signature file (in this case http.merged0.srt)\n\
";
void get_options(int argc, char *argv[])
{
  int c;
  extern char *optarg;
  
  while ((c = getopt(argc, argv, "a:z:T:")) != -1) {
    switch (c)
      {
      case 'a':
	if (!action.empty())
	  zop.usage_and_die("dupplicate 'action' argument");
	action = optarg;
	break;
      case 'z':
	zop.dbgOptionSet.insert(string(optarg));
	break;
/*
      case 'T':
      { int n=0;
	if ((sscanf(optarg, "%d", &n) != 1) || (n < 0)) {
	  zop.usage_and_die("Wrong number of threads specified with -T");
	}
	numThreads = n;
      }
	break;
*/
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
  cout << "Command line: ";
  for (int i = 0; i < argc; ++i) {
    cout << argv[i] << " ";
  }
  cout << endl;
  //TBD:need this? DUMP(PARANOIA) << " (should be a macro) " << endl;
  
  numThreads = zop.dbgNumOption("numThreads",numCPUs());
  //if (numThreads < 0)  zop.usage_and_die("Bad number of threads");
  condorProcess = zop.dbgNumOption("condorProcess", 0)+1; //Condor Processes start from 0
  numCondorProcesses = zop.dbgNumOption("numCondorProcesses", 1);
  if (//condorProcess < 0 ||
      numCondorProcesses <= 0 ||
      condorProcess > numCondorProcesses) zop.usage_and_die("Bad number of condor processes");
  minimMDFA = zop.dbgNumOption("minimMDFA",0) == 1;
  cout << "action = " << action << endl;
  cout << "numThreads = " << numThreads << endl;
  cout << "condorProcess = " << condorProcess << endl;
  cout << "numCondorProcesses = " << numCondorProcesses << endl;
  zop.dumpOptions();
  conflictWeight = zop.dbgNumOption("conflictWeight", -1);
  DMP(conflictWeight) << endl;
  DMP(minimMDFA) << endl; // DMP(zop.dbgNumOption("MDFASTATES", MDFASTATES));
  DMP(zop.dbgDoubleOption("falsePositives", -1)) << endl;
  if (action.empty())
    zop.usage_and_die("missing 'action' argument");
  if (zop.dbgDefinedOption("cutLim"))
    zop.usage_and_die("Obsolete option 'cutLim'. Use 'falsePositives' instead.");
}

void build_conflict_graph() {
  MyTime tm("build_conflict_graph");
  
  vector<vector<int> > conflicts;
  vector<dfa_tab_t*> dfas;
  read_saved_signature_dfas(dfas);
  
  // get the output file where the conflict graph is saved:
  string CGname = string("CG.") + zop.dbgStringOption("sigFile");
  if (!zop.dbgStringOption("condorProcess").empty())
    CGname = CGname + string(".") + zop.dbgStringOption("condorProcess");
  dout.write(CGname);
  buildConflictGraph(conflicts, dfas, 1);
  dout.close();
  
  delete_dfas(dfas);//keep valgrind happy
}

void static dbgReadPayload() {
  int fd = zop.optFileDescriptor("PAYLOAD");
  //bool dbgXOR = zop.dbgDefinedOption("dbgXOR");  bool dbgBytes = zop.dbgDefinedOption("dbgBytes");
  MyTime tm("readPayload");
  unsigned char buf[MAX_PACKET_SIZE];
  unsigned long packets_extracted = 0;
  unsigned long long total_bytes = 0;
  int size = 0;
  char dbg_xor = 0;
  
  while(read_packet(fd, buf, size)) {
#if 1
    packets_extracted ++;
    total_bytes+=size;
    for (int i=0; i<size; ++i)
      dbg_xor ^= buf[i];
#endif
  }
  tm.stop();
  DMP(packets_extracted);
  DMP(total_bytes);
  DMP(((int)dbg_xor));
}

void main_action() {
  // mdfa -a build-conflict-graph -zsigFile:DBG-http -z readSigs:SMDFAs/sfa_smdfa_
  if (action == "build-conflict-graph")
    build_conflict_graph();
  // mdfa -a group-mdfa -zCG:CG.DBG-http -zsigFile:DBG-http -z readSigs:SMDFAs/sfa_smdfa_  -zskip:skip -zMDFASTATES=1000  2>&1 | tee log.group-mdfa.1000
  else if (action == "group-mdfa") //TBD: debug output: what signatures go in what file ...
    group_mdfa();
  // mdfa -a build-bdfa-tree -zBOTCH:files2botch -zPAYLOAD:../cs-trace.merged23.http.payload -zMDFASTATES=1000 -zfalsePositives=0.015  2>&1 | tee log.botch
  else if (action == "build-bdfa-tree")
    build_bdfa_tree();
  // mdfa -a scan-bdfa-tree -zlogf:log.botch -zPAYLOAD:../cs-trace.merged23.http.payload 2>&1 | tee res.scan
  else if (action == "scan-bdfa-tree")
    scan_bdfa_tree(); // scan both the tree and the leaves
// debugging info:
  // mdfa -a dbg-botch -zMDFA2BOTCH:FILE [-zfalsePositives=0.015] [-zPAYLOAD:../cs-trace.merged23.http.payload]
  else if (action == "dbg-botch")
    dbg_botch();
  // mdfa -a get-tree-stats -zlogf:log.botch 2>&1 | tee res.tree-stats
  else if (action == "get-tree-stats")
    get_tree_stats(); // dump tree statistics
  // mdfa -a dbgReadPayload -zPAYLOAD:../cs-trace.merged23.http.payload
  //   gets info about how fast the payload is read.
  else if (action == "dbgReadPayload")
    dbgReadPayload();
  // mdfa -a dbgCheckTree -zPAYLOAD:../cs-trace.merged23.http.payload
  //   consistency check of the tree.
  else if (action == "dbgCheckTree")
    dfaTree::dbgCheckTree(zop.optFileDescriptor("PAYLOAD"));
  else
    zop.usage_and_die("Unknown 'action' (-a) argument");
}

int main(int argc, char *argv[])
{
  MyTime tm("main");
  zop.usage = usage;
  get_options(argc, argv);
  init(argc, argv);
  main_action();
  return 0;
}
