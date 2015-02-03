/*-----------------------------------------------------------------------------
 * dout.h
 * author:  Daniel Luchaup
 * date:    26 Oct 2011
 *
 * Split output: debugging and 
 * split output functionality, similar to that in boost, but I don't want to
 * add a dependency on it and #include <boost/iostreams/tee.hpp> etc...
 *
 *
 *History
 *---------------------------------------------------------------------------
 * $Log: dout.h,v $
 * Revision 1.1.1.1  2012/04/05 02:09:05  dl
 * local mbyte
 *
 * Revision 1.1  2011/10/26 22:22:49  luchaup
 * dump the bdfa-tree structure to a separate file. Add support for split output: debug to cout and if needed to another file: use dout.h
 *
 *---------------------------------------------------------------------------*/
#ifndef _DOUT_H_
#define _DOUT_H_

#include <iostream>
#include <fstream>
#include <string>
//using namespace std;


class Dout {
  std::ofstream file;
  int level;
  int what;
public:
  Dout() {}
  Dout(const char * filename, std::_Ios_Openmode mode = std::ios::out) {
    file.open(filename, mode);
    if (file.fail()) {
      perror((std::string("Cannot open ")+std::string(filename)).c_str()); exit(1);
    }
  }
  
  ~Dout() {
    close();
  }
  
  template <class T>
  Dout& operator<< (T t) {
    if (level >= what)
      std::cout << (t);
    if (file.is_open())
      file << (t);
    return *this;
  }
  
  Dout& flush() {
    std::cout.flush();
    if (file.is_open())
      file.flush();
    return *this;
  }
  
  void write(const std::string &str) {
    write(str.c_str());
  }
  void write(const char * filename) {
    file.close();
    file.open(filename, std::ios::out);
    if (file.fail()) {
      perror((std::string("Cannot open ")+std::string(filename)).c_str()); exit(1);
    }
  }
  
  void append(const std::string &str) {
    append(str.c_str());
  }
  void append(const char * filename) {
    file.close();
    file.open(filename, std::ios::app);
    if (file.fail()) {
      perror((std::string("Cannot open ")+std::string(filename)).c_str()); exit(1);
    }
  }
  void close() {
    if (file.is_open()) {
      file.flush();
      file.close();
    }
  }
  Dout& operator()(int x) {
    level = x;
    return *this;
  }
};

const char * const ENDL="\n"; // not sure why using std:endl fails compilation

#endif //_DOUT_H_
