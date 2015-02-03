/*-----------------------------------------------------------------------------
 * File:    mm.c
 *
 * Author:  Daniel Luchaup
 * Date:    21 October 2008
 *
 *
 *    Copyright 2008 Daniel Lucahup luchaup@cs.wisc.edu
 *
 *    This file contains unpublished confidential proprietary
 *    work of Daniel Luchaup, Department of Computer Sciences,
 *    University of Wisconsin--Madison.  No use of any sort, including
 *    execution, modification, copying, storage, distribution, or reverse
 *    engineering is permitted without the express written consent of
 *    Daniel Luchaup.
 *
 *---------------------------------------------------------------------------*/

#include <assert.h>
#include <iostream>
#include <fstream>
#include <bitset>
#include "mm.h"
#include "globals.h"
#include <algorithm>
#include <stack>
#include "misc.h"
#include "dout.h"

#include "MyTime.h"
#include <fcntl.h>
#include "zoptions.h"
#include "dbg.h"

#ifndef PARANOIA
#define PARANOIA 0
#endif

#define LARGE_BOTCHED_STATE 1000000000

using namespace std;

///////////////////////////////////////////////////////////////////////////////
typedef unsigned long long TimerType;

#define IF_ONLY_A_SINGLE_MATCH(X) X // X - if we only care about any match
                                  //   - (nothing) if we want all matches

#ifndef LAPTOP //ifdef XEON
#define MM_START_TIME_CODE TimerType __mm__starttm, __mm__endtm; \
  __mm__starttm = new_rdtsc(); //rdtsc(__mm__starttm);
#define MM_END_TIME_CODE __mm__endtm = new_rdtsc(); /* rdtsc(__mm__endtm); */ \
  time_count(__mm__endtm - __mm__starttm);
#else
#define MM_START_TIME_CODE unsigned long long __mm__starttm, __mm__endtm; \
  rdtsc(__mm__starttm);
#define MM_END_TIME_CODE rdtsc(__mm__endtm);		\
  time_count(__mm__endtm - __mm__starttm);
#endif

#ifdef TIME
#define MM_START_TIME MM_START_TIME_CODE
#define MM_END_TIME MM_END_TIME_CODE
#define MEMORY_COUNT(steps, accesses) // nothing: try obtaining pure timming
#define COUNT_INACTIVE(inactive)
#else
#define MM_START_TIME
#define MM_END_TIME
#define MEMORY_COUNT(steps, accesses) memory_count((steps), (accesses))
#define COUNT_INACTIVE(inactive) inactive_cycle_machines += (inactive)
#endif
///////////////////////////////////////////////////////////////////////////////

unsigned int MM::packet_size = 0;
unsigned int MM::packet_size_low = 0;
unsigned int MM::packet_size_high = 0;
bool MM::do_size_stats = false;

bool MM::safety_check = false;
int MM::do_simd_statistics = 0;
unsigned int MM::dbg_total_safety_errors = 0;


MMStandard* MM::g_Std = NULL;

static MMStdStat::SortingKind get_sort_kind(MMStdStat::SortingKind default_sk);


unsigned long long StatsPerSize::print_statistics(const std::string &msg) {
  unsigned long long total = 0;
  for (int s = 0; s < MAX_PACKET_SIZE; ++s) {
    if (info[s].pks_scanned) {
      total += info[s].pks_scanned;
      cout << msg << " pkt_size= " << s << "\t";
#define sDUMP(x) cout << #x << "= " << (info[s].x) << " "
      sDUMP(pks_scanned);
#ifdef TIME
      sDUMP(total_scanned_time);
      //sDUMP(min_scanned_time);
#else
      //sDUMP(total_scanned_length);
      sDUMP(total_memory_steps);
      sDUMP(total_memory_accesses);
#endif
      cout << endl;
    }
  }
  return total;
}

bool filter_matches = false;
bool only_matching_packets = false;

bool MM::std_hist_match(const unsigned char *buf, unsigned int len) {
  bool found_match = false;
  state_id_t cur = this->start;
  for (unsigned int i=0; i < len; i++) {
    cur = ARR((this->tab), cur, buf[i]);
    dbg_history[i] = cur;
    if (this->acc[cur])
      found_match = true;
  }
  return found_match;
}

bool MM::should_scan_packet(const unsigned char *buf, unsigned int len) {
  if (this->num_states == 0)
    return false;
  
  bool should_scan = true;
  should_scan = should_scan && (packet_size == 0 || packet_size == len);
  should_scan = should_scan && (packet_size_low == 0 || packet_size_low <= len);
  should_scan = should_scan && (   packet_size_high == 0
				|| packet_size_high >= len);

  if (filter_matches) {
    should_scan = should_scan && //?
      (only_matching_packets == std_hist_match(buf, len));
  }
  
  if (should_scan)
    pks_scanned++;
  else
    pks_ignored++;
  
  return should_scan;
}

bool MM::dbg_safety_check(state_id_t *history,
			  const unsigned char *buf, unsigned int len) {
  (void) std_hist_match(buf, len);
  for (unsigned int i = 0; i < len; ++i) {
    if (dbg_history[i] != history[i]) {
      IF_ONLY_A_SINGLE_MATCH(if (history[i] == illegal_state) continue;)
      ++dbg_safety_errors; ++dbg_total_safety_errors;
      cout << "ERROR for " << name() << " at packet " << pks_scanned << " len= " << len << " pos: " << i << " ... " << dbg_history[i] << " != " << history[i] << endl;
      //assert(0);
    }
  }
  return true;
}

void MM::reset_stats() {
  total_scanned_length = 0;
  total_scanned_time = 0;
  total_memory_steps = 0;
  total_memory_accesses = 0;
  total_matches = 0; 
  pks_scanned = 0;
  pks_ignored = 0;
  total_spu_usage = 0;
  dbg_safety_errors = 0;
  if (size_stats) {
    delete size_stats;
    size_stats = NULL;
  }
  assert ((num_states == (((num_states)<<8) >> 8)) && "needed by ARR macro");
}

void MM::init(dfa_tab_t *dfa, bool reset) {
  //assert(dfa);
  if (dfa) {
    num_states = dfa->num_states;
    start = dfa->start;
    tab = dfa->tab;
    acc = dfa->acc;
  }
  if (reset) {
    reset_stats();
  }
  //assert(!reset || (num_states > 0 && tab != NULL && acc != NULL));
  assert ((num_states == (((num_states)<<8) >> 8)) && "needed by ARR macro");
}

