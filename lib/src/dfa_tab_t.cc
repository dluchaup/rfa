/*-----------------------------------------------------------------------------
 * File:    dfa_tab_t.cc
 * Author:  Daniel Luchaup
 * Date:    26 November 2013
 * Copyright 2007-2013 Daniel Luchaup luchaup@cs.wisc.edu
 *
 * This file contains unpublished confidential proprietary work of
 * Daniel Luchaup, Department of Computer Sciences, University of
 * Wisconsin--Madison.  No use of any sort, including execution, modification,
 * copying, storage, distribution, or reverse engineering is permitted without
 * the express written consent (for each kind of use) of Daniel Luchaup.
 *
 *---------------------------------------------------------------------------*/
#include <assert.h>
#include <string>
#include <list>
#include <vector>
#include <iostream>

#include "dfa_tab_t.h"
#include "utils.h"
#include "dbg.h"
#include "trex.h"
#include "tnfa.h"

#include "alphabet_byte.h"


using namespace std;
bool minimMDFA = true;

#if 0 //TBD: enable on per need basis
////////////////////////////////////////////////////////////////////////////////
#define NOCLASS  (class_id_t)(-1)
void dbgTrace(const Partition& , const vector<StateInfo> &si,
	      set<class_id_t>& W,
	      unsigned int *tab,
	      state_id_t s1, state_id_t s2, int a) {
  state_id_t t1 = ARR(tab, s1, a);
  state_id_t t2 = ARR(tab, s2, a);
  cout << "dbgTrace: ";
  
  DMP(s1);DMP(si[s1].c); DMP(a); DMP(t1); DMP(si[t1].c); DMP(CONTAINS(W, si[t1].c));
  DMP(s2);DMP(si[s2].c); DMP(a); DMP(t2); DMP(si[t2].c); DMP(CONTAINS(W, si[t2].c));
  cout << endl;
}
////////////////////////////////////////////////////////////////////////////////
// from nfa.cc:
bool dfa_tab_t::dbg_same(dfa_tab_t &other) {
  bool ok_states = num_states == other.num_states;
  bool ok_start = start == other.start;
  bool ok_tab = (0 == memcmp(tab, other.tab, num_states*sizeof(*tab)));
  //bool ok_acc = (0 == memcmp(acc, other.acc, num_states*sizeof(*acc)));
  bool ok_acc = true;
  for (state_id_t s=0; s < num_states; ++s) {
    if ((acc[s] == NULL) != (other.acc[s] == NULL)) {
      ok_acc = false;
      break;
    }
    if (acc[s] != NULL) {
      set<int> accept_id1;
      this->gather_accept_ids(s, accept_id1);
      set<int> accept_id2;
      other.gather_accept_ids(s, accept_id2);
      if (accept_id1 != accept_id2) {
	cout << " Distinct accept_id!" << endl;
	ok_acc = false;
	dumpContainer(accept_id1, "accept_id1");
	dumpContainer(accept_id2, "accept_id2");
	break;
      }
    }
  }
  
#define DUMP(x) cout << #x << "= " << (x) << " "
  DUMP(ok_states);
  DUMP(ok_start);
  DUMP(ok_tab);
  if (!ok_tab) {
    state_id_t errors = 0;
    for (state_id_t s=0; s < num_states; ++s) {
      for (unsigned int c = 0; c < MAX_SYMS; ++c) {
	if (ARR(tab, s, c) != ARR(other.tab, s, c)) {
	  cout << "Difference: " << s <<"[" << c << "," << (char)c << ",]: "
	       <<ARR(tab, s, c) << " != " << ARR(other.tab, s, c) << endl;
	  if (++errors == 10)
	    exit(1);
	}
      }
    }
  }
  DUMP(ok_acc);
//#undef DUMP
  return ok_states && ok_start && ok_tab && ok_acc;
}

int dfa_tab_t::num_accept_ids(state_id_t s) {
  int num = 0;
  if (this->acc[s]) {
    for (int *ptr = this->acc[s];  (*ptr) != -1;  ++ptr) {// OLD STYLE
      cout << *ptr << endl;
      assert(*ptr > 0);
      num++;
    }
  }
  return num;
}

int* dfa_tab_t::clone_accept_ids(state_id_t s) {
  int *res = NULL;
  int num = num_accept_ids(s);
  if (num) {
    res = new int[num + 1];
    int i;
    for (i = 0; i < num; ++i) {// OLD STYLE
      res[i] = acc[s][i];
    }
    assert(acc[s][i] == -1); // OLD STYLE
    res[i] = acc[s][i]; // == -1 OLD STYLE
  }
  return res;
}

bool dfa_tab_t::bin_dfa_load(const char *filename) {
 /*
   dfa_tab_t::dump "incorrectly"(?) saves the 'acc' pointer array. This only
   tells whether a state is accepting or not, which is OK if the DFA is not
   a Moore DFA. For Moore DFAs, dumping and loading should also account for
   the content of the 'acc', which should be saved and restored properly.
   In the OLD STYLE the acc array ends with a sentinel element equal to -1.
  */
 assert(0&&"THIS IS NOT COMPATIBLE with new dfa_tab_t->acc");
 int ifd;

  if (filename && filename[0]) {
    ifd = open(filename, O_RDONLY);
    if (ifd == -1) {
      fprintf(stdout, "ERROR:  could not open file %s for reading.\n"
	      "Reason: %s\n", filename, strerror(errno));
      exit(1);
      return false;
    }
  } else
    return false;
  
  if (acc)
    delete[]acc;
  if (tab)
    delete[]tab;
  
  assert(read(ifd, &num_states, sizeof(num_states)) == sizeof(num_states));
  assert(read(ifd, &start, sizeof(start)) == sizeof(start));
  this->acc = new int *[num_states];
  assert((size_t)read(ifd, acc, (num_states *sizeof(*acc)))
	 == (num_states *sizeof(*acc)));
  this->tab = new unsigned int [num_states*MAX_SYMS];
  assert((size_t)read(ifd, tab, ((num_states*MAX_SYMS)*sizeof(*tab)))
	 == ((num_states*MAX_SYMS)*sizeof(*tab)));
  close(ifd);
  return true;
}

