/*-----------------------------------------------------------------------------
 * File:    mm_h
 *
 * Author:  Daniel Luchaup
 * Date:    July 2014
 *
 *
 *    Copyright 2008-2014 Daniel Lucahup luchaup@cs.wisc.edu
 *
 *    This file contains unpublished confidential proprietary
 *    work of Daniel Luchaup, Department of Computer Sciences,
 *    University of Wisconsin--Madison.  No use of any sort, including
 *    execution, modification, copying, storage, distribution, or reverse
 *    engineering is permitted without the express written consent of
 *    Daniel Luchaup.
 *
 *---------------------------------------------------------------------------*/

#include <string>
#include <vector>
#include "dfa_tab_t.h"
#include "dfa_dominators.h"
#include <iostream>
#include "dbg.h"

#define MAX_PACKET_SIZE 1500

#define DUMP(x) cout << #x << "= " << (x) << " "

extern bool filter_matches;
extern bool only_matching_packets;

class StatInfo {
  public:
  //unsigned long long total_scanned_length;
  unsigned long long total_scanned_time;
  // unsigned long long min_scanned_time;
  unsigned long long total_memory_steps;
  unsigned long long total_memory_accesses;
  unsigned long long pks_scanned;
  StatInfo() {stat_reset();}
  void stat_reset() {
    //total_scanned_length = 0;
    total_scanned_time = 0;
    total_memory_steps = 0;
    total_memory_accesses = 0;
    pks_scanned = 0;
  }  
};

class StatsPerSize {
  public:
  StatInfo info[MAX_PACKET_SIZE];
  void pkt_count(int size) {
    info[size].pks_scanned += 1;
  }
  void time_count(unsigned long long time, int size) {
    info[size].total_scanned_time += time;
    /*
      if (info[size].min_scanned_time == 0
      || time < info[size].min_scanned_time)
      info[size].min_scanned_time = time;
    */
  }
  void memory_count(int steps, int accesses, int size) {
    info[size].total_memory_steps    += steps;
    info[size].total_memory_accesses += accesses;
  }
  unsigned long long print_statistics(const std::string &msg);
};

//Matching Machine
class MM {
  public:
  // Allow filtering / controlling which packets to consider
  // if:
  //     - packet_size == 0, then all packets allowed
  //     - packet_size > 0, then only packets of len == packet_size allowed
  //     - packet_size = 0, then only packets of len == packet_size allowed
  
  static unsigned int packet_size;
  static unsigned int packet_size_low;
  static unsigned int packet_size_high;
  
  static class MMStandard *g_Std;
  static bool do_size_stats;

  static bool safety_check;
  static int do_simd_statistics;
  unsigned int dbg_safety_errors;
  static unsigned int dbg_total_safety_errors;
  
  unsigned long long pks_scanned;
  unsigned long long pks_ignored;
  bool should_scan_packet(const unsigned char *buf, unsigned int len);
  
   // These are the same as in dfa_tab_t
  protected:
   state_id_t num_states;
   state_id_t start;
   state_id_t *tab;
   int ** acc;
   
  public:
   state_id_t numStates() const { return num_states;}
   state_id_t startState() const { return start;}
   StatsPerSize *size_stats;
   
   state_id_t dbg_history[MAX_PACKET_SIZE];
   bool dbg_safety_check(state_id_t *history,
			 const unsigned char *buf, unsigned int len);
   
   // Statistics gathering
   unsigned long long total_scanned_length; // total scanned length
   unsigned long long total_scanned_time;   // total scanning time
   unsigned long long total_matches;
   
   /* To understand the difference between memory_steps and memory_accesses,
   ** consider a SIMD machine with 4 CPUs, each executing the SIMD instruction
   ** "Mem[Addr]=...;" in one SIMD step.
   ** This represents 1 memory_step and 4 memory_accesses (1/CPU).
   */
   unsigned long long int total_memory_steps;
   unsigned long long int total_memory_accesses;
   unsigned long long int total_spu_usage; // for MMHWChunk
   int current_length;
   inline void time_count(unsigned long long time) {
     total_scanned_time += time;
     if (size_stats)
       size_stats->time_count(time, current_length);
   }
   inline void memory_count(int steps, int accesses) {
     total_memory_steps    += steps;
     total_memory_accesses += accesses;
     total_spu_usage += processors();
     if (size_stats)
       size_stats->memory_count(steps, accesses, current_length);
   }
   inline void count_length(int len) { total_scanned_length += current_length = len;
     if (size_stats)
       size_stats->pkt_count(len);
   }
   
  public:
   MM() {
     size_stats = NULL;
     num_states = 0;
     start = 0;
     tab = NULL;
     acc = NULL;
     if (do_size_stats)
       size_stats = new StatsPerSize;
   }
   MM(dfa_tab_t *dfa) //: dbg_dt(dfa)
     {
     size_stats = NULL;
     init(dfa);
     if (do_size_stats)
       size_stats = new StatsPerSize;
   }
   void clear() { if (size_stats) delete size_stats;}
   virtual ~MM() { clear(); }
   void init(dfa_tab_t *dfa, bool reset = true);
   void reset_stats();
   virtual bool match(const unsigned char *buf, unsigned int len) = 0;
   bool std_hist_match(const unsigned char *buf, unsigned int len);
   virtual void print_statistics(const std::string &msg);
   virtual std::string name() const = 0;
   
   unsigned int virtual processors() const { return 1; } // processor count
   virtual unsigned long long spu_usage() const {
     std::cout << name();
     assert(total_spu_usage == total_memory_steps * processors()); // All except MMHWChunk
     return total_memory_steps * processors();
   }