void MM::print_statistics(const std::string &msg) {
  cout << msg << "\t\t";
  DUMP(pks_scanned);
  DUMP(pks_ignored);
  DUMP(total_scanned_time);
  DUMP(total_scanned_length);
  DUMP(total_memory_steps);
  DUMP(total_memory_accesses);
  DUMP(total_matches);
  
  cout << endl;
  
  if (size_stats) {
    unsigned long long total = size_stats->print_statistics(name());
    if (total != pks_scanned)
      cout << "Possible error/mismatch " << total << " vs " <<  pks_scanned
	   << " Have you used the -L list ? " << endl;
    //assert(total == pks_scanned);
  }
}

///////////////////////////////////////////////////////////////////////////////
int read_payload_packet(int payload_fd, unsigned char* buf, int &size) {
  int res;
  res = read(payload_fd,  &size, sizeof(size));
  assert(res >= 0);
  if (res != 0) {
    if (!(res == sizeof(size) && 0 < size && size <= MAX_PACKET_SIZE))
      printf("res=%d, size=%d\n", res, size);
    assert(res == sizeof(size) && 0 < size && size <= MAX_PACKET_SIZE);
    res = read(payload_fd,  buf, size);
    assert(res == size);
  }
  return res;
}

unsigned long long  MM::scan_payload_trace(int payload_fd) {
  MyTime tm("scan_payload_trace");
  int size = 0;
  unsigned char buf[MAX_PACKET_SIZE];
  unsigned long long packets_extracted = 0;
  
  assert(payload_fd != 0);//not really needed, maybe at some point 0 will be OK
  assert(lseek(payload_fd, 0, SEEK_SET) == 0);
  
  if (MM::do_size_stats && !this->size_stats) {
    this->size_stats = new StatsPerSize;
  }
  
  this->dbg_safety_errors = 0;
  while(read_payload_packet(payload_fd, buf, size)) {
    packets_extracted++;
    this->match(buf, size);
  }
  
  if (MM::safety_check) {
    if (this->dbg_safety_errors) {
      cout << "CHECK of " << this->name()
	   << ((this->dbg_safety_errors == 0)? " PASSED": " FAILED") << endl;
    }
  }
  //else if(list_index == -1) this->print_statistics(this->name());
  return packets_extracted;
}

unsigned long long  compare_scan_payload_trace(int payload_fd,
					       MM& m1,
					       MM& m2,
					       //unsigned long long in1,
					       //unsigned long long in2,
					       unsigned long long &in1no2,
					       unsigned long long &in2no1) {
  m1.reset_stats();
  m2.reset_stats();
  int size = 0;
  unsigned char buf[MAX_PACKET_SIZE];
  unsigned long long packets_extracted = 0;
  
  assert(payload_fd != 0);//not really needed, maybe at some point 0 will be OK
  assert(lseek(payload_fd, 0, SEEK_SET) == 0);
  
  //in1 = 0;
  //in2 = 0;
  in1no2 = 0;
  in2no1 = 0;
  
  while(read_payload_packet(payload_fd, buf, size)) {
    packets_extracted++;
    bool a1 = m1.match(buf, size);
    bool a2 = m2.match(buf, size);

    //if (a1) ++ in1;
    if (a1 && !a2) ++ in1no2;
    
    //if (a2) ++ in2;
    if (a2 && !a1) ++ in2no1;
  }

  assert(m1.total_matches - in1no2 == m2.total_matches - in2no1);
  
  return packets_extracted;
}

int MM::open_payload_trace(const string& payload_file_name) {
  int payload_fd = open(payload_file_name.c_str(), O_RDONLY);
  if (payload_fd == -1) {
    perror("Cannot open file ");
    zop.usage_and_die((name() + " machine was specified a bad  file:"
		       + payload_file_name).c_str());
  }
  return payload_fd;
}

unsigned long long  MM::scan_payload_trace(const string& payload_file_name) {
  if (payload_file_name.empty())
    zop.usage_and_die(" empty file name passed to machine " + name());

  int payload_fd = open_payload_trace(payload_file_name);
  unsigned long long res = scan_payload_trace(payload_fd);
  close(payload_fd); // keep valgrind happy
  return res;
}

bool MM::equivalent(const dfa_tab_t *other) {
  dfa_tab_t dt;
  
  dt.num_states = num_states;
  dt.start = start;
  dt.tab = tab;
  dt.acc = acc;
  
  bool res = equalDfa(&dt, other);

  // avoid dfa_tab_t'a destructor taking over...
  dt.num_states = 0;
  dt.start = 0;
  dt.tab = NULL;
  dt.acc = NULL;

  return res;
}

/*---------------------------------------------------------------------------
** Standard DFA 
*---------------------------------------------------------------------------*/
bool MMStandard::match(const unsigned char *buf, unsigned int len)
{
  matched = false;
  if (!should_scan_packet(buf, len))
    return false;
  
  count_length(len);
  MM_START_TIME;
  bool found_match = false;
  state_id_t cur = this->start;
  for (unsigned int i=0; i < len; i++) {
    MEMORY_COUNT(1, 1);
    cur = ARR((this->tab), cur, buf[i]);
    if (this->acc[cur])
      {
	found_match = true;
	IF_ONLY_A_SINGLE_MATCH(break);
      }
  }
  MM_END_TIME;
  if (found_match) total_matches ++;
  return matched = found_match;
}

/*---------------------------------------------------------------------------
** Gather Statistics for Standard DFA 
*---------------------------------------------------------------------------*/
MMStdStat::~MMStdStat()  {
  assert(freq_state != NULL);
  if (freq_state)
    delete []freq_state;
}

MMStdStat::MMStdStat(dfa_tab_t *dfa) : MM(dfa) {
  freq_state = new unsigned long long [num_states];
  for (state_id_t s = 0; s < num_states; ++s)
    freq_state[s] = 0;
  
#if USE_DOMINATORS == 1
  idom.clear();
  get_dominators(dfa, idom);
#endif
}