bool dfa_dump(const dfa_tab_t *dt, const char *filename, const char *kind) {
  if (kind) {
    if (strcmp(kind,"bin"))
      return dt->bin_dfa_dump(filename);
    else if (strcmp(kind,"txt"))
      return dt->txt_dfa_dump(filename);
    fprintf(stderr, "Incorrect dump kind %s!\n", kind);
  } else {
    fprintf(stderr, "dump kind unspecified");
  }
  assert(false);
  return false;
}

void dbgCheck(dfa_tab_t *dt, const char *msg) {
  return;
  assert(dt);
  if (msg) cout << endl << dt << " : " << msg << " num_states=" << dt->num_states << " ";  cout.flush();
  assert(dt->num_states > 0);
  for (unsigned int i=0; i < dt->num_states; i++) {
    assert(dt->acc);
    if (dt->acc[i]) {
      //DMP(dt->acc[i]);
      assert(dt->acc[i] != (void*)-1);
      assert(dt->acc[i] != (void*)0x1);
      assert(dt->acc[i][0] != -1);//no ACCEPT_PTR_HACK
      for (int *ptr = dt->acc[i];  (*ptr) != -1;  ++ptr) {//OLD STYLE
	//(cout << " i= " << i << " ptr - acc[i] = " << (ptr - dt->acc[i]) ).flush();
	//(cout << " ptr = " << ptr).flush();
	//cout << " *ptr= " << *ptr << endl; cout.flush();
	if (*ptr <= 0)  {DMP(ptr - dt->acc[i]); DMP(*ptr);DMP(i);}
	assert(*ptr > 0);
	
	if (0) // if check for order
	  if (ptr > dt->acc[i]) { assert(ptr[-1] < ptr[0]); }
	
	//cout << " next ptr ..."; cout.flush();
      }
      //cout << " done " << i << " "; cout.flush();
    }
  }
  //cout << " OK \n"; cout.flush();
}
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
bool dfa_tab_t::txt_dfa_dump(const char *filename) {
  static DbgTime tm("dfa_tab_t::txt_dfa_dump", false);
  tm.resume();
  dbgCheck(this);
   FILE *out = NULL;
   if (filename && filename[0]) {
      out = fopen(filename, "w");
      if (out == NULL) {
	 fprintf(stdout, "ERROR:  could not open file %s for writing.\n"
		 "Reason: %s\n", filename, strerror(errno));
	 return false;
      }
   } else
      out = stderr;
   
   /* number of states, start state */
   fprintf(out, "Number of states:%u. Start state:%u\n",
	   (uint)num_states, (uint)start);
   
   /* transitions */
   for (unsigned int i=0; i < num_states; i++) {
      state_id_t src = i;
      fprintf(out, "state%u", src);
      if (acc[src]) {
	assert(acc[src] != (void*)1);
	assert(acc[src] != (void*)-1);
	assert(acc[src][0]!=-1);
	fprintf(out, " a(");
	for (int *ptr = acc[src];
	     *ptr != -1;
	     ++ptr) {//OLD STYLE
	  if (*ptr <= 0) {
	    DMP(i);DMP(*ptr); DMP(ptr-acc[src]);
	  }
	  assert(*ptr > 0);
	  fprintf(out, "%d ", (*ptr));
	}
	fprintf(out, "):");
      } else
	 fprintf(out, ":");
      /* destinations */
      for (unsigned int c=0; c < MAX_SYMS; c++) {
	 if (c > 0)
	    fprintf(out, ",");
	 fprintf(out, "%u", ARR(tab, src,c));
      }
      fprintf(out, "\n");
   }
   fprintf(out, "id:%u", machine_id);
   fclose (out);
   printf("DBG: Done Dumping DFA to %s\n", filename);fflush(0);
   tm.stop();

   //bin_dfa_dump((string(filename)+"bin").c_str());
   return true;
}
bool dfa_tab_t::bin_dfa_dump(const char *filename) {
  static DbgTime tm("dfa_tab_t::bin_dfa_dump", false);
  tm.resume();
  int ofd;

  if (filename && filename[0]) {
    ofd = open(filename, O_RDWR|O_TRUNC|O_CREAT,S_IRUSR|S_IWUSR);
    if (ofd == -1) {
      fprintf(stdout, "ERROR:  could not open file %s for writing.\n"
	      "Reason: %s\n", filename, strerror(errno));
      return false;
    }
  } else
    return false;

  assert(write(ofd, &num_states, sizeof(num_states)) == sizeof(num_states));
  assert(write(ofd, &start, sizeof(start)) == sizeof(start));
  assert((size_t)write(ofd, acc, (num_states *sizeof(*acc)))
	 == (num_states *sizeof(*acc)));
  assert((size_t)write(ofd, tab, ((num_states*MAX_SYMS)*sizeof(*tab)))
	 == ((num_states*MAX_SYMS)*sizeof(*tab)));
  close(ofd);
  return true;
}


static void consume(FILE *in, const char* token) {
   assert(token);
   while (*token)
      assert(*token++ == fgetc(in));
}

