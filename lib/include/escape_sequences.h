/*-----------------------------------------------------------------------------
 * file:    escape_sequences.h
 * author:  Randy Smith
 * date:    13 July 2006
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

#ifndef ESCAPE_SEQUENCES_H
#define ESCAPE_SEQUENCES_H
#include <string>

char *convert_escape_sequences(char *string, int *len=NULL);
int parse_char_constant(const char *s, int *actual_read);
int hex_digit(char in);
int octal_digit(char in);
char * output_char(char *str, unsigned int n, unsigned int c,
		   bool inside_char_class = false);
std::string output_char2string(unsigned int c);
std::string output_string2string(const std::string &raw);

#endif