#define PER_PACKET_STATS 1 // quick hack to experiment with freq_state/packet
#if PER_PACKET_STATS == 0
bool MMStdStat::match(const unsigned char *buf, unsigned int len)
{
  matched = false;
  if (!should_scan_packet(buf, len))
    return false;

  count_length(len);
  MM_START_TIME;
  bool found_match = false;
  state_id_t cur = this->start;
  for (unsigned int i=0; i < len; i++) {
    MEMORY_COUNT(1, 1);
    cur = ARR((this->tab), cur, buf[i]);
    this->freq_state[cur]++;
        
    if (this->acc[cur])
      {
	found_match = true;
	IF_ONLY_A_SINGLE_MATCH(break);
      }
  }
  MM_END_TIME;
  if (found_match) total_matches ++;
  return matched = found_match;
}
#else
bool MMStdStat::match(const unsigned char *buf, unsigned int len)
{
#define MAXBIT 200000
  static vector<bool> stateseen(MAXBIT, false);
  matched = false;
  //if (!should_scan_packet(buf, len))   return false;
  
  state_id_t history1[MAX_PACKET_SIZE]; unsigned h1 = 0;
  
  count_length(len);
  MM_START_TIME;
  bool found_match = false;
  state_id_t cur = this->start;
  for (unsigned int i=0; i < len; i++) {
    MEMORY_COUNT(1, 1);
    cur = ARR((this->tab), cur, buf[i]);
    if (!stateseen[cur]) {
      stateseen[cur]=true;
      history1[h1++] = cur;
    }
    if (this->acc[cur])
      {
	found_match = true;
	IF_ONLY_A_SINGLE_MATCH(break);
      }
  }
  //DMPNL("END");
  MM_END_TIME;
  if (found_match) {total_matches ++;}
  for (unsigned k = 0; k < h1; ++k) {
    assert(stateseen[history1[k]]);
    stateseen[history1[k]] = false;
    assert(!stateseen[history1[k]]);
    if (!found_match)
      this->freq_state[history1[k]]++;
  }
  
  return matched = found_match;
}
#endif


void MMStdStat::print_statistics(const std::string &/*msg*/) {
  if (zop.dbgBoolOption("StdStatSilent"))
    return;
  unsigned long long total_occurence = 0;
  long double total_freq = 0;
  //long double probability_no_match = 1;
  long double probability_match = 0;
  state_id_t s;

  //this->MM::print_statistics(msg);
  
  long double dbg_total_freq = 0;
  //long double dbg_probability_no_match = 1;
  long double dbg_freq = ((long double)1)/num_states;

  for (s = 0; s < num_states; ++s) {
    total_occurence += this->freq_state[s];
  }
  //cout << total_occurence << " " << total_memory_accesses  << " " << total_occurence == total_memory_accesses << endl;
  assert(total_occurence == total_memory_accesses);
  unsigned long long so_far_occurence = 0;
  for (s = 0; s < num_states; ++s) {
    so_far_occurence += this->freq_state[s];
    long double freq = ((long double)this->freq_state[s])/total_memory_accesses;
    total_freq += freq;
    
    //probability_no_match = probability_no_match * (1-freq*freq) * (1-freq*freq);
    probability_match += freq*freq;
    
    cout // << s << " "
      << this->freq_state[s] << " "
      << freq << " " << (((long double)so_far_occurence)/total_occurence)  << " "
      //<< " (? " << total_freq - (((long double)so_far_occurence)/total_occurence) << " )"
      << (1 - (((long double)so_far_occurence)/total_occurence)) << " "
      << s << endl;
    
    
    dbg_total_freq += dbg_freq;
    //dbg_probability_no_match = dbg_probability_no_match * (1-dbg_freq*dbg_freq)*(1-dbg_frerq*dbg_freq);
  }
  long double expected_validation = 1.0/probability_match;
#if 1
  DUMP(total_freq);
  //DUMP(probability_no_match);
  //DUMP(1-probability_no_match);
  DUMP(probability_match);
  DUMP(1-probability_match);
  DUMP(expected_validation);
  cout << endl;
  DUMP(total_occurence);
  DUMP(total_memory_accesses);
  DUMP(total_occurence == total_memory_accesses);
  cout << endl;
  DUMP(dbg_freq);
  DUMP(dbg_total_freq);
  //DUMP(dbg_probability_no_match);
  //DUMP(1-dbg_probability_no_match);
  cout << endl;
#endif
}

void dbg_check_freq_accept_last(const dfa_tab_t &sdt,
				const state_id_t *order_from_new_to_old,
				const unsigned long long *sdt_freq,
				unsigned long long total_memory_accesses,
				struct HelperCompare &cmp);
void MMStdStat::get_sorted(int payload_fd,
			   struct HelperCompare &cmp,
			   // 'sdt' returns the sorted version of this DFA
			   //       allocates sdt.tab and sdt.acc
			   dfa_tab_t &sdt,
			   unsigned long long ** ret_sdt_freq)
{
  MyTime tm_sort("MMStdStat::get_sorted()");

  //bool old_filter_matches = filter_matches;
  //filter_matches = true;// For freq_state[*] based ordering, skipping matching packets is different than placing accepting at the  end.
  DMP(filter_matches);
  unsigned long long  train_packets = scan_payload_trace(payload_fd); tm_sort.lap("payload-scanned");
  //filter_matches = old_filter_matches;
  
  DMP(train_packets); DMP(total_matches) << " seen in training dataset." << endl;
  
  state_id_t *order_from_old_to_new = new state_id_t[num_states];
  state_id_t *order_from_new_to_old = new state_id_t[num_states];

  cmp.init(this);
  for (state_id_t s = 0; s < num_states; ++s)
    order_from_new_to_old[s] = s;
  {
    MyTime tm("cost-freq-sort");
    std::sort(order_from_new_to_old, order_from_new_to_old + num_states, cmp);
    assert(order_from_new_to_old[0] == this->start);
    
    if (PARANOIA > 80) {
      MyTime tm("cost-verify-compare-order");
      for (state_id_t /*new_s*/s = 1; s < num_states; ++s) {
	bool ok_order = !cmp(order_from_new_to_old[s], order_from_new_to_old[s-1]);
	if(!ok_order)
	  cout << "BAD! " << s << endl;
	assert(ok_order);
      }
    }
  }
  
  // order_from_new_to_old : NEW_sorted_order -->INITIAL_order
  // order_from_old_to_new  : INITIAL_order --> NEW_sorted_order
  for (state_id_t new_s = 0; new_s < num_states; ++new_s) {
    order_from_old_to_new[order_from_new_to_old[new_s]] = new_s;
  }
  
  // 'sdt' will hold the sorted version of this DFA
  sdt.num_states = num_states;
  sdt.tab = new unsigned int [MAX_SYMS * num_states];
  sdt.acc = new int* [num_states];
  
  unsigned long long *sdt_freq = new unsigned long long [num_states];
  // Re-order the states in sorted order
  {
    MyTime tm("cost-of-reordering");
    unsigned long long dbg_occurences = 0;
    for (state_id_t old_s = 0; old_s < num_states; ++old_s) {
      for (unsigned int j=0; j < MAX_SYMS; j++)	{
	ARR(sdt.tab, order_from_old_to_new[old_s], j) = order_from_old_to_new[ARR(tab, old_s, j)];
      }
      sdt.acc[order_from_old_to_new[old_s]] = clone_accept_ids(this->acc[old_s]);
      sdt_freq[order_from_old_to_new[old_s]] = freq_state[old_s];
      
      dbg_occurences += freq_state[old_s];
    }
    DMP(PER_PACKET_STATS); DMP(dbg_occurences); DMP(total_memory_accesses) << endl;
    assert(PER_PACKET_STATS || (dbg_occurences == total_memory_accesses));
  }
  sdt.start = order_from_old_to_new[start];
  assert(sdt.start == 0);
  
  if (PARANOIA > 50) { //PARANOIA
    {
      MyTime tm("cost-verify-sorted-dt-is-equivalent-with-this-one: equalDfa(sorted, original)");
      assert(equivalent(&sdt));
    }
    if (cmp.get_kind() == sk_freq_accept_last) {
      dbg_check_freq_accept_last(sdt, order_from_new_to_old, sdt_freq, total_memory_accesses, cmp);
    }
  }
  
  // Assuming that this is sorted, then no minimization is needed
  if (ret_sdt_freq) {
    *ret_sdt_freq = sdt_freq;
  } else {
    delete [] sdt_freq; sdt_freq = NULL;
  }
  delete []order_from_old_to_new;
  delete []order_from_new_to_old;
}