bool dfa_tab_t::txt_dfa_load(const char *filename) {
  static MyTime tm("dfa_tab_t::txt_dfa_load", false);
  tm.resume();
  FILE *in = NULL;
  if (filename && filename[0]) {
    in = fopen(filename, "r");
    if (in == NULL) {
      fprintf(stdout, "ERROR:  could not open file %s for reading.\n"
	      "Reason: %s\n", filename, strerror(errno));
      return false;
    }
  } else
    return false;
   
  /* number of states. start state */
  state_id_t size;
  state_id_t start;
  assert(fscanf(in, "Number of states:%u. Start state:%u\n",
		&size, &start) == 2);

  this->num_states = size;
  this->start = start;
  this->acc = new int *[size];
  this->tab = new unsigned int [size*MAX_SYMS];
#define ACCEPT_PTR_HACK (new int[1])//((int*)0x1)
  
#ifdef STATS
   this->stat = new unsigned long long [size];
   for (unsigned int i=0; i<size; ++i)
      this->stat[i]=0;
#else
   this->stat = NULL;
#endif
   bool dbg_seen[size];

   for (unsigned int i=0; i < size; i++) {
     this->acc[i] = NULL;
     dbg_seen[i] = false;
   }
  
   /* transitions */
   for (unsigned int i=0; i < size; i++) {
      char next_c;
      state_id_t src;
      
      assert(fscanf(in, "state%u", &src) == 1);
      assert(src < size);
      assert(dbg_seen[src] == false); dbg_seen[src] = true;
      next_c = fgetc(in);
      if (next_c != ':') {
	 set<int> accept_id;
	 assert(next_c == ' ');
	 consume(in, "a(");
	 //this->acc[src] = ACCEPT_PTR_HACK; this->acc[src][0]=-1;
	 int id;
	 while (fscanf(in, "%d ", &id) > 0)
	   accept_id.insert(id);
	 consume(in, "):");
	 this->acc[src] = new int[accept_id.size()+1];//OLD STYLE
	 copy(accept_id.begin(), accept_id.end(),this->acc[src]);
	 this->acc[src][accept_id.size()] = -1;//OLD STYLE
      }
      
      /* destinations */
      for (unsigned int c=0; c < MAX_SYMS; c++) {
	 unsigned dst = 0;
	 if (c > 0)
	    consume(in, ",");
	 assert(fscanf(in, "%u", &dst) > 0);
	 assert(dst < size);
	 //result_dfa->states[src].trans[c].push_back(dst);
	 ARR(this->tab, src, c) = dst;
      }
      consume(in, "\n");
   }
   if (fscanf(in, "id:%u", &(this->machine_id)) == 1) {
     //machine id was not always there, for older dumps ... 
     ;
   }
   this->filename = filename;
   fclose (in);
   dbgCheck(this);
   tm.stop();
   return true;
}

typedef state_id_t class_id_t;
const class_id_t illegal_class = illegal_state;

bool dfa_tab_t::strict_match(const unsigned char *buf, unsigned int len)
{
  if (this->num_states == 0)
    return false;
  
  unsigned int i;

  bool found_match = false;
  unsigned int cur = this->start;
  for (i=0; i < len; i++) {
    cur = ARR((this->tab), cur, buf[i]);
  }
  found_match = this->acc[cur]; 
  return found_match;
}

typedef list<state_id_t> StateList;

typedef StateList Class; // States in a Class can be collapsed into one state
typedef vector<Class> Partition;

typedef struct StateInfo{
  class_id_t c;
  vector<StateList> preds;
  StateInfo();
} StateInfo;

StateInfo::StateInfo() {
  preds.resize(MAX_SYMS);
  //cout << "resized: " << preds.size() << endl;
}

void getPreds(const Class& c, unsigned int a,
	      vector<StateInfo> &si, StateList& res) {
  res.clear();
  for(Class::const_iterator it = c.begin(); it != c.end(); ++it) {
    state_id_t s = *it;
    res.insert(res.end(), si[s].preds[a].begin(),si[s].preds[a].end());
  }
}


