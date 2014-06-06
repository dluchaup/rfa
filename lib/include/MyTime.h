/*-----------------------------------------------------------------------------
 * File:    MyTime.h
 * Author:  Daniel Luchaup
 * Date:    26 November 2013
 * Copyright 2009-2013 Daniel Luchaup luchaup@cs.wisc.edu
 *
 * This file contains unpublished confidential proprietary work of
 * Daniel Luchaup, Department of Computer Sciences, University of
 * Wisconsin--Madison.  No use of any sort, including execution, modification,
 * copying, storage, distribution, or reverse engineering is permitted without
 * the express written consent (for each kind of use) of Daniel Luchaup.
 *
 *---------------------------------------------------------------------------*/
#ifndef MYTIME_H
#define MYTIME_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <vector>
#include <iostream>
#include <sstream>
#include <assert.h>
#include <stdint.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <errno.h>


typedef unsigned long long TimerType;
extern "C" {
   inline TimerType new_rdtsc() {
       uint32_t lo, hi;
       /* cpuid will serialize the following rdtsc with respect to all other
        * instructions the processor may be handling.
        */
       __asm__ __volatile__ (
         "xorl %%eax, %%eax\n"
         "cpuid\n"
         "rdtsc\n"
         : "=a" (lo), "=d" (hi)
         :
         : "%ebx", "%ecx");
     return (uint64_t)hi << 32 | lo;
   }
}

#define CPUHZ 2261054000 //OK, this holds on 'floreasca'. Cross your fingers!
#define TimerType2Seconds(t) (((double)(t))/CPUHZ)

class MyTime {
  std::string name;
  TimerType total;
  TimerType last_start;
  TimerType last_end;
  TimerType last_lap;
public:
  MyTime(std::string name0, bool start=true):
    name(name0), total(0), last_start(0), last_end(0), last_lap(0) {
    //chomp(name);
    if (start)
      resume();
  }
  MyTime(int n, std::string name0): total(0), last_start(0), last_end(0) {
    //chomp(name0);
    char buf[32];
    snprintf(buf,32,"%d",n);
    name = name0+"."+buf;
    resume();
  }
  void reset() {total = last_start = last_end = 0;}
  void resume() {last_start = new_rdtsc();}
  void stop() {
    last_end = new_rdtsc();
    last_lap = 0;
    if (last_start)
      last_lap = (last_end - last_start);
    total += last_lap;
    last_start = last_end = 0;
  }
  void lap(const char *msg=NULL) {
    stop();
    if (msg) print(std::string("[lap]")+msg);
    resume();
    last_lap = 0;
  }
  void lap(const std::string &msg) {
    lap(msg.c_str());
  }
  void print(const std::string& msg) {
    if (last_lap)
      std::cout << "Timer: " << ((double)last_lap)/CPUHZ << " of " << ((double)total)/CPUHZ << " s? " << name << " # " << msg << " "  << total << " : " << ((double)total)/CPUHZ << " s?" << std::endl;
    else
      std::cout << "Timer: " << ((double)total)/CPUHZ << " s? " << name << " # " << msg << " "  << total << " : " << ((double)total)/CPUHZ << " s?" << std::endl;
    //DMP(last_start); DMP(last_end); DMP(last_end - last_start) << std::endl;
  }
  ~MyTime() {
    last_end = new_rdtsc();
    if (last_start) {
      total += (last_end - last_start);
    }
    print("@done@");
  }
  TimerType get_total() const { return total; }
};

class VoidTime {
  std::string name;
  TimerType total;
  TimerType last_start;
  TimerType last_end;
  TimerType last_lap;
public:
  VoidTime(std::string name0, bool =true):
    name(name0), total(0), last_start(0), last_end(0), last_lap(0) {}
  VoidTime(int , std::string ): total(0), last_start(0), last_end(0) {}
  void reset() {}
  void resume() {}
  void stop() {}
  void lap(const char* =NULL) {}
  void lap(const std::string &) {}
  void print(const std::string& ) {}
  ~VoidTime() {}
  TimerType get_total() const { return total; }
};

#ifdef VERBOSE_TIME
#define DbgTime MyTime
#else
#define DbgTime VoidTime
#endif

#endif
