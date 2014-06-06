/*-----------------------------------------------------------------------------
 * File:    utils.h
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
/* Support functions
 *
 *---------------------------------------------------------------------------*/
#ifndef UTILS_H
#define  UTILS_H

#include <algorithm>
#include <string>

#define CONTAINS(C,e) ((C).find(e) != (C).end())
#define SEQCONTAINS(C, e) (find(((C).begin()), ((C).end()), (e)) != ((C).end()))

extern void chomp(std::string &str);
extern void chomp(char *str);


#endif
