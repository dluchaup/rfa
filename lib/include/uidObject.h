/*-----------------------------------------------------------------------------
 * File:    uidObject.h
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
#ifndef UIDOBJECT_H
#define UIDOBJECT_H

/********** uidObject ***************************/
struct uidObject {
public:
  struct cmpUidObject {// for determimnistic debugging
    bool operator() (uidObject* o1, uidObject* o2) const {
      assert(o1 == o2 || o1->uid != o2->uid);
      return o1->uid < o2->uid;
    }
  };

public:
  static unsigned int g_uido;
  unsigned int uid;
  int id;
  uidObject(): uid(++g_uido) {id = -1;}
};
#endif // UIDOBJECT_H