int dbgOut = 0;
void dfa_tab_t::minimize(bool clean_old)
{
  Partition P;
  P.reserve(this->num_states);
  vector<StateInfo> si(this->num_states);
  
  if (minimMDFA) {
    // Create possible multiple initial partitions for the final/accepting
    // states based on the accept_id set.
    assert(P.size() == 0);
    for (state_id_t s = 0; s < this->num_states; s++) {
      if (this->acc[s]) {
	set<int> acceptIds2;
	for (int *ptr2 = this->acc[s]; ptr2 && (*ptr2) != -1;  ++ptr2) {
	  acceptIds2.insert(*ptr2);
	}
	assert(!acceptIds2.empty());
	//dbgDump(acceptIds2,"acceptIds2");
	
	unsigned int cls = 0;
	for (cls = 0; cls < P.size(); ++cls) { // Find proper class for 's'
	  state_id_t member = P[cls].front();
	  assert(this->acc[member]);
	  set<int> acceptIds1;
	  bool same = true;
	  for (int *ptr1 = this->acc[member]; same && ptr1 && (*ptr1) != -1;  ++ptr1) {
	    //if (CONTAINS(acceptIds1, (*ptr1))) {      (DMP(*ptr1)).flush();exit(1);    }
	    assert(!CONTAINS(acceptIds1, (*ptr1)));
	    acceptIds1.insert(*ptr1);
	    if (!CONTAINS(acceptIds2, (*ptr1)))
	      same = false;
	  }
	  assert(!acceptIds1.empty());
	  //dbgDump(acceptIds1,"acceptIds1");

	  same = same && (acceptIds1.size() == acceptIds2.size());
	  
	  //dbgDump(acceptIds1,"equal? acceptIds1");
	  //dbgDump(acceptIds2,"equal? acceptIds2");
	  //if (acceptIds2 == acceptIds2)
	  if (same) {// s belongs to 'cls' class
	    break;
	  }
	}
	if (cls == P.size()) // s belongs to a class of its own.
	  P.resize(P.size()+1);
	si[s].c = cls;
	P[si[s].c].push_back(s);
      }
    }
  }
  else { // Create a single initial partition for all final states
    const class_id_t FinalIX = 0;    // Class& Final    = P[FinalIX];
    P.resize(1); // for FinalIX
    for (state_id_t s = 0; s < this->num_states; s++) {
      if (this->acc[s]) {
	si[s].c = FinalIX;
	P[si[s].c].push_back(s);
      }
    }
    assert(!P[FinalIX].empty());//The DFA must accept something! (unless used for language intersection: TBD)
  }

  size_t dbgNumFinals = 0;
  class_id_t NonFinalIX = illegal_class; // None yet known
  for (state_id_t s = 0; s < this->num_states; s++) { // does 2 things: nonFinals + preds.
    if (!this->acc[s]) {
      if (NonFinalIX == illegal_class)
	P.resize(1 + (NonFinalIX = P.size()));
      si[s].c = NonFinalIX;
      P[si[s].c].push_back(s);
    } else
      ++dbgNumFinals;
    
    for (unsigned int c=0; c < MAX_SYMS; c++) {
      state_id_t t = ARR((this->tab), s, c);
      si[t].preds[c].push_back(s);
    }
  }
  
  class_id_t P_size = P.size();
  //if (P_size == 1) //just one class, and is 'minimized' - sort of
  //  return;
  
  set<class_id_t> W;
  for (unsigned int cls = 0; cls < P.size(); ++cls) {
    assert(!P[cls].empty());
    W.insert(cls);
  }
  
  unsigned int dbg_w_count = 0;
  //if (dbgOut) dbgDump(P[FinalIX],"Final");
  
  //size_t dbgNumFinals = P[FinalIX].size();
  
  while(!W.empty()) {
    class_id_t cls = *W.begin();
    bool split_cls = false;
    W.erase(W.begin());
    Class S = P[cls]; // Splitter/Class
    set<state_id_t> SSet;
    SSet.insert(S.begin(),S.end());
    if (dbgOut) cout << "Splitter Class id: " << cls << " ";//<< endl;
    if (dbgOut) dbgDump(S,"Splitter States:");
    
    for (unsigned int a=0; a < MAX_SYMS && !split_cls; a++) {
      StateList la;
      getPreds(S, a, si, la); //set of States that transition in Splitter on a
      
      set<class_id_t> Rset;
      list<class_id_t> Rlist; // Classes that have some states that transition to Splitter on 'a'
      for (StateList::iterator it = la.begin(); it != la.end(); ++it) {
	state_id_t s_pred = *it;
	if (Rset.insert(si[s_pred].c).second) {// Newly inserted
	  if (si[s_pred].c != cls)
	    Rlist.push_back(si[s_pred].c);
	}
	state_id_t t = ARR((this->tab), s_pred, a);
	assert(CONTAINS(SSet, t));
      }
      if (CONTAINS(Rset, cls))
	Rlist.push_back(cls);
      
      for (list<class_id_t>::iterator it = Rlist.begin(); it != Rlist.end(); ++it) {
	class_id_t c = *it;
	StateList notInLa;
	class_id_t next_class_id = P.size();
	Class& Cl = P[c]; // Don't use after P is resized!
	Class::iterator next;
	size_t dbgSz1 = Cl.size();
	if (dbgOut)  dbgDump(Cl,"Cl");
	for(Class::iterator sit = next = Cl.begin(); sit != Cl.end(); sit = next) {
	  next++;
	  state_id_t s = *sit;
	  state_id_t t = ARR((this->tab), s, a);
	  if (dbgOut) cout << s << " " << a << " " << t << endl;
	  if (!CONTAINS(SSet, t)) {
	    notInLa.push_back(s);//TBD: optimization: use multiple lists: one per 'target' class. Nope: need just 2 for n log(n).
	    Cl.erase(sit);
	    si[s].c = next_class_id;
	  }
	}
	
	size_t Cl_size = Cl.size();
	assert(Cl_size > 0);
	assert(dbgSz1 == Cl_size + notInLa.size());
	if (!notInLa.empty()) {
	  P.push_back(notInLa);//optimization possible: direct in P[next_class_id].
	  P_size++;
	  if (CONTAINS(W,c))
	    W.insert(next_class_id);
	  else {
	    size_t new_splitter = (Cl_size < notInLa.size())? c: next_class_id;
	    W.insert(new_splitter);
	  }
	}
      }
    }
    if (dbgOut) cout << "dbg_w_count= " << dbg_w_count++ << endl;
  }
  
  if (dbgOut) cout << "dbg_w_count= " << dbg_w_count++ << endl;
  if (dbgOut) cout << num_states << " dfa_tab_t::minimize " << P.size() << " " << P_size << endl;

  if (dbgOut) cout << "Creating the new DFA + Consistency checking ...\n";
  class_id_t newStart = illegal_class;
  unsigned int *newTab = new unsigned int [MAX_SYMS*P_size];//DBGNEW(newTab);
  int **newAcc = new int* [P_size];
  assert(P_size == P.size() && "Otherwise may need to use P_size as exit cond");
  set<class_id_t> dbgSeen;
  size_t dbgNumFinals2 = 0;
  
  for (class_id_t cur = 0; cur != P_size; ++cur) {
    Class& Cl = P[cur];
    assert(!Cl.empty());
    state_id_t s0 = *Cl.begin();
    newAcc[cur] = NULL;
    
    set<unsigned int> acceptIds;
    bool isAccept = false;
    
    for(Class::iterator sit = Cl.begin(); sit != Cl.end(); ++sit) {
      state_id_t s = *sit;
      assert(si[s].c == cur);
      assert(!CONTAINS(dbgSeen, s));
      dbgSeen.insert(s);
      
      if (s == this->start) {
	assert(newStart == illegal_class);
	newStart = cur;
      }
      
      for (int *ptr1 = this->acc[s]; ptr1 && (*ptr1) != -1;  ++ptr1) {
	//DUMP(*ptr1);
	acceptIds.insert(*ptr1);
      }
      
      if (this->acc[s]) {
	isAccept = true;
	++dbgNumFinals2;
      }
    }
    if (!acceptIds.empty()) {// Assuming no ACCEPT_PTR_HACK ...
      newAcc[cur] = new int [1 + acceptIds.size()];
      copy(acceptIds.begin(), acceptIds.end(), newAcc[cur]);
      newAcc[cur][acceptIds.size()] = -1;
    }
    else if (isAccept) {newAcc[cur] = ACCEPT_PTR_HACK; newAcc[cur][0]=-1;}
    
    for (unsigned int a=0; a < MAX_SYMS; a++) {
      state_id_t t0 = ARR((this->tab), s0, a);
      ARR(newTab, cur, a) = si[t0].c;
      for(Class::iterator sit = Cl.begin(); sit != Cl.end(); ++sit) {//Consistency check
	state_id_t s = *sit;
	state_id_t t = ARR((this->tab), s, a);
	if (si[t].c != si[t0].c) {
	  DMP(cur); DMP(a);DMP(s0);DMP(t0);DMP(si[t0].c);DMP(s);DMP(t);DMP(si[t].c);
	}
	assert(si[t].c == si[t0].c);
      }
    }
  }

  assert(dbgNumFinals2 > 0 && dbgNumFinals2 <= dbgNumFinals);
  assert(newStart != illegal_class);
  assert(dbgSeen.size() == num_states);
  assert(P_size <= num_states);
  
  if (dbgOut) cout << "Checking OK" << endl;
  unsigned int machine_id = this->machine_id;
  if (clean_old)
    this->cleanup();
  this->tab = newTab;
  this->num_states = P_size;
  this->acc = newAcc;
  this->start = newStart;
  this->machine_id = machine_id;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
/*-----------------------------------------------------------------------------
 *dfa_tab_t::cleanup
 *  frees up all the used memory
 *---------------------------------------------------------------------------*/
void dfa_tab_t::partial_cleanup_for_resize(state_id_t lim)
{
   for (state_id_t i = lim; i < this->num_states; i++)
   {
     //this will fail when used with older TAB file-formats!
     if (this->acc[i])//  && this->acc[i] != ((int*)0x1)// see ACCEPT_PTR_HACK
      {
	assert(acc[i] != (void*)1);
	assert(acc[i] != (void*)-1);
	assert(acc[i][0]!=-1);
	delete[] this->acc[i];
	this->acc[i] = NULL;// so that cleanup() will skip it
      }
   }
}

void dfa_tab_t::cleanup(void)
{
  //cerr << " cleaning " << id << endl;

   delete[] this->tab;
   this->tab = 0;

   partial_cleanup_for_resize(0);
   
   delete[] this->acc;//ok.checked
   this->acc = 0;

   this->num_states = 0;
   this->start = 0;
}

void dfa_tab_t::init(const char *rex, unsigned int id) {
  trex *trx = regex2trex(rex, NULL, NULL);
  stateNFA *snfa = trx->toNFA(NULL);
  assert(snfa->is_root());
  dfa_tab_t *tmp = snfa->make_dfa(id, this);
  assert(tmp == this);
  stateNFA::delete_all_reachable(snfa);
  delete trx;
}

void dfa_tab_t::dfa2nfa2dotty(const char *) {
  assert(0 && "TBD: from nfa.cc");
  //   nfa_t* n = tab2nfa(this);
  //   n->xfa_output(filename, 1);
  //   delete n;
}

#include <map>
#include <string.h>
/* combine */
enum CombineKind{
  CK_UNION,
  CK_INTERSECTION,
  CK_DIFFERENCE
};
typedef unsigned long long pair_state_id_t;
static void merge_sorted_accept_ids(int* &acc_dest, // OLD STYLE
				    int* acc_src1, int* acc_src2,
				    CombineKind ckind);
static dfa_tab_t* combine(const dfa_tab_t* dt1, const dfa_tab_t* dt2,
			  unsigned int max_states, CombineKind ckind)
{

#define MakeKey(dest, k1, k2)  {(dest = k1); (dest) = (((dest) << (8*sizeof(k1)))|(k2));}
#define FirstInKey(dest)  ((dest) >> (8*sizeof(dest)/2))
#define SecondInKey(dest) ((dest) & ((state_id_t) -1))
  assert(sizeof(unsigned long long) == 2*sizeof(state_id_t));


  /* the worklist keeps track of the nodes that we still need to process */
  std::list<pair_state_id_t>  wl;
  // TBD: Use a (rectangular) bitset where key(s1,s2) = s1*num_states2 +s2 ...
  std::map<pair_state_id_t, state_id_t> comb_nodes;
  std::map<pair_state_id_t, state_id_t>::iterator comb_nodes_iter;

  
  //unsigned int machine_id;
  state_id_t num_states = 0;
  state_id_t start;
  assert((max_states > 0) && "Otherwise we have to fix the sizes bellow");
  state_id_t allowed_num_states = dt1->num_states*dt2->num_states;
  if (max_states > 0 && allowed_num_states > max_states)
    allowed_num_states = max_states;
  //TBD fix this: use allowed_num_states(not max_states) as a limit in the following code
  
  state_id_t *tab = new state_id_t[MAX_SYMS*allowed_num_states];//over-allocate ...
  assert(tab);
  
  /* key         -- work state 
   * destkey     -- key node corresponding to the trans dest
   * st_data_src -- the combined node corresponding to the key
   * st_data_dst -- the combined node corresponding to the trans dest.
   */
  pair_state_id_t key;
  pair_state_id_t destkey;
  state_id_t st_data_src;
  state_id_t st_data_dst;
  
  /* initialize the worklist with the combined start states,
   *  add the new entry to the hash list
   *  add the entry to the combined rca */
  {

    MakeKey(key, dt1->start, dt2->start);
    assert(FirstInKey(key) == dt1->start); assert(SecondInKey(key) == dt2->start);
    
    wl.push_back(key);
    comb_nodes[key] = start = num_states;
    num_states++;
  }
  
  /* Main iteration loop: as long as the worklist is not empty, there are
   * "crossed" nodes that have been created but not fully processed yet.
   * Each iteration through the loop removes and processes one node from
   * the worklist; nodes that have been processed are never placed on the
   * worklist again.  Multiple nodes may be added during each iteration,
   * but the process eventually terminates since the number of combined
   * nodes |first| X |second| is finite. */
  while (!wl.empty() && 
	 !(max_states > 0 && num_states >= max_states) )
    { assert(num_states < max_states);
      /* denote key as <s, t> */
      key = wl.front(); wl.pop_front();
      
      /* pull the combined state out of our data structure */
      comb_nodes_iter = comb_nodes.find(key);
      assert(!(comb_nodes_iter == comb_nodes.end()));
      st_data_src = (*comb_nodes_iter).second;

      /* now, look at each symbol and build transitions to new combined
	 states as necessary */
      for (unsigned int i=0; i < MAX_SYMS; i++)
	{
	  if ((max_states > 0 && num_states >= max_states))
	    break;
	  /* look at the i^th symbol in each machine. */
	  state_id_t d1 = ARR(dt1->tab, FirstInKey(key), i);
	  state_id_t d2 = ARR(dt2->tab, SecondInKey(key),i);
	  MakeKey(destkey, d1, d2);
	  assert(FirstInKey(destkey) == d1);
	  assert(SecondInKey(destkey) == d2);
	  
	  /* Check to see if we have seen the crossed destination
	   * node destkey = <s',t'> before.  
	   * No - create new data node, 
	   add to worklist and to hash table
	   add new combined node to combined rca
	   * Yes - do nothing */
	  
	  if ((comb_nodes_iter = comb_nodes.find(destkey)) == comb_nodes.end())
	    {
	      /* create a new data node and add to worklist and hash table*/
	      comb_nodes[destkey] = st_data_dst = num_states;
	      wl.push_back(destkey);
	      num_states++;
	    }
	  else
	    {
	      /* the destination node already exists, just get it. */
	      st_data_dst = (*comb_nodes_iter).second;
	    }
	  
	  /* create a transition from the src node in the combined
	   * RCA to the dest node in the combined RCA.*/
	  ARR(tab, st_data_src, i) = st_data_dst;
	}
    }

  dfa_tab_t *combined_dt = NULL;
  /* if we exceeded memory, then we need to report as such.  Setting
   * the parameter to 0 indicates we exceeded the available # of states */
  if ( max_states <= 0 || num_states < max_states) {
    combined_dt = new dfa_tab_t();
    combined_dt->machine_id = dt1->machine_id + dt2->machine_id;//dl: this seems wrong
    combined_dt->num_states = num_states;
    combined_dt->start = start;
    
    combined_dt->tab = new state_id_t [MAX_SYMS*num_states];
    memcpy(combined_dt->tab, tab, MAX_SYMS*num_states*sizeof(tab[0]));
    combined_dt->acc = new int* [num_states];
  }
  
  delete [] tab;
  if (combined_dt) {
    state_id_t dbgCount = 0;
    state_id_t num_accepting_states = 0;
    for (comb_nodes_iter = comb_nodes.begin();
	 comb_nodes_iter != comb_nodes.end();
	 ++comb_nodes_iter) {
      ++dbgCount;
      key = comb_nodes_iter->first;
      state_id_t s1 = FirstInKey(key);
      state_id_t s2 = SecondInKey(key);
      state_id_t s = comb_nodes_iter->second;
      assert(s < num_states);
      assert(s1 < dt1->num_states);
      assert(s2 < dt2->num_states);
      combined_dt->acc[s] = NULL;
      merge_sorted_accept_ids(combined_dt->acc[s], dt1->acc[s1], dt2->acc[s2],
			      ckind);
      if (combined_dt->acc[s] != NULL)
	++num_accepting_states;
    }
    assert(dbgCount == num_states);
    if (num_accepting_states == 0) {   // Resulting DFA doesn't accept anything
      assert(ckind == CK_INTERSECTION
	     || ckind == CK_DIFFERENCE);
      if (combined_dt->acc)
	delete[](combined_dt->acc);
      combined_dt->acc = NULL;
      if (combined_dt->tab)
	delete[](combined_dt->tab);
      combined_dt->tab = NULL;
      combined_dt->num_states = 0;
      combined_dt->start = illegal_state;
    }
  }

  //combined_dt->dfa2nfa2dotty("dot.combined_dt.new");
  //DMPNL(combined_dt->num_states);
  return combined_dt;
}

static void merge_sorted_accept_ids(int* &acc_dest, // OLD STYLE
				    int* acc_src1, int* acc_src2,
				    CombineKind ckind) {
  assert(acc_dest == NULL);
  int *p1 = NULL;
  int *p2 = NULL;
  if (acc_src1) {
    for (p1 = acc_src1; *p1 != -1; ++p1) {
      if (p1 > acc_src1)
	assert(p1[-1] < p1[0]);
    }
  }
  if (acc_src2) { 
    for (p2 = acc_src2; *p2 != -1; ++p2) {
      if (p2 > acc_src2)
	assert(p2[-1] < p2[0]);
    }
  }

  std::vector<int> vres;
  switch (ckind) { 
  case CK_UNION:
    //union: preserves Moore info, if any...
    p1 = acc_src1;
    p2 = acc_src2;
    while (p1 && p2 && *p1!= -1 && *p2 != -1) {
      if (*p1 < *p2) {
	vres.push_back(*p1);
	++p1;
      } else if (*p2 < *p1) {
	vres.push_back(*p2);
	++p2;
      }
      else { // (*p1 == *p2)
	vres.push_back(*p1);
	++p1;
	++p2;
      }
    }
    while (p1 && *p1!= -1) {
      vres.push_back(*p1);
      ++p1;
    }
    while (p2 && *p2!= -1) {
      vres.push_back(*p2);
      ++p2;
    }
    break;
  case CK_INTERSECTION:
    //intersection: unclear semantics for Moore info.
    //So, we ignore Moore information for now.
    //Assert that there is no Moore info ...
    assert(acc_src1 == NULL || (acc_src1[0]==1 && acc_src1[1]==-1));
    assert(acc_src2 == NULL || (acc_src2[0]==1 && acc_src2[1]==-1));
    if (acc_src1 && acc_src2) {
      assert(acc_src1[0]==acc_src2[0]); // Redundant. No Moore info.
      vres.push_back(acc_src1[0]);
    }
    break;
  case CK_DIFFERENCE:
    //difference: unclear semantics for Moore info.
    //So, we ignore Moore information for now.
    //Assert that there is no Moore info ...
    assert(acc_src1 == NULL || (acc_src1[0]==1 && acc_src1[1]==-1));
    assert(acc_src2 == NULL || (acc_src2[0]==1 && acc_src2[1]==-1));
    if (acc_src1 && !acc_src2) {
      vres.push_back(acc_src1[0]);
    }
    break;
  default: assert(false && "Unknown dfa combination kind.");
  }
  if (!vres.empty()) {
    acc_dest = new int[vres.size()+1];//OLD STYLE
    acc_dest[vres.size()] = -1;
    copy(vres.begin(), vres.end(), acc_dest);
    assert(acc_dest[vres.size()] == -1);
  }
  return;
}

/* Get a new DFA which accepts the union of dt1 and dt2's languages        */
dfa_tab_t* dt_union(const dfa_tab_t* dt1, const dfa_tab_t* dt2,
		    unsigned int max_states, bool minimize) {
  dfa_tab_t * res = combine(dt1, dt2, max_states, CK_UNION);
  if (minimize && (res != NULL))
    res->minimize();
  return res;
}

/* Get a new DFA which accepts the intersection of dt1 and dt2's languages */
dfa_tab_t* dt_intersection(const dfa_tab_t* dt1, const dfa_tab_t* dt2,
			   unsigned int max_states) {
  dfa_tab_t * res = combine(dt1, dt2, max_states, CK_INTERSECTION);
  if (res && res->num_states > 0)
    res->minimize();
  return res;
}

/* Get a new DFA which accepts the difference of dt1 and dt2's languages   */
dfa_tab_t* dt_difference(const dfa_tab_t* dt1, const dfa_tab_t* dt2,
			 unsigned int max_states) {
  dfa_tab_t * res = combine(dt1, dt2, max_states, CK_DIFFERENCE);
  if (res && res->num_states > 0)
    res->minimize();
  return res;
}

/* If do_clone == true, get a new identical copy/clone of dt               */
/* Otherwise, gets a new DFA that accepts dt's complement                  */
static dfa_tab_t* dt_clone_or_complement(const dfa_tab_t* dt, bool do_clone) {
  dfa_tab_t *res = new dfa_tab_t();
  res->machine_id = dt->machine_id;//dl: is this needed?
  state_id_t num_states = res->num_states = dt->num_states;
  res->start = dt->start;
  
  res->tab = new state_id_t [MAX_SYMS*(num_states)];
  memcpy(res->tab, dt->tab, MAX_SYMS*(num_states)*sizeof(res->tab[0]));
  res->acc = new int* [num_states];
  for (state_id_t s=0; s < num_states; ++s) {
    if (do_clone) { // clone
      if (dt->acc[s] != NULL) { //OLD STYLE
	//assert(dt->acc[s][0]! = -1);//sanity
	int size=0;
	while(dt->acc[s][size] != -1)
	  ++size;
	res->acc[s] = new int[size+1];
	memcpy(res->acc[s], dt->acc[s], (size+1)*sizeof(res->acc[s][0]));
      }
      else
	res->acc[s] = NULL;
    } else { //complement
      if (dt->acc[s] == NULL) { //OLD STYLE
	res->acc[s] = new int[2];
	res->acc[s][0] = 1;
	res->acc[s][1] = -1;
      }
      else
	res->acc[s] = NULL;
    }
  }
  return res;
}

/* Get a new identical copy/clone of dt                                    */
dfa_tab_t* dt_clone(const dfa_tab_t* dt) {
  return dt_clone_or_complement(dt, true);
}

/* Get a new DFA which accepts the complement of dt's language             */
dfa_tab_t* dt_complement  (const dfa_tab_t* dt) {
  return dt_clone_or_complement(dt, false);
}

/* Change dt in place s.t. it accepts the complement of original language  */
void dt_negate  (dfa_tab_t* dt) {
  state_id_t num_states = dt->num_states;
  for (state_id_t s=0; s < num_states; ++s) {
    if (dt->acc[s] == NULL) { //OLD STYLE
      dt->acc[s] = new int[2];
      dt->acc[s][0] = 1;
      dt->acc[s][1] = -1;
    } else {
      delete[] dt->acc[s];
      dt->acc[s] = NULL;
    }
  }
}

/*****************************************************************************/
void dfa_tab_t::gather_accept_ids(state_id_t s, set<int> & accept_ids) const {
  for (const int* p = acc[s]; p && *p != -1; ++p) {// OLD STYLE
    accept_ids.insert(*p);
  }
}

bool equalDfa(const dfa_tab_t * dfa1, const dfa_tab_t* dfa2) {
  assert(dfa1 && dfa2);
  state_id_t numStates1 = num_states(dfa1);
  state_id_t numStates2 = num_states(dfa2);
  if (numStates1 != numStates2) {
    DMP(numStates1 != numStates2);
    return false;
  }
  
  map<state_id_t, state_id_t> statemap;
  list<state_id_t> wlist;
  statemap[dfa1->start] = dfa2->start;
  wlist.push_back(dfa1->start);
  
  while(!wlist.empty()) {
    state_id_t s1 = wlist.front(); wlist.pop_front();
    assert(CONTAINS(statemap, s1));
    state_id_t s2 = statemap[s1];
    if (dfa1->is_accepting(s1) && dfa2->is_accepting(s2)) {
      set<int> accept_id1;
      dfa1->gather_accept_ids(s1, accept_id1);
      set<int> accept_id2;
      dfa2->gather_accept_ids(s2, accept_id2);
      if (accept_id1 != accept_id2) {
	cout << " Distinct accept_id! s1: " << s1 << " vs s2: " << s2 << endl;
	dumpContainer(accept_id1, "accept_id1");
	dumpContainer(accept_id2, "accept_id2");
	return false;// Actually, I should ad an extra parameter 'bool isMoore", and here if (isMoore) return false
      }
    } else if (dfa1->is_accepting(s1) || dfa2->is_accepting(s2)) {
      DMP(dfa1->is_accepting(s1) || dfa2->is_accepting(s2));
      return false;
    }

    for (unsigned int c = 0; c < MAX_SYMS; ++c) {
      state_id_t d1 = transition(s1, dfa1, c);
      if (CONTAINS(statemap, d1)) {
	if (statemap[d1] != transition(s2, dfa2, c)) {
	  DMP(statemap[d1] != transition(s2, dfa2, c));
	  return false;
	}
      } else {
	statemap[d1] = transition(s2, dfa2, c);
	wlist.push_back(d1);
      }
    }
  }
  { //More paranoic checks:
    /* The mapping should be 1-to-1.
       There may be alternative checks, where this assumption would not hold,
       maybe the case where the DFAs represent the same language, but dfa1 is
       not minimized. However, I haven't thought about such cases, and for now
       the behavior to check for equivalence between two DFAs with the same
       number of states, possibly in different order.
    */
    map<state_id_t, state_id_t> reverse_statemap;
    for (map<state_id_t, state_id_t>::iterator it = statemap.begin();
	 it != statemap.end();
	 ++it) {
      state_id_t s1 = it->first;
      assert( s1 < numStates1 );
      state_id_t s2 = it->second;
      assert( s1 < numStates2 );
      if (CONTAINS(reverse_statemap, s2)) {
	state_id_t old_s = reverse_statemap[s2];
	cout << " ERROR: reverse_statemap already contains"
	     << " s2= " << s2
	     << " old_s= " << old_s
	     << " , new: s1= " << s1
	     << endl;
	return false;
      } else {
	reverse_statemap[s2] = s1;
      }
    }

    if (statemap.size() != numStates1) {
      cout << " ERROR: not all states of dfa1 were visited! ";
      DMP(statemap.size()); DMP(numStates1) << endl;
      return false;
    }
    
    if (reverse_statemap.size() != numStates2) {
      cout << " ERROR: not all states of dfa2 were visited! ";
      DMP(reverse_statemap.size()); DMP(numStates2) << endl;
      return false;
    }
    
    cout << "DBG: equalDfa checked numStates: " << numStates1 << endl;
  }
  return true;
}

int num_accept_ids(int *acc) {
  int num = 0;
  if (acc) {
    for (int *ptr = acc;  (*ptr) != -1;  ++ptr) {// OLD STYLE
      assert(*ptr > 0);
      num++;
    }
  }
  return num;
}

int* clone_accept_ids(int* acc) {
  int *res = NULL;
  int num = num_accept_ids(acc);
  if (num) {
    res = new int[num + 1];
    int i;
    for (i = 0; i < num; ++i) {// OLD STYLE
      res[i] = acc[i];
    }
    assert(acc[i] == -1); // OLD STYLE
    res[i] = acc[i]; // == -1 OLD STYLE
  }
  return res;
}

void guarded_minimize(dfa_tab_t * &dt, state_id_t &lastMinNumberOfStates) {
  if (minimMDFA)
    return; //Theorem: combining minimized and disjoint Moore DFAs results in a minimized mdfa.
  //cout << "guarded_minimize " << dt->num_states << " " << lastMinNumberOfStates << endl;
  if (dt->num_states > lastMinNumberOfStates) {
    unsigned int old = dt->num_states;
    dt->minimize();
    lastMinNumberOfStates = dt->num_states;
    std::cout << "[min:"<<old<<":"<<lastMinNumberOfStates<<"]";
  }
}
