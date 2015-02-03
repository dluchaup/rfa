/*-----------------------------------------------------------------------------
 * File:    gram_tests.cc
 * Author:  Daniel Luchaup
 * Date:    December 2013
 * Copyright 2013 Daniel Luchaup luchaup@cs.wisc.edu
 *
 * This file contains unpublished confidential proprietary work of
 * Daniel Luchaup, Department of Computer Sciences, University of
 * Wisconsin--Madison.  No use of any sort, including execution, modification,
 * copying, storage, distribution, or reverse engineering is permitted without
 * the express written consent (for each kind of use) of Daniel Luchaup.
 *
 *---------------------------------------------------------------------------*/
#include <iostream>
#include "gram.h"
#include "MyTime.h"
#include <cstdarg>

// Utility function for easy GParseTree construction. Meant for YACC use.
extern "C"
GRule::GParseTree* mkgpt(GRule *gr, ... )
{
  va_list arguments;
  va_start(arguments, gr);

  GRule::GParseTree *gpt = new GRule::GParseTree(gr);
  ParseTree *pt;
  for(pt = va_arg(arguments,ParseTree*);
      pt != NULL;
      pt = va_arg(arguments,ParseTree*))
    gpt->push_child(pt);
  va_end(arguments);
  return gpt;
}

extern "C"
GRule::GParseTree* mkgptn(GRule *gr, int num, ... )
{
  va_list arguments;
  va_start(arguments, num);

  GRule::GParseTree *gpt = new GRule::GParseTree(gr);
  gpt->children.resize(num);
  for(int c = 0; c < num; ++c)
    gpt->add_child(va_arg(arguments,ParseTree*), c);
  va_end(arguments);
  return gpt;
}

extern "C"
GRule::GParseTree* mkgptr(GRule *gr, ... )
{
  va_list arguments;
  int num = gr->children.size();
  va_start(arguments, gr);

  GRule::GParseTree *gpt = new GRule::GParseTree(gr);
  gpt->children.resize(num);
  for(int c = 0; c < num; ++c)
    gpt->add_child(va_arg(arguments,ParseTree*), c);
  va_end(arguments);
  return gpt;
}
