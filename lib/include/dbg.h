/*-----------------------------------------------------------------------------
 * File:    dbg.h
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
#ifndef _DBG_H
#define _DBG_H

#include <sstream>
#include <iostream>
#include <algorithm>
#include <string>

/*****************************************************************************/
/* The DMP macros are supposed to be always on                               */
/*     dbgDBP macros should be turned on(debugging) or off (production)      */
/*****************************************************************************/
#define DMP(x) std::cout << #x << "= " << (x) << " "
#define DMPNL(x) std::cout << #x << "= " << (x) << "\n"
#define DMPC(x) std::cout << #x << "= " << (dumpContainer(x,""), ".") << " "

#ifdef VERBOSE_DMP
#define dbgDMP(x) DMP(x)
#define dbgDMPNL(x) DMPNL(x)
#define dbgDMPC(x) DMPC(x)
#else
#define dbgDMP(x)
#define dbgDMPNL(x)
#define dbgDMPC(x)
#endif

#define Wish(cond) ((cond)? (cout << "") : (cout << endl <<\
                    __FILE__ <<":" << __LINE__ << \
					    " Failed Wish: " << #cond << endl))


template <class Container> std::string toString(const Container c) {
  std::stringstream out;
  out << c ;
  return out.str();
}

template <class Container> void dumpContainer(const Container c,
					      const std::string &msg ) {
  std::cout << msg << "...:[";
  for (typename Container::const_iterator it = c.begin(); it != c.end(); ++it){
    std::cout << *it << ", ";
  }
  std::cout << "]" << std::endl;
}

template <class T>
void tdump(const T& o, const std::string & msg = "") {
  std::cout << msg << o << " ";
}

//TBD: get rid of one of the dumpContainer's
template <class Conteiner>
void dbgDump(const Conteiner & c, std::string message, bool newline=true) {
  std::cout << "Container: [" << c.size() << "] " << message << " : ";
  for(typename Conteiner::const_iterator it = c.begin(); it != c.end(); ++it) {
    tdump(*it);
  }
  if (newline) std::cout << std::endl;
}


#endif