/*---------------------------------------------------------------------------
** Used for botching: Gather Statistics, sort, etc.
*---------------------------------------------------------------------------*/
MMBotcher::MMBotcher(const std::string &filename)
  : original_dfa_filename(filename), sdt_freq(NULL), pkt_freq(NULL)
{
  MyTime tm("MMBotcher::MMBotcher("+filename+")");
  int payload_fd = open_payload_trace(zop.dbgStringOption("PAYLOAD"));
  dfa_tab_t dt;
  dt.txt_dfa_load(filename);
  tm.lap(filename + " loaded-unsorted ");
  
  MMStdStat mm(&dt);
  
  //TBD: better to count each seen state per packet, not per byte.
  struct HelperCompare cmp(get_sort_kind(MMStdStat::sk_freq_accept_last));
  
  mm.get_sorted(payload_fd, cmp, sdt, NULL);
  
  tm.lap(filename + " just got sorted ");
  
  DMP(sdt.num_states); DMP(mm.numStates()); DMP(sdt.start) << endl;
  assert(sdt.num_states == mm.numStates() && sdt.start == 0);
  
  init(&sdt);
  close(payload_fd);
}

MMBotcher::~MMBotcher() {
  if (sdt_freq)
    delete []sdt_freq;
  if (pkt_freq)
    delete []pkt_freq;
  sdt.cleanup();//!!!
}

// Should only be used for figuring out max state frequencies
bool MMBotcher::match(const unsigned char *buf, unsigned int len)
{
  matched = false;
  if (!should_scan_packet(buf, len))
    return false;

  count_length(len);
  bool found_match = false;
  state_id_t cur = this->start;
  state_id_t max_seen_state = 0;
  for (unsigned int i=0; i < len; i++) {
    MEMORY_COUNT(1, 1);
    cur = ARR((this->tab), cur, buf[i]);
    if (cur > max_seen_state)
      max_seen_state = cur;
    if (this->acc[cur]) {
	found_match = true;
	break;
    }
  }
  //++pkt_freq[max_seen_state];
  if (found_match) total_matches ++;
  else {
    //!!!!!!!!!! YYEES!!
    pkt_freq[max_seen_state]++;
    if (dbg_na_pkt_freq) dbg_na_pkt_freq[max_seen_state]++;
  }
  return matched = found_match;
}

