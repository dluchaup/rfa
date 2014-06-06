/*-----------------------------------------------------------------------------
 * file:    escape_sequences.cc
 * author:  Randy Smith
 * date:    14 July 2006
 * descr:   routines for manipulating and resolving escape sequences
 *
 *
 *
 *    Copyright 2006,2007 Randy Smith, smithr@cs.wisc.edu
 *
 *    This file contains unpublished confidential proprietary
 *    work of Randy Smith, Department of Computer Sciences,
 *    University of Wisconsin--Madison.  No use of any sort, including
 *    execution, modification, copying, storage, distribution, or reverse
 *    engineering is permitted without the express written consent of
 *    Randy Smith.
 *
 * History:
 *-----------------------------------------------------------------------------
 * modified by author: Daniel Luchaup
 *---------------------------------------------------------------------------*/
#include <stdlib.h>
#include <string.h>
#include <string>
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include "escape_sequences.h"
#include "errmsg.h"


extern int errno;
extern int lineno;

/*-----------------------------------------------------------------------------
 * convert_escape_sequences
 *   Given a string containing standard C escape sequences, this will
 *   return the same string, but all escape sequences are replaced with their
 *   actual values.
 *---------------------------------------------------------------------------*/
char *convert_escape_sequences(char *str, int *len)
{
   char *read, *write;
   int actual_read, val;
   
   write = read = str;
   
   while (*read != 0) {
      if (*read == '\\') {
	 val = parse_char_constant(read, &actual_read);
	 if (val < 0) {
	    *write++ = 'X';
	 } else {
	    *write++ = (char) val;
	 }
	 read += actual_read;
      } else {
	 *write++ = *read++;
      }
   }
   *write = 0;
   if (len)
       *len = (write - str);
   return(str);
}

/*-----------------------------------------------------------------------------
 * parse_char_constant
 *   given a char constant string, such as b,c,d,\n,\r, etc., this will
 *   give you the integer ascii value of that string.
 *---------------------------------------------------------------------------*/
int parse_char_constant(const char *s, int *actual_read)
{
   assert(s[0] == '\\');
   *actual_read = 2;
   
   switch (s[1]) {
      case 'f': return '\f';
      case 'n': return '\n';
      case 'r': return '\r';
      case 't': return '\t';
      case 'v': return '\v';
      case '|': return '|';
      case '(': return '(';
      case ')': return ')';
      case '{': return '{';
      case '}': return '}';
      case '[': return '[';
      case ']': return ']';
      case '^': return '^';
      case '$': return '$';
      case '\\': return '\\';
      case '.': return '.'; //dl: I think this was missing ...
      case '+': return '+';
      case '*': return '*';
      case '?': return '\?';
      case '/': return '/';
      case '\'': return '\'';
      case '\"': return '\"';
      case 'x': {
	  int val=0, tmp, i;

	  for (i=2; (tmp = hex_digit(s[i])) >= 0; i++) {
	      val = 16 * val + tmp;
	  }
	  *actual_read = i;
	  if (i==2) {
	      errf(lineno, "expecting hex digits after \\x");
	      return(-1);
	  }
	  if (val > 255) {
	      errf(lineno, 
		   "explicit hex character (\\x) out of range with value %d",
		   val);
	      return(-1);
	  }
	  return(val);
	  break;
      }
      default:
	if (octal_digit(s[1]) >= 0) {  /* octal constant */
	    int val=0, tmp, i;
	    
	    for (i=1; (tmp = octal_digit(s[i])) >= 0  && i<4; i++) {
		val = 8 * val + tmp;
	    }
	    if (val > 255) {
		errf(lineno, "explicit octal character out of range");
		return(-1);
	    }
	    *actual_read = i;
	    return(val);
	} else {
	   /* perl warns that the special character is unknown, but
	    * it treats it as though it were just a plain (unescaped)
	    * character. So, we'll do the same */
	    warnf(lineno, "unknown special character \"\\%c\".  "
		  "Interpreting as \"%c\"", s[1], s[1]);
	    return (s[1]);
	}
    }
    /* unreachable */
}

/*-----------------------------------------------------------------------------
 * hex_digit
 *   returns the numeric value of the corresponding hex digit
 *---------------------------------------------------------------------------*/
int hex_digit(char in)
{
    if (in >= '0' && in <= '9') return(in - '0');
    if (in >= 'a' && in <= 'f') return(in - 'a' + 10);
    if (in >= 'A' && in <= 'F') return(in - 'A' + 10);
    return -1;
}

/*-----------------------------------------------------------------------------
 * octal_digit
 *  returns the value of the corresponding octal digit.
 *---------------------------------------------------------------------------*/
int octal_digit(char in)
{
    if (in >= '0' && in <= '7') return(in - '0');
    return -1;
}
