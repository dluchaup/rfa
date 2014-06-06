/*-----------------------------------------------------------------------------
 * File:    MemoryInfo.h
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
#ifndef MEMORYINFO_H
#define MEMORYINFO_H

#include <assert.h>
#include <string>
#include <sys/sysinfo.h>
#include <stdio.h>

struct MemoryInfo {
// Information about the memory currently being used by this process, in bytes.
// Grab the info directly from the /proc pseudo-filesystem.  Reading from
// /proc/self/statm gives info on your own process, as one line of numbers.
// The mem sizes should all be multiplied by the page size.
    unsigned long size;      //total program size (same as VmSize in /proc/[pid]/status)
    unsigned long resident;  //resident set size  (same as VmRSS in /proc/[pid]/status)
    unsigned long share;     //shared pages (from shared mappings)
    unsigned long text;      //text (code)
    unsigned long lib;       //library (unused in Linux 2.6)
    unsigned long data;      //data + stack
    unsigned long dt;        //dirty pages (unused in Linux 2.6)
    MemoryInfo() : size(0), resident(0),share(0),text(0),lib(0),data(0),dt(0) {}
    unsigned long read() {
      FILE *file = fopen("/proc/self/statm", "r");
      if (file) {
#define READ(VAL) if (1 != fscanf (file, "%lu", &(VAL))) assert(0); \
	      (VAL) = (size_t)(VAL) * getpagesize(); //printf(#VAL "=%ld",(VAL));
	    
        READ(size);
	READ(resident);
	READ(share);
	READ(text);
	READ(lib);
	READ(data);
	READ(dt);
	fclose (file);
      }
      return size;
    }
    void dump(const char* msg) {
      printf("MemoryInfo:%s ", (msg?msg:""));
#define MIDUMP(VAL)  printf(#VAL "= %ld ; ", (VAL));
      MIDUMP(size);
      MIDUMP(resident);
      MIDUMP(share);
      MIDUMP(text);
      MIDUMP(lib);
      MIDUMP(data);
      MIDUMP(dt);
#undef MIDUMP
      fflush(NULL);
    }
    void dump(const std::string &str) {dump(str.c_str());}
    void read_and_dump(const char* msg) {read(); dump(msg);}
    void read_and_dump(const std::string &str) {read_and_dump(str.c_str());}
};

#endif