   int open_payload_trace(const std::string& payload_file_name);
   unsigned long long  scan_payload_trace(int payload_fd);
   unsigned long long  scan_payload_trace(const std::string& payload_file_name);
   bool equivalent(const dfa_tab_t *other);
  public:
   bool matched; // cache result of last match 
};

/*---------------------------------------------------------------------------
** Standard DFA 
*---------------------------------------------------------------------------*/
class MMStandard : public MM {
  public:
  MMStandard(dfa_tab_t *dfa) : MM(dfa) {}
  public:
  bool match(const unsigned char *buf, unsigned int len);
  std::string name() const {return "MMStandard";}
};

/*---------------------------------------------------------------------------
** Gather Statistics for Standard DFA 
*---------------------------------------------------------------------------*/
class MMStdStat : public MM {
  unsigned long long  *freq_state;      // freq_state[s] = s' state frequency
  
  friend struct HelperCompare;
  friend bool stdStatcmp0(state_id_t s1, state_id_t s2);
  public:
  enum SortingKind {
    sk_freq = 0,
    sk_freq_accept_last = 1,
    sk_NUM
  };
  std::vector<int> idom;
  
  inline unsigned long long occurence(state_id_t s) {return freq_state[s];}
  ~MMStdStat();
  MMStdStat(dfa_tab_t *dfa);
  public:
  virtual void print_statistics(const std::string &msg);
  bool match(const unsigned char *buf, unsigned int len);
  std::string name() const {return "MMStdStat:states";}

  void get_sorted(int payload_fd,
		  struct HelperCompare &cmp,
		  // 'sdt' returns the sorted version of this DFA
		  dfa_tab_t &sdt,
		  // *ret_sdt_freq returns the state occurence for sdt
		  unsigned long long **ret_sdt_freq=NULL);

  bool _compare(state_id_t s1, state_id_t s2,
		MMStdStat::SortingKind kind) const {
    if (s1 == start) return true;
    if (s2 == start) return false;

    if (kind == sk_freq_accept_last) {
#define ACCEPTING(s) (acc[s] != NULL)      
      if (ACCEPTING(s1) != ACCEPTING(s2))
	return (!ACCEPTING(s1)); //place s1 first
    }

#define USE_DOMINATORS 1
#if USE_DOMINATORS == 1
    if (state_dominates(s1, s2, idom)) {
      {
	if (!(freq_state[s1] >= freq_state[s2])) {
	  std::cout << "\n s1>s2:===================================\n";
	  DMP(s1); DMP(freq_state[s1]);
	  DMP(s2); DMP(freq_state[s2]);
	  DMPNL(num_states);
	  std::cout << "\n===================================\n";
	  fflush(0);
	}
      }
      assert (freq_state[s1] >= freq_state[s2]);
      return true;
    }
    if (state_dominates(s2, s1, idom)) {
      {
	if (!(freq_state[s2] >= freq_state[s1])) {
	  std::cout << "\n s2>s1 ===================================\n";
	  DMP(s1); DMP(freq_state[s1]);
	  DMP(s2); DMP(freq_state[s2]);
	  DMPNL(num_states);
	  std::cout << "\n===================================\n";
	  fflush(0);
	}
      }
      assert (freq_state[s2] >= freq_state[s1]);
      return false;
    }
#endif
    
    return (freq_state[s1] > freq_state[s2]);
  }
};

/*---------------------------------------------------------------------------
** Sort helpers
*---------------------------------------------------------------------------*/
struct HelperCompare {
  /*
    This is to be used in std::sort.
    Some facts learned the hard way :-/
    Don't make virtual functions, and derived classes from HelperCompare
    std::sort seems to copy, and mess with its parameter, using its
    syntactic type. As a result, when I tried such inheritence:
    I had virtual destructors being called instead of the () operators.
    On other attempt, the virtual () operator was never called; instead
    the () operator of the base class (which was the type of some pointer
    passed around) was called instead.      
  */
  protected:
  MMStdStat::SortingKind kind;
  const MMStdStat *pmm;
  public:
  
  void init(const MMStdStat *pmm0) {
    pmm = pmm0;
  }
  void set_kind(MMStdStat::SortingKind kind0) {kind = kind0;}
  MMStdStat::SortingKind get_kind() const {return kind;}
  void clear() {
    pmm = NULL;
  }
  HelperCompare(MMStdStat::SortingKind kind0,
		const MMStdStat *pmm = NULL) : kind(kind0) {init(pmm);}
  HelperCompare() {clear();}

  inline bool operator() (state_id_t s1, state_id_t s2) { // does s1 go before s2?
    bool res = pmm->_compare(s1, s2, kind);
    //std::cout << " COMPARED: " << s1 << " " << s2 << " " << res << std::endl;   
    return res;
  }
};

/*---------------------------------------------------------------------------
** Used for botching: Gather Statistics, sort, etc.
*---------------------------------------------------------------------------*/
class MMBotcher : public MM {
  dfa_tab_t sdt; //store the sorted/processed but still precise DFA
  std::string original_dfa_filename;
  unsigned long long *sdt_freq; // sdt_freq[s] = #occurences of state s in sdt
  unsigned long long *pkt_freq; // pkt_freq[s] = #times s was largest state/pkt
  unsigned long long *dbg_na_pkt_freq;
  public:
  MMBotcher(const std::string &filename);
  ~MMBotcher();
  std::string name() const {return "MMBotcher of "+original_dfa_filename;}
  dfa_tab_t* botch(double falsePositives);
  dfa_tab_t* botch(state_id_t forcedStates, double &expected_fp);
  bool match(const unsigned char *buf, unsigned int len);
  static std::string BDFA_name(const std::string &original_dfa_filename,
			       double falsePositives);
};