dfa_tab_t* MMBotcher::botch(double falsePositives) {
  MyTime tm("MMBotcher::botch " + original_dfa_filename);
  
  pkt_freq = new unsigned long long [num_states]; assert(pkt_freq);
  bzero(pkt_freq, num_states*sizeof(*pkt_freq));
  
  dbg_na_pkt_freq = new unsigned long long [num_states]; assert(dbg_na_pkt_freq);
  bzero(dbg_na_pkt_freq, num_states*sizeof(*dbg_na_pkt_freq));
  
  // Fill pkt_freq: pkt_freq[s] = number of times s was largest seen state/pkt
  //      !!! ... but still working on which input set: Training set I, or I- Accepting
  unsigned long long all_packets = scan_payload_trace(zop.dbgStringOption("PAYLOAD"));
  tm.lap("payload-scanned");
  unsigned long long all_non_match_packets = 0;
  for (state_id_t s = 0; s < num_states; ++s) {
    all_non_match_packets += pkt_freq[s]; //OBSOLETE: Aah! This also counts accepting pks. OK?
  }
  DMP(all_packets); DMP(total_matches); DMP(all_packets - total_matches); DMP(all_non_match_packets) << " seen in training set." << endl;
  assert(all_non_match_packets == (all_packets - total_matches));
  
  unsigned long long max_allowed_bad_packets = all_non_match_packets * falsePositives;//rounding here...!
  unsigned long long min_required_good_packets = all_non_match_packets - max_allowed_bad_packets;//...so this is safe
  
  cout << "\n DBG: TARGET: fp0:"
       << ((all_non_match_packets - (double)min_required_good_packets)/all_non_match_packets)
       << endl;
  
  unsigned long long good_packets = 0;
  unsigned long long dbg_good_1_less = 0;  // what if we allowed 1 less state
  state_id_t botched_num_states = 0;
  state_id_t merged_final_state = illegal_state;
  for (botched_num_states = 1; botched_num_states <= num_states; ++botched_num_states) {
    state_id_t max_botch_state = botched_num_states-1;
    good_packets += pkt_freq[max_botch_state];// Nope! ISSUE: accepting packets.
    
    if (this->acc[max_botch_state])
      merged_final_state = max_botch_state;
    
    if (good_packets >= min_required_good_packets) {
      break;
    }
    dbg_good_1_less = good_packets;
  }
/*
VERY interesting: (from ~/mbyte/mdfa -a dbg-botch -zMDFA2BOTCH:mdfa_2_from_1000 -zPAYLOAD:/scratch/traces/from_randy/all-but0  -zfalsePositives:0.005 2>&1 | tee log.puzzling.fp005
......
DBG: TARGET: fp0:0.00499988
pkt_freq[botched_num_states -2]= 12236 acc[botched_num_states-2]= 0 pkt_freq[botched_num_states -1]= 2063 acc[botched_num_states-1]= 0 pkt_freq[botched_num_states]= 3 acc[botched_num_states]= 0 
dbg_good_1_less= 1937084 dbg_good_1_more= 1939150 


DBG: BEST ACHIEVABLE(?!):  #states 12 [%]good-rate: 99.5636 fp-rate: 0.00436424 1-state-less-fp-rate: 0.00542347 1-state-more-fp-rate: 0.0043627!  Expected-false-matches: 8500!!!
all_non_match_packets= 1947647 min_required_good_packets= 1937909 good_packets= 1939147
...

HOW LUCKY TO HIT/USE a falsePositives THAT TRIGGERED THIS EXAMPLE:
 - Look at what difference 1 more state makes!
So, what happens?
-  pkt_freq is NOT ordered (good to remind this)
-  pkt_freq may contain 'spikes', such as this at pkt_freq[botched_num_states-(1 or 2)]
-  OK, so there is a jump in pkt_freq[X],
   I bet that X corresponds to states close to a '*' (or some back edge target)
   in some signature ;-)
   
   Does it mean that despide the requested 'falsePositives', it may be worth it
   to add a few more states to get a dramatic jump in falsePositives?
   -- pros: "much" better falsePositives
   -- may be the risk of triggering the (.*s1.*s2) | (.*s3|.*s4)
TBD: investigate this!
*/
  unsigned long long dbg_good_1_more = good_packets + // what if we allowed 1 more state
    ((botched_num_states < num_states)? pkt_freq[botched_num_states] : 0);
  if (botched_num_states < num_states) {
    if (botched_num_states >= 2)
      { DMP(pkt_freq[botched_num_states -2]); DMP(acc[botched_num_states-2]);}
    DMP(pkt_freq[botched_num_states -1]); DMP(acc[botched_num_states-1]);
    DMP(pkt_freq[botched_num_states]); DMP(acc[botched_num_states]) << endl;
  }
  DMP(dbg_good_1_less); DMP(dbg_good_1_more);
  cout << "\n\n\nDBG: BEST ACHIEVABLE(?!): "
       << " #states " << botched_num_states
       << " [%]good-rate: " << ((100.0*good_packets)/all_non_match_packets)
       << " fp-rate: " << ((all_non_match_packets - (double)good_packets)/all_non_match_packets)
       << " 1-state-less-fp-rate: " << ((all_non_match_packets - (double)dbg_good_1_less)/all_non_match_packets)
       << " 1-state-more-fp-rate: " << ((all_non_match_packets - (double)dbg_good_1_more)/all_non_match_packets)
       << "! "
       << " Expected-false-matches: " << (all_non_match_packets - good_packets)
       << "!!!"
       << endl;
  DMP(all_non_match_packets); DMP(min_required_good_packets);DMP(good_packets);
  cout << "\n---------------\n";
  
  //assert(botched_num_states <= num_states && "Unattainable approximation");
  
  if (merged_final_state == illegal_state)
    assert(botched_num_states < num_states);
  
  //////////////////////////////////////////////////////////////////////
  
  if (merged_final_state == illegal_state) {
    assert(botched_num_states < num_states);
    merged_final_state = botched_num_states;
    botched_num_states++;
  }

  //////////////////////////////////////////////////////////////////////
  // build the BOTCHed dfa
  //////////////////////////////////////////////////////////////////////
  dfa_tab_t* pbdt = new dfa_tab_t(); // the botched version of this DFA
  dfa_tab_t &bdt = *pbdt;
  bdt.num_states = botched_num_states;
  bdt.start = this->start; assert(bdt.start == 0);
  bdt.tab = new unsigned int [MAX_SYMS * bdt.num_states];
  bdt.acc = new int* [bdt.num_states];
  string BDFA_file = BDFA_name(original_dfa_filename, falsePositives);
  bdt.filename = BDFA_file;
  
  // Re-order the states in sorted order
  { MyTime tm("cost-of-setting-up-the-botched-table");// Seems Cheap
    for (state_id_t s = 0; s < bdt.num_states; ++s) {
      for (unsigned int j=0; j < MAX_SYMS; j++) {
	state_id_t next = ARR(this->tab, s, j);
	if (next >= bdt.num_states)
	  // TBD: Could it speed up the minimization if we arrange transitions to accepting states by hand?
	  next = merged_final_state;
	ARR(bdt.tab, s, j) = next;
      }
      bdt.acc[s] = NULL;
      if (this->acc[s] || s == merged_final_state) {
	bdt.acc[s] = new int[2]; bdt.acc[s][0] = LARGE_BOTCHED_STATE; bdt.acc[s][1] = -1;//OLD STYLE
	for (unsigned int j=0; j < MAX_SYMS; j++)
	  ARR(bdt.tab, s, j) = merged_final_state;
      }
    }
  }
  
  {
    if (PARANOIA > 80) { // PARANOIA - part 1
      MyTime tm("cost-paranoic check on in-memory, not-minim bdt for "+BDFA_file);//EXPENSIVE? 
      dfa_tab_t dt1;
      dt1.txt_dfa_load(original_dfa_filename);
      tm.lap(" loaded original:"+original_dfa_filename);
      MMStdStat mm1(&dt1);
      
      dfa_tab_t dt2;
      MMStdStat mm2(&bdt);
      
      unsigned long long in1no2 = 0;
      unsigned long long in2no1 = 0;
      unsigned long long packets_extracted;
      {
	int payload_fd = open_payload_trace(zop.dbgStringOption("PAYLOAD"));
	MyTime tm("cost-compare_scan_payload_trace "+ original_dfa_filename);
	packets_extracted = compare_scan_payload_trace(payload_fd, mm1, mm2, in1no2, in2no1);
	close(payload_fd);
      }
      
      cout << " (1)DBG MMBotcher::botch " << original_dfa_filename
	   << " original-states " << mm1.numStates()
	   << " botched-states " << mm2.numStates()
	   << " intermediate-botched_num_states " << botched_num_states
	   << " matches-original= " << mm1.total_matches
	   << " i.e. " << (100.0 * mm1.total_matches)/packets_extracted << " %;"
	   << " matches-botch= " << mm2.total_matches
	   << " i.e. " << (100.0 * mm2.total_matches)/packets_extracted << " %;"
	   << " matches-only-original= " << in1no2
	   << " (false-positives)matches-only-botched= " << in2no1
	   << " i.e. " << (100.0 * in2no1)/packets_extracted << " %;"
	   << " overapproximation?=" << (in1no2 == 0)
	   << " fp-rate-ok?= " << (((double)in1no2)/packets_extracted < falsePositives)
	   << " falsePositives= " << falsePositives
	   << " VALID-PREDICTION?" << ((all_non_match_packets - good_packets) == in2no1)
	   << endl;
      assert(((all_non_match_packets - good_packets) == in2no1) && "VALID-PREDICTION?");
    }
    
    { // Minimization
      MyTime tm("cost-minimization for "+BDFA_file);//EXPENSIVE sometimes: for num.states over 100k
      cout << " Minimizing bdt ..." << endl;
      bdt.minimize();
      tm.lap("minimization done");
      cout << " Minimized bdt has " << bdt.num_states << " num_states" << endl;
      bdt.txt_dfa_dump((BDFA_file).c_str()); // Still needed for caching!
    }
    
    if (PARANOIA > 80) { // PARANOIA - part 2
      MyTime tm("cost-paranoic check for "+BDFA_file);//EXPENSIVE sometimes: for num.states over 100k
      dfa_tab_t dt1;
      dt1.txt_dfa_load(original_dfa_filename);
      tm.lap(" loaded original:"+original_dfa_filename);
      MMStdStat mm1(&dt1);
      
      dfa_tab_t dt2;
      dt2.txt_dfa_load(BDFA_file);
      tm.lap(" loaded botched ");
      assert(bdt.dbg_same(dt2));
      tm.lap(" cost-verify-load-what-dumped");
      
      
      MMStdStat mm2(&dt2);
      
      unsigned long long in1no2 = 0;
      unsigned long long in2no1 = 0;
      unsigned long long packets_extracted;
      {
	int payload_fd = open_payload_trace(zop.dbgStringOption("PAYLOAD"));
	MyTime tm("cost-compare_scan_payload_trace "+ original_dfa_filename);
	packets_extracted = compare_scan_payload_trace(payload_fd, mm1, mm2, in1no2, in2no1);
	close(payload_fd); // keep valgrind happy
      }
      
      cout << " (2)DBG MMBotcher::botch " << original_dfa_filename
	   << " original-states " << mm1.numStates()
	   << " botched-states " << mm2.numStates()
	   << " intermediate-botched_num_states " << botched_num_states
	   << " matches-original= " << mm1.total_matches
	   << " i.e. " << (100.0 * mm1.total_matches)/packets_extracted << " %;"
	   << " matches-botch= " << mm2.total_matches
	   << " i.e. " << (100.0 * mm2.total_matches)/packets_extracted << " %;"
	   << " matches-only-original= " << in1no2
	   << " (false-positives)matches-only-botched= " << in2no1
	   << " i.e. " << (100.0 * in2no1)/packets_extracted << " %;"
	   << " overapproximation?=" << (in1no2 == 0)
	   << " fp-rate-ok?= " << (((double)in1no2)/packets_extracted < falsePositives)
	   << " falsePositives= " << falsePositives
	   << " VALID-PREDICTION?" << ((all_non_match_packets - good_packets) == in2no1)
	   << endl;
      assert(((all_non_match_packets - good_packets) == in2no1) && "VALID-PREDICTION?");
    }
  }

  double result_falsePositives = ((all_non_match_packets - (double)good_packets)/all_non_match_packets);
  {
    Dout ostats("stats.botch", std::ios::app);
    ostats << ENDL
	   << " STATS: Botcher::botch of-file " << original_dfa_filename
	   << " to-file " << BDFA_file
	   << " reduced num_states= " << num_states
	   << " to " << pbdt->num_states
	   << " (intermediate-botched_num_states " << botched_num_states << ")"
	   << " fp-rate: " << result_falsePositives
	   << " fp-requested: " << falsePositives
	   << " how-close:%: " << (100*falsePositives-result_falsePositives)/falsePositives
	   << " Expected-false-matches: " << (all_non_match_packets - good_packets)
	   << ENDL;
  }

  if (pkt_freq) delete [] pkt_freq; pkt_freq = NULL;
  if (dbg_na_pkt_freq) delete [] dbg_na_pkt_freq; dbg_na_pkt_freq = NULL;
  return pbdt;
}

