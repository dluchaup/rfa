/*-----------------------------------------------------------------------------
 * File:    zoptions.h
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
#ifndef ZOPTIONS_H
#define ZOPTIONS_H

#include <set>
#include <iostream>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <fcntl.h>

typedef struct zOptions {
  std::set<std::string> dbgOptionSet;
  const char*usage;
  
  zOptions(const char* usage0="") {usage = usage0;}


  bool key_matches(const char* key, const std::string& candidate) {
    return ( (candidate.compare(0,strlen(key), key) == 0)
	    && ( (strlen(key) == candidate.size())
		|| candidate[strlen(key)] == ':' || candidate[strlen(key)] == '=')
      );
  }
  
  bool dbgBoolOption(const char* opt) {
    return (dbgOptionSet.find(std::string(opt)) != dbgOptionSet.end());
  }
  bool dbgDefinedOption(const char* key) {
    for (std::set<std::string>::const_iterator it = dbgOptionSet.begin();
	 it != dbgOptionSet.end();
	 ++it)
      if (key_matches(key, (*it)))
	return true;
    return false;
  }
  long dbgNumOption(const char* opt, long defaultVal) {
    for (std::set<std::string>::const_iterator it = dbgOptionSet.begin();
	 it != dbgOptionSet.end();
	 ++it) {
      if (key_matches(opt, (*it))) {
	std::string sval = (*it).substr(1+strlen(opt), (*it).length());
	long val = strtol(sval.c_str(), NULL, 10);
	return val;
      }
    }
    return defaultVal;
  }
  double dbgDoubleOption(const char* key, double defaultVal) {
    for (std::set<std::string>::const_iterator it = dbgOptionSet.begin();
	 it != dbgOptionSet.end();
	 ++it) {
      if (key_matches(key, (*it))) {
	std::string sval = (*it).substr(1+strlen(key), (*it).length());
	double val = strtod(sval.c_str(), NULL);
	return val;
      }
    }
    return defaultVal;
  }
  std::string dbgStringOption(const char* key) {
    for (std::set<std::string>::const_iterator it = dbgOptionSet.begin();
	 it != dbgOptionSet.end();
	 ++it) {
      if (key_matches(key, (*it))) {
	std::string val = (*it).substr(1+strlen(key), (*it).length());
	return val;
      }
    }
    return std::string();
  }
  void dumpOptions() {
    std::cout << "The -z options are:........." << std::endl;
    for (std::set<std::string>::const_iterator it = dbgOptionSet.begin();
	 it != dbgOptionSet.end();
	 ++it) {
      std::cout << *it << std::endl;
    }
    std::cout << "........." << std::endl;
  }


  FILE* optFILE(const char* optionName, const char* how="r") {
    std::string fileName = dbgStringOption(optionName);
    if (fileName.empty())
      usage_and_die((std::string(optionName)+
		     " and its argument were not specified with -z").c_str());
    
    FILE *f = fopen(fileName.c_str(), how);
    if (f == NULL) {
      perror("Cannot open file ");
      usage_and_die(((std::string(optionName)+
		      " specified a bad  file:" + fileName)).c_str());
    }
    return f;
  }
  
  int optFileDescriptor(const char* optionName) {
    std::string fileName = dbgStringOption(optionName);
    if (fileName.empty())
      usage_and_die((std::string(optionName)+
		     " and its argument were not specified with -z").c_str());
    
    int fd;
    if (-1 == (fd = open(fileName.c_str(), O_RDONLY))) {
      perror("Cannot open file");
      usage_and_die(("Bad (trace?) file:" + std::string(fileName)).c_str());
    }
    return fd;
  }

  void usage_and_die(const char *message,...)
    {
      va_list arglist;
      va_start(arglist, message);
      vfprintf(stdout, message, arglist);
      fprintf(stdout, "\n");
      std::cout << usage << std::endl;
      va_end(arglist);
      exit(1);
    }
  void usage_and_die(const std::string& msg) {
    std::cout << msg << std::endl << usage << std::endl;
    exit(1);
  }

} zOptions;

extern zOptions zop; // for now, to be defined in the file containing main
#endif

