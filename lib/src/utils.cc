/*-----------------------------------------------------------------------------
 * File:    utils.cc
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
#include <stdio.h>
#include <string.h>
#include "utils.h"
#include "dbg.h"


void chomp(std::string &str) {
  if (str[str.length() - 1] == '\n') {
    str.resize(str.length()-1);
  }
}
void chomp(char *str) {
  if (str[strlen(str) - 1] == '\n') {
    str[strlen(str) - 1] = 0;
  }
}

void pin_processors() {
    cpu_set_t mask;
    unsigned int len = sizeof(mask);
    if (sched_getaffinity(0, len, &mask) < 0) {
	perror("sched_getaffinity");
	exit(1);
    }
    //DMP(sizeof(long unsigned int)); DMPNL(sizeof(mask)); // for printf
    //assert(sizeof(long unsigned int) == sizeof(mask)); // for printf
    //printf("my affinity mask is: %08lx\n", (*((long unsigned int*)&mask)));
    CPU_CLR(1, &mask);
    //printf("Trying to change my affinity mask to: %08lx\n", (*((long unsigned int*)&mask)));
    if (sched_setaffinity(0, len, &mask) < 0) {
	perror("sched_setaffinity");
	exit(1);
    }
    if (sched_getaffinity(0, len, &mask) < 0) {
	perror("sched_getaffinity");
	exit(1);
    }
}

///////////////
//Randy Smith's code from escape_sequences.cc
/*-----------------------------------------------------------------------------
 * output_char
 *   converts a character into an output sequence
 * str - string into which printable output goes
 * n   - length of str (including space for \0
 * c   - value to convert
 *---------------------------------------------------------------------------*/
char * output_char(char *str, unsigned int n, unsigned int c,
		   bool inside_char_class)
{
   assert (c <= 255);
   assert ( n >= 4);

   bool icc = inside_char_class;

   const char *s = NULL;
   switch (c)
   {
      /* note that the following items do not
       * need to be escape inside []'s: |(){}[+*?^$.   */

      case '\f': s = "\\f"; break; 
      case '\n': s = "\\n"; break;
      case '\r': s = "\\r"; break; 
      case '\t': s = "\\t"; break;
      case '\v': s = "\\v"; break;
      case '|':  s = (icc ? "|" : "\\|"); break;
      case '(':  s = (icc ? "(" : "\\("); break;
      case ')':  s = (icc ? ")" : "\\)"); break;
      case '{':  s = (icc ? "{" : "\\{"); break;
      case '}':  s = (icc ? "}" : "\\}"); break;
      case '[':  s = (icc ? "[" : "\\["); break;
      case ']':  s = "\\]"; break;
      case '^':  s = (icc ? "^" : "\\^"); break;
      case '$':  s = (icc ? "$" : "\\$"); break;
      case '\\': s = "\\\\"; break; 
      case '+':  s = (icc ? "+" : "\\+"); break; 
      case '*':  s = (icc ? "*" : "\\*"); break;
      case '?':  s = (icc ? "?" : "\\?"); break;
      case '/':  s = (icc ? "/" : "\\/"); break;
      case '\'': s = "\\'"; break;
      case '\"': s = "\\\""; break;
      case '-':  s = (icc ? "-" : "\\-"); break;
      default:
	 break;
   }



   if (s != NULL)
   {
      strcpy(str, s);
   }
   else
   {
      if (c >= 0x20 && c <= 0x7e)
	 sprintf(str, "%c",c);
      else //sprintf(str, "\\x%x", (unsigned char)c;
	sprintf(str, "\\x%02x", (unsigned char)c);
   }

   return str;
}

/************************************************/
std::string output_char2string(unsigned int c) {
  char outstr[10];
  output_char(outstr, 10, c, false);
  std::string su = outstr;
  return su;  
}

std::string output_string2string(const std::string &raw) {
  std::string res("");
  for (size_t i = 0; i < raw.length(); ++i)
    res = res + output_char2string(raw[i]);
  return res;
}