dfa_tab_t* MMBotcher::botch(state_id_t forcedStates, double &expected_fp) {// HACK: CUT AND PASTE FROM ABOVE!
  double falsePositives = 1;
  MyTime tm("MMBotcher::botch " + original_dfa_filename);
  DMP(forcedStates);
  
  pkt_freq = new unsigned long long [num_states]; assert(pkt_freq);
  bzero(pkt_freq, num_states*sizeof(*pkt_freq));
  
  dbg_na_pkt_freq = new unsigned long long [num_states]; assert(dbg_na_pkt_freq);
  bzero(dbg_na_pkt_freq, num_states*sizeof(*dbg_na_pkt_freq));
  
  // Fill pkt_freq: pkt_freq[s] = number of times s was largest seen state/pkt
  //      !!! ... but still working on which input set: Training set I, or I- Accepting
  unsigned long long all_packets = scan_payload_trace(zop.dbgStringOption("PAYLOAD"));
  tm.lap("payload-scanned");
  unsigned long long all_non_match_packets = 0;
  for (state_id_t s = 0; s < num_states; ++s) {
    all_non_match_packets += pkt_freq[s]; //OBSOLETE: Aah! This also counts accepting pks. OK?
  }
  DMP(all_packets); DMP(total_matches); DMP(all_packets - total_matches); DMP(all_non_match_packets) << " seen in training set." << endl;
  assert(all_non_match_packets == (all_packets - total_matches));
  
  unsigned long long good_packets = 0;
  unsigned long long dbg_good_1_less = 0;  // what if we allowed 1 less state
  state_id_t botched_num_states = 0;
  state_id_t merged_final_state = illegal_state;
  for (botched_num_states = 1; botched_num_states <= num_states; ++botched_num_states) {
    state_id_t max_botch_state = botched_num_states-1;
    good_packets += pkt_freq[max_botch_state];// Nope! ISSUE: accepting packets.
    
    if (this->acc[max_botch_state])
      merged_final_state = max_botch_state;
    
    if (botched_num_states > forcedStates)
      break;
    dbg_good_1_less = good_packets;
  }
  unsigned long long dbg_good_1_more = good_packets + // what if we allowed 1 more state
    ((botched_num_states < num_states)? pkt_freq[botched_num_states] : 0);
  if (botched_num_states < num_states) {
    if (botched_num_states >= 2)
      { DMP(pkt_freq[botched_num_states -2]); DMP(acc[botched_num_states-2]);}
    DMP(pkt_freq[botched_num_states -1]); DMP(acc[botched_num_states-1]);
    DMP(pkt_freq[botched_num_states]); DMP(acc[botched_num_states]) << endl;
  }
  DMP(dbg_good_1_less); DMP(dbg_good_1_more);
  expected_fp = ((all_non_match_packets - (double)good_packets)/all_non_match_packets);
  cout << "\n\n\nDBG: BEST ACHIEVABLE(?!): "
       << " #states " << botched_num_states
       << " [%]good-rate: " << ((100.0*good_packets)/all_non_match_packets)
       << " fp-rate: " << ((all_non_match_packets - (double)good_packets)/all_non_match_packets)
       << " 1-state-less-fp-rate: " << ((all_non_match_packets - (double)dbg_good_1_less)/all_non_match_packets)
       << " 1-state-more-fp-rate: " << ((all_non_match_packets - (double)dbg_good_1_more)/all_non_match_packets)
       << "! "
       << " Expected-false-matches: " << (all_non_match_packets - good_packets)
       << "!!!"
       << endl;
  DMP(all_non_match_packets); DMP(good_packets);
  cout << "\n---------------\n";
  
  //assert(botched_num_states <= num_states && "Unattainable approximation");
  
  if (merged_final_state == illegal_state)
    assert(botched_num_states < num_states);
  
  //////////////////////////////////////////////////////////////////////
  
  if (merged_final_state == illegal_state) {
    assert(botched_num_states < num_states);
    merged_final_state = botched_num_states;
    botched_num_states++;
  }

  //////////////////////////////////////////////////////////////////////
  // build the BOTCHed dfa
  //////////////////////////////////////////////////////////////////////
  dfa_tab_t* pbdt = new dfa_tab_t(); // the botched version of this DFA
  dfa_tab_t &bdt = *pbdt;
  bdt.num_states = botched_num_states;
  bdt.start = this->start; assert(bdt.start == 0);
  bdt.tab = new unsigned int [MAX_SYMS * bdt.num_states];
  bdt.acc = new int* [bdt.num_states];
  string BDFA_file = BDFA_name(original_dfa_filename, falsePositives);
  bdt.filename = BDFA_file;
  
  // Re-order the states in sorted order
  { MyTime tm("cost-of-setting-up-the-botched-table");// Seems Cheap
    for (state_id_t s = 0; s < bdt.num_states; ++s) {
      for (unsigned int j=0; j < MAX_SYMS; j++) {
	state_id_t next = ARR(this->tab, s, j);
	if (next >= bdt.num_states)
	  // TBD: Could it speed up the minimization if we arrange transitions to accepting states by hand?
	  next = merged_final_state;
	ARR(bdt.tab, s, j) = next;
      }
      bdt.acc[s] = NULL;
      if (this->acc[s] || s == merged_final_state) {
	bdt.acc[s] = new int[2]; bdt.acc[s][0] = LARGE_BOTCHED_STATE; bdt.acc[s][1] = -1;//OLD STYLE
	for (unsigned int j=0; j < MAX_SYMS; j++)
	  ARR(bdt.tab, s, j) = merged_final_state;
      }
    }
  }
  
  {
    if (PARANOIA > 80) { // PARANOIA - part 1
      MyTime tm("cost-paranoic check on in-memory, not-minim bdt for "+BDFA_file);//EXPENSIVE? 
      dfa_tab_t dt1;
      dt1.txt_dfa_load(original_dfa_filename);
      tm.lap(" loaded original:"+original_dfa_filename);
      MMStdStat mm1(&dt1);
      
      dfa_tab_t dt2;
      MMStdStat mm2(&bdt);
      
      unsigned long long in1no2 = 0;
      unsigned long long in2no1 = 0;
      unsigned long long packets_extracted;
      {
	int payload_fd = open_payload_trace(zop.dbgStringOption("PAYLOAD"));
	MyTime tm("cost-compare_scan_payload_trace "+ original_dfa_filename);
	packets_extracted = compare_scan_payload_trace(payload_fd, mm1, mm2, in1no2, in2no1);
	close(payload_fd);
      }
      
      cout << " (1)DBG MMBotcher::botch " << original_dfa_filename
	   << " original-states " << mm1.numStates()
	   << " botched-states " << mm2.numStates()
	   << " intermediate-botched_num_states " << botched_num_states
	   << " matches-original= " << mm1.total_matches
	   << " i.e. " << (100.0 * mm1.total_matches)/packets_extracted << " %;"
	   << " matches-botch= " << mm2.total_matches
	   << " i.e. " << (100.0 * mm2.total_matches)/packets_extracted << " %;"
	   << " matches-only-original= " << in1no2
	   << " (false-positives)matches-only-botched= " << in2no1
	   << " i.e. " << (100.0 * in2no1)/packets_extracted << " %;"
	   << " overapproximation?=" << (in1no2 == 0)
	   << " fp-rate-ok?= " << (((double)in1no2)/packets_extracted < falsePositives)
	   << " falsePositives= " << falsePositives
	   << " VALID-PREDICTION?" << ((all_non_match_packets - good_packets) == in2no1)
	   << endl;
      assert(((all_non_match_packets - good_packets) == in2no1) && "VALID-PREDICTION?");
    }
    
    { // Minimization
      MyTime tm("cost-minimization for "+BDFA_file);//EXPENSIVE sometimes: for num.states over 100k
      cout << " Minimizing bdt ..." << endl;
      bdt.minimize();
      tm.lap("minimization done");
      cout << " Minimized bdt has " << bdt.num_states << " num_states" << endl;
      bdt.txt_dfa_dump((BDFA_file).c_str()); // Still needed for caching!
    }
    
    if (PARANOIA > 80) { // PARANOIA - part 2
      MyTime tm("cost-paranoic check for "+BDFA_file);//EXPENSIVE sometimes: for num.states over 100k
      dfa_tab_t dt1;
      dt1.txt_dfa_load(original_dfa_filename);
      tm.lap(" loaded original:"+original_dfa_filename);
      MMStdStat mm1(&dt1);
      
      dfa_tab_t dt2;
      dt2.txt_dfa_load(BDFA_file);
      tm.lap(" loaded botched ");
      assert(bdt.dbg_same(dt2));
      tm.lap(" cost-verify-load-what-dumped");
      
      
      MMStdStat mm2(&dt2);
      
      unsigned long long in1no2 = 0;
      unsigned long long in2no1 = 0;
      unsigned long long packets_extracted;
      {
	int payload_fd = open_payload_trace(zop.dbgStringOption("PAYLOAD"));
	MyTime tm("cost-compare_scan_payload_trace "+ original_dfa_filename);
	packets_extracted = compare_scan_payload_trace(payload_fd, mm1, mm2, in1no2, in2no1);
	close(payload_fd); // keep valgrind happy
      }
      
      cout << " (2)DBG MMBotcher::botch " << original_dfa_filename
	   << " original-states " << mm1.numStates()
	   << " botched-states " << mm2.numStates()
	   << " intermediate-botched_num_states " << botched_num_states
	   << " matches-original= " << mm1.total_matches
	   << " i.e. " << (100.0 * mm1.total_matches)/packets_extracted << " %;"
	   << " matches-botch= " << mm2.total_matches
	   << " i.e. " << (100.0 * mm2.total_matches)/packets_extracted << " %;"
	   << " matches-only-original= " << in1no2
	   << " (false-positives)matches-only-botched= " << in2no1
	   << " i.e. " << (100.0 * in2no1)/packets_extracted << " %;"
	   << " overapproximation?=" << (in1no2 == 0)
	   << " fp-rate-ok?= " << (((double)in1no2)/packets_extracted < falsePositives)
	   << " falsePositives= " << falsePositives
	   << " VALID-PREDICTION?" << ((all_non_match_packets - good_packets) == in2no1)
	   << endl;
      assert(((all_non_match_packets - good_packets) == in2no1) && "VALID-PREDICTION?");
    }
  }

  cout << " Botcher::botch of " << original_dfa_filename
       << " reduced num_states= " << num_states
       << " to " << pbdt->num_states
       << " (intermediate-botched_num_states " << botched_num_states << ")"
       << endl;

  if (pkt_freq) delete [] pkt_freq; pkt_freq = NULL;
  if (dbg_na_pkt_freq) delete [] dbg_na_pkt_freq; dbg_na_pkt_freq = NULL;
  return pbdt;
}

