/*-----------------------------------------------------------------------------
 *nodes.h
 * Implements the nodes used for building the pcre parse tree.
 *
 * Author:  Randy Smith
 * Date:    10 August 2006
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
 *-----------------------------------------------------------------------------
 * History:
 * modified by author: Daniel Luchaup
 *---------------------------------------------------------------------------*/
#ifndef NODES_H
#define NODES_H

#include <list>
#include <string>
#include <bitset>


extern int lineno;

using namespace std;

/* predeclare all classes */
class base_tree;
class disjunction;
class conjunction;
class term;
class assertion;
class quantifier;
class atom;
class char_class;
class range;
class modifiers;

class trex;
class trex_disjunction;
typedef disjunction regex_tree_t;

class base_tree
{
  protected:
  public:
  unsigned int uid;
  static unsigned int g_uid;
      base_tree(void);    
      void virtual unparse(FILE *out, int indent=0) = 0;
      void virtual printtree(FILE *out, int indent=0) = 0;
      virtual ~base_tree(void) = 0;

      virtual base_tree* clone() = 0;
      virtual trex* to_trex() = 0;
      
   private:
      base_tree(base_tree&);
      base_tree& operator=(const base_tree & T);
};



class disjunction : public base_tree
{
   public:
      disjunction();
      ~disjunction();
      void unparse(FILE *out, int indent=0);
      void printtree(FILE *out, int indent=0);
      void add_star(void);

      disjunction * clone();
      trex* to_trex();
      list<conjunction *> alternatives;
};


class conjunction : public base_tree
{
   public:
      conjunction();
      ~conjunction();
      void unparse(FILE *out, int indent=0);
      void printtree(FILE *out, int indent=0);
      list<term *> terms;
      conjunction  * clone();
      trex  * to_trex();
};

/*-----------------------------------------------------------------------------
 * term
 *   Terms are either assertions or atoms.  We create distinct classes
 *   for each type of atom - atomstar, atomplus, atomqm, atomrange 
 *---------------------------------------------------------------------------*/
class term : public base_tree
{
   public:
      term(atom *child, quantifier *q);
      ~term();
      void unparse(FILE *out, int ident=0);
      void printtree(FILE *out, int indent=0);
      
      atom       *a_;
      quantifier *quant_;
      
      term * clone();
      trex * to_trex();
};


/*-----------------------------------------------------------------------------
 * assertion
 *---------------------------------------------------------------------------*/
class assertion : public term
{
   public:
      typedef enum {CIRCUM, DOLLAR} assert_t;
      
   public:
      assertion(assert_t a);
      ~assertion(void);
      void unparse(FILE *out, int ident=0);
      void printtree(FILE *out, int indent=0);
      
      assert_t at;
      assertion  * clone();
      trex* to_trex();
};


/*-----------------------------------------------------------------------------
 * quantifier
 *---------------------------------------------------------------------------*/
class quantifier : public base_tree
{
   public:
      typedef enum {STAR, PLUS, QM, RANGE} q_t;
      
   public:
      quantifier(q_t q);
      ~quantifier();
      void unparse(FILE *out, int ident=0);
      void printtree(FILE *out, int indent=0);
      void set_greedy(unsigned int);
      unsigned int get_greedy(void);
      unsigned int greedy_;
      
      
      /* can be either a disjunction or a character class */
      q_t quant_type_;
      
      int low_, high_;
      quantifier * clone();
      trex* to_trex();
};


/*-----------------------------------------------------------------------------
 * atom
 *---------------------------------------------------------------------------*/
class atom : public base_tree
{
   public:
      typedef enum {CHAR_CLASS, DISJUNCT} a_t;
      
   public:
      atom(char_class *cc);
      atom(base_tree* dis);
      ~atom();
      void unparse(FILE *out, int ident=0);
      void printtree(FILE *out, int indent=0);
            
      /* can be either a disjunction or a character class */
      a_t atom_type_;
      base_tree *obj;
      
      atom * clone();
      trex* to_trex();
      atom(a_t at0, base_tree *obj0);
};




/*-----------------------------------------------------------------------------
 * char_class
 *   char_class is a unified representation of atoms.
 *---------------------------------------------------------------------------*/
class char_class : public base_tree
{
   public:
      typedef enum {ADD, SET} add_t;

   public:
      char_class();
      ~char_class();
      void unparse(FILE *out, int ident=0);
      void printtree(FILE *out, int indent=0);
      void set_chars(unsigned char clname, add_t mode);
      void add_chars(range *r, add_t mode);
      bool is_w();
      bool is_W();
      bool is_d();
      bool is_D();
      bool is_s();
      bool is_S();
      bool is_dot();
      
      /* if negated_ is true, then chars_ contains the
	 inverse of what we want to match */
      unsigned char named_class_;
      bool negated_;
      std::bitset<256> chars_;
      char_class * clone();
      trex* to_trex();
};


/*-----------------------------------------------------------------------------
 * range
 *   range represents a particular range, or a character, in a user-defined
 *   character class.  It is not present in the parse tree but is instead
 *   only used to transfer information during parsing.  Hence, it is not
 *   derived from base_tree.
 *
 *   Since perl allows things like [\D] (which is the same as \D), we will
 *   support it too, hence the need for cclass_.
 *---------------------------------------------------------------------------*/
class range
{
   public:
      range(int low, int high);
      int low_;
      int high_;
      unsigned char cclass_;
};


class modifiers
{
   public:
      modifiers(void);
      void append(char x);
      void process(void);

      std::list<char> mods;
};

/*-----------------------------------------------------------------------------
 * parse_regex
 *   This is the intended public interface for parsing a regular expression
 *   into a tree.  It presumes the regex is read from memory, and it performs
 *   modifier actions as necessary.
 *
 *   Call this function only if you want intermediate results (a parse tree)
 *   rather than an NFA.
 *
 *   Code is in parse_utils.cc
 *---------------------------------------------------------------------------*/
regex_tree_t* parse_regex(const char *data, int len, int line);

trex* to_trex(regex_tree_t* re);
void indent_tree(FILE *out, int amount);

/* The following #define yy... are a workaround to what it seems a bug in yacc:
 * If I use a prefix: yacc -pPREFIX X.y, then the generated X.cc file properly
 * redefines yy... variables (using a macro), i.e. yylval becomes PREFIXlval
 * But the corresponding X.hh does not have these definitions, and is looking
 * for yylval which no longer exists (because it was #defined as PREFIXlval).
 */
#define yyparse pcre_parse
#define yylval pcre_lval
#define yyerror pcre_error

#endif