std::string MMBotcher::BDFA_name(const std::string &original_dfa_filename,
			       double falsePositives) {
  return "BDFA.from." + original_dfa_filename + ".fp." + toString(falsePositives);
}

#define MIN(x,y) ((x<=y)? x:y)

void dbg_check_freq_accept_last(
  const dfa_tab_t &sdt,
  const state_id_t *order_from_new_to_old,
  const unsigned long long *sdt_freq,
  unsigned long long total_memory_accesses,
  struct HelperCompare &cmp)
{
  assert(sdt_freq);
  unsigned long long dbg_occurences = 0;
  bool dbg_seen_accepted_state = false;
  state_id_t non_accepting = 0;
  state_id_t accepting = 0;
  for (state_id_t /*new*/s = 0; s < sdt.num_states; ++s) {
    dbg_occurences += sdt_freq[s];
    if (s > 0)
      assert(!cmp(order_from_new_to_old[s], order_from_new_to_old[s -1]));
    bool transition = (s > 0 && ((sdt.acc[s-1]==0) != (sdt.acc[s]==0)));
    if (!(!transition || !dbg_seen_accepted_state)) {
      assert(s > 1);
      cout << " ERROR/tranition-FAILURE " << s << " " << cmp(s-1, s)
	   << "; freq: " << sdt_freq[s-1] << " <?> " << sdt_freq[s]
	   << "; acc?: " << sdt.acc[s-1] << "," << sdt.acc[s]
	   << endl;
    }
    assert (!transition || !dbg_seen_accepted_state);// Once we see an accepting state, all are accepting.
    if (s > 2 && !transition) { // 0=start state, gets placet 1st anyway
      if ( !(sdt_freq[s-1] >= sdt_freq[s])) {
	cout << " ERROR/FAILURE " << s << " " << cmp(s-1, s) << "; "
	     << sdt_freq[s-1] << " <?> " << sdt_freq[s] << "; acc?: "
	     << sdt.acc[s-1] << "," << sdt.acc[s-1]
	     << endl;
      }
      assert(sdt_freq[s-1] >= sdt_freq[s]); // in order, unless we are witnesing the unique transition from non-accepting to accepting.
    }
    if (sdt.acc[s]) {
      accepting ++;
      if (!dbg_seen_accepted_state)
	cout << " DBG:TRANSITION AT " << s << endl;
      dbg_seen_accepted_state = true;
    } else {
      non_accepting ++;
    }
  }
  assert(dbg_occurences == total_memory_accesses);
  cout << " PARANOIA(dbg_check_freq_accept_last): accepting= " << accepting << " , non-accepting= " << non_accepting << endl;
}

MMStdStat::SortingKind get_sort_kind(MMStdStat::SortingKind default_sk) {
  // Allow the sorting kind to be overwritten from command line. Hackish ...
  MMStdStat::SortingKind res_sk =
    (MMStdStat::SortingKind) zop.dbgNumOption("sk", default_sk);
  if (res_sk != default_sk) {
    if (res_sk >= 0 && res_sk < MMStdStat::sk_NUM) {
      cout << "DBG:: MMBotcher sort kind forced to sk= " << res_sk << endl;
    } else {
      cout << "ERROR: Incorrect sort kind specified:"
	   << zop.dbgStringOption("sk") << endl;
      assert(0);
    }
  }
  return res_sk;
}

