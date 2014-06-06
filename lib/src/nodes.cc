/*-----------------------------------------------------------------------------
 *nodes.cc
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
#include <list>
#include <assert.h>
#include <stdio.h>

#include "nodes.h"
#include "escape_sequences.h"
#include "globals.h"
#include "trex.h"
#include "zoptions.h" //TBD:nov26: delete
#include "MyTime.h" //TBD:nov26: delete

#define UNUSED_ARG(x)  (void)(x)

void indent_(FILE *out, int amount)
{
   UNUSED_ARG(out);
   UNUSED_ARG(amount);
}

void indent_tree(FILE *out, int amount)
{
   unsigned int i;
   
   for (i=0; i < (unsigned int)amount; i++)
      fprintf(out, " ");
}


/*-----------------------------------------------------------------------------
 * base_tree implementation
 *---------------------------------------------------------------------------*/
int unsigned base_tree::g_uid = 0;
base_tree::base_tree(void) {uid=++g_uid;}
base_tree::~base_tree(void) {}


/*-----------------------------------------------------------------------------
 * disjunction implementation
 *---------------------------------------------------------------------------*/
disjunction::disjunction(void) {}
disjunction * disjunction::clone() {
  disjunction *cld = new disjunction();
  std::list<conjunction *>::iterator li;
  for (li = alternatives.begin(); li != alternatives.end(); li++)
    cld->alternatives.push_back((*li)->clone());
  return cld;
}

// use left associativity: turn r1|r2|r3|r4 to ((tr1|tr2)|r3)|r4
trex * disjunction::to_trex() {
  trex *trx = NULL;
  std::list<conjunction *>::iterator li;
  for (li = alternatives.begin(); li != alternatives.end(); li++) {
    trex *trx2 = (*li)->to_trex();
    if (trx == NULL)
      trx = trx2;
    else
      trx = new trex_disjunction(trx, trx2);
  }
  assert(trx);
  return trx;
}


disjunction::~disjunction(void) 
{
   std::list<conjunction *>::iterator li;

   for (li = alternatives.begin(); li != alternatives.end(); li++)
      delete (*li);
}

void disjunction::unparse(FILE *out, int indent)
{
   std::list<conjunction *>::iterator li;

   bool first = true;
   for (li = alternatives.begin(); li != alternatives.end(); li++)
   {
       if (!first)
	   fprintf(out, "|");

      indent_(out, indent);
      (*li)->unparse(out, indent+2);
      first = false;
   }
}


void disjunction::printtree(FILE *out, int indent)
{
   std::list<conjunction *>::iterator li;

   indent_tree(out, indent);
   fprintf(out, "<DIS>\n");
   for (li = alternatives.begin(); li !=alternatives.end(); li++)
   {
      (*li)->printtree(out, indent+2);
      //fprintf(out,"\n");
   }
}
   

/*-----------------------------------------------------------------------------
 * add_implicit_star
 *   adds a .* to the front of the parse
 *---------------------------------------------------------------------------*/
void disjunction::add_star(void)
{
   /* build the .* */
   char_class *cc = new char_class();
   cc->chars_.set();
   cc->named_class_ = '.';

   atom *a = new atom(cc);
   quantifier *q = new quantifier(quantifier::STAR);

   term *dotstar = new term(a, q);

   /* now, build up a spot for the new term */
   disjunction *d = new disjunction();
   d->alternatives = this->alternatives;
   this->alternatives.clear();

   atom *a2 = new atom(d);
   term *t2 = new term(a2, NULL);

   conjunction *alt = new conjunction();
   alt->terms.push_back(dotstar);
   alt->terms.push_back(t2);

   this->alternatives.push_back(alt);
}


/*-----------------------------------------------------------------------------
 * conjunction implementation
 *---------------------------------------------------------------------------*/
conjunction::conjunction(void) {}
conjunction* conjunction::clone() {
  conjunction *cld = new conjunction();
  std::list<term *>::iterator li;
  for (li = terms.begin(); li != terms.end(); li++)
    cld->terms.push_back((*li)->clone());
  return cld;
}

// use left associativity: turn r1.r2.r3.r4 to ((tr1.tr2).r3).r4
trex * conjunction::to_trex() {
  trex *trx = NULL;
  std::list<term *>::iterator li;
  for (li = terms.begin(); li != terms.end(); li++) {
    trex *trx2 = (*li)->to_trex();
    if (trx == NULL)
      trx = trx2;
    else
      trx = new trex_conjunction(trx, trx2);
  }
  if (!trx)
    trx = new trex_epsilon();
  return trx;
}


conjunction::~conjunction(void) 
{
   std::list<term *>::iterator li;

   for (li = terms.begin(); li != terms.end(); li++)
      delete (*li);
}

void conjunction::unparse(FILE *out, int indent)
{
   std::list<term *>::iterator li;


   //if (terms.size() > 1)
   //{
   //   fprintf(out, "(");
   //}

   for (li = terms.begin(); li != terms.end(); li++)
   {
      indent_(out, indent);
      (*li)->unparse(out, indent+2);
   }

   //if (terms.size() > 1)
   //{
   //   fprintf(out, ")");
   //}
}

void conjunction::printtree(FILE *out, int indent)
{
   std::list<term *>::iterator li;

   indent_tree(out, indent);
   fprintf(out, "<TERM>\n");
   for (li = terms.begin(); li != terms.end(); li++)
   {
      (*li)->printtree(out, indent+2);
      //fprintf(out,"\n");
   }
}


/*-----------------------------------------------------------------------------
 * term implementation
 *---------------------------------------------------------------------------*/
term::term(atom *child, quantifier *q) : a_(child), quant_(q) {}
term* term::clone() {
  atom *a = a_?a_->clone():NULL;
  quantifier *q = quant_?quant_->clone():NULL;
  term *cld = new term(a, q);
  return cld;
}

trex* term::to_trex() {
  assert(a_);
  trex *trx = a_->to_trex();
  if (quant_) {
    
    switch(quant_->quant_type_)  {
    case quantifier::STAR://      fprintf(out, "*");
      trx = new trex_disjunction(new trex_epsilon(), new trex_plus(trx));
      break;
    case quantifier::PLUS:
      trx = new trex_plus(trx);
      //fprintf(stderr, "+");
      break;
    case quantifier::QM:
      trx = new trex_disjunction(new trex_epsilon(), trx);
      //fprintf(stderr, "?");
      break;
    case quantifier::RANGE: {
     bool thompson_expand_range = zop.dbgBoolOption("thompson_expand_range"); //false;
     trex *trx0 = trx;
     assert(quant_->low_ >= 0 && quant_->high_ >= -1 );
     if (thompson_expand_range || quant_->high_ == -1) {
      //DMP(quant_->low_);DMP(quant_->high_);
      /* R^low_ | R^(low_+1)  | ... R^high */
      trex *tmp_mult = trx;
      
      for (int i = 1; i < quant_->low_; ++i)
	tmp_mult = new trex_conjunction(tmp_mult, trx0->clone());

      if (quant_->high_ == -1) {
	trx = new trex_conjunction(tmp_mult, new trex_plus(trx0->clone()));
	break;
      }

      assert(quant_->low_ >= 0 && quant_->high_ >= quant_->low_ );
      
      trex *alt = tmp_mult;
      
      int low = (quant_->low_ == 0)? 1: quant_->low_;
      for (int i = low; i < quant_->high_; ++i) {
	tmp_mult = new trex_conjunction(tmp_mult->clone(), trx0->clone());
	alt = new trex_disjunction(alt, tmp_mult);
      }
      
      if (quant_->low_ == 0)
	trx = new trex_disjunction(new trex_epsilon(), alt);
      else
	trx = alt;
     }
     else {
       trx = new trex_range(trx0, quant_->low_, quant_->high_);
     }
     break;
    }
      assert(0 && "UNIMPLEMENTED"); 
      if (quant_->low_ == quant_->high_)
	  fprintf(stderr, "{%d}", quant_->low_);
	else if (quant_->high_ == -1)
	  fprintf(stderr, "{%d,}", quant_->low_);
	else
	  fprintf(stderr, "{%d,%d}", quant_->low_, quant_->high_);
	break;
    default:
      assert(0);
    }
  }
  assert(trx);
  return trx;
}

term::~term(void) 
{
   if (a_)
      delete a_;

   if (quant_)
      delete quant_;
}

void term::unparse(FILE *out, int indent)
{
    if (a_)
	a_->unparse(out, indent+2);
   if(quant_)
      quant_->unparse(out, indent+2);
}

void term::printtree(FILE *out, int indent)
{
   indent_tree(out, indent);
   fprintf(out, "<ATOM>");
   if (a_) 
      a_->printtree(out, indent+2);
   else
   {
      indent_tree(out, indent+2);
      fprintf(out, "**NO ATOM**\n");
   }

   if (quant_)
   {
      indent_tree(out, indent);
      fprintf(out, "<QUANT>");
      if (quant_) 
	 quant_->printtree(out, indent+2);
      
      else
      {
	 indent_tree(out, indent+2);
	 fprintf(out, "**NO QUANT**\n");
      }
   }
}


/*-----------------------------------------------------------------------------
 * assertion implementation
 *---------------------------------------------------------------------------*/
assertion::assertion(assert_t a) : term(NULL, NULL), at(a) {}
assertion* assertion::clone() {
  assertion *cld = new assertion(at);
  return cld;
}

trex* dot_star() {
  bitset<MAX_SYMS> chars;
  chars.set();
  // build '.'
  trex *trx = NULL;
  if (!zop.dbgDefinedOption("TREX_EXPAND_DOTS") && (chars.count() > 1)) {
    trx = new trex_byte_set(chars);
  } else {
    for (unsigned int i=0; i < MAX_SYMS; i++) {
      //if (i == 64) {fprintf(stderr, "\nHAKKKKKKKKKKKKK\n"); break;}
      if (chars.test(i)) {
        trex *trx2 = new trex_byte(i);
      if (trx == NULL)
	trx = trx2;
      else
	trx = new trex_disjunction(trx, trx2);
      }
    }
  }
  assert(trx);
  // build '.*'
  trx = new trex_disjunction(new trex_epsilon(), new trex_plus(trx));
  assert(trx);
  return trx;
}

trex* assertion::to_trex() {
 if (this->at == assertion::CIRCUM) {
    if (g_flags.multi_line) /* we've got an assert and multiline allowed*/
    { /* .*\n */
      // build '.*\n'
      //return new trex_conjunction(dot_star(), new trex_byte('\n'));
      return new trex_disjunction(new trex_epsilon(),
				  new trex_conjunction(dot_star(), new trex_byte('\n')));
    }
    else
    {
      fprintf(stdout, 
	      "WARNING: ^ assert present, but multi-line flag not set. "
	      "Ignoring.\n");
      return new trex_epsilon();
    }
 } else if (this->at == assertion::DOLLAR)
 {
    fprintf(stdout, 
	    "WARNING: ignoring occurrence of $ assert.");
    return new trex_epsilon();
 }
 assert(0 && "UNIMPLEMENTED"); return NULL;
}

assertion::~assertion(void) {}

void assertion::unparse(FILE *out, int indent)
{
   UNUSED_ARG(indent);

   if (at == CIRCUM)
      fprintf(out, "^");
   else if (at == DOLLAR)
      fprintf(out, "$");
   else
      assert(0);
}


void assertion::printtree(FILE *out, int indent)
{
   indent_tree(out, indent);

   if (at == CIRCUM)
      fprintf(out, "<ASSERT> ^\n");
   else if (at == DOLLAR)
      fprintf(out, "<ASSERT> $\n");
   else
      assert(0);
}


/*-----------------------------------------------------------------------------
 * quantifier implementation
 *---------------------------------------------------------------------------*/
quantifier::~quantifier(void) {}
quantifier* quantifier::clone() {
  quantifier *cld = new quantifier(quant_type_);
  cld->greedy_ = greedy_;
  cld->low_  = low_;
  cld->high_ = high_;
  return cld;
}

trex* quantifier::to_trex() {
  assert(0 && "UNIMPLEMENTED"); return NULL;
}

quantifier::quantifier(q_t a) : quant_type_(a) 
{
   low_ = high_ = -1;
   greedy_ = 1;
}

void quantifier::set_greedy(unsigned int val)
{
   greedy_ = (val > 0);
}

unsigned int quantifier::get_greedy(void)
{
   return greedy_;
}

void quantifier::unparse(FILE *out, int indent)
{
   UNUSED_ARG(indent);

   switch(quant_type_)
   {
      case STAR:
	 fprintf(out, "*"); break;
      case PLUS:
	 fprintf(out, "+"); break;
      case QM:
	 fprintf(out, "?"); break;
      case RANGE:
	 if (low_ == high_)
	    fprintf(out, "{%d}", low_);
	 else if (high_ == -1)
	    fprintf(out, "{%d,}", low_);
	 else
	    fprintf(out, "{%d,%d}", low_, high_);
	 break;
      default:
	 assert(0);
   }

   if (!greedy_)
       fprintf(out, "?");
}

void quantifier::printtree(FILE *out, int indent)
{
   indent_tree(out, indent);

   switch(quant_type_)
   {
      case STAR:
	 fprintf(out, "<STAR> *"); break;
      case PLUS:
	 fprintf(out, "<PLUS> +"); break;
      case QM:
	 fprintf(out, "<QM> ?"); break;
      case RANGE:
	 if (low_ == high_)
	    fprintf(out, "{%d}", low_);
	 else if (high_ == -1)
	    fprintf(out, "{%d,}", low_);
	 else
	    fprintf(out, "{%d,%d}", low_, high_);
	 break;
      default:
	 assert(0);
   }

   fprintf(out, "\n");
}


/*-----------------------------------------------------------------------------
 * atom implementation
 *---------------------------------------------------------------------------*/
atom::atom(char_class *cc) : obj(cc)  { atom_type_ = CHAR_CLASS; }
atom::atom(base_tree *dis) : obj(dis) { atom_type_ = DISJUNCT; }
atom::atom(a_t at0, base_tree *obj0): atom_type_(at0), obj(obj0) {}

atom* atom::clone() {
  atom* cld = new atom(atom_type_, obj->clone());
  return cld;
}

trex* atom::to_trex() {
  assert(obj);
  trex *trx = obj->to_trex();
  assert(trx);
  return trx;
}

atom::~atom(void) 
{
   if (obj)
      delete obj;
}

void atom::unparse(FILE *out, int indent)
{
   (void)indent;

   if (atom_type_ == DISJUNCT)
      fprintf(out, "(");
   obj->unparse(out, indent);
   if (atom_type_ == DISJUNCT)
      fprintf(out, ")");
}

void atom::printtree(FILE *out, int indent)
{
   (void)indent;

   //indent_tree(out, indent);
   obj->printtree(out, indent+2);
   //fprintf(out, "\n");
}


/*-----------------------------------------------------------------------------
 * char_class implementation
 *---------------------------------------------------------------------------*/
char_class *char_class::clone() {
  char_class *cld = new char_class();
  cld->named_class_ = named_class_;
  cld->negated_ = negated_;
  cld->chars_ = chars_;
  return cld;
}

#define USE_TREX_BYTE_SET 1
trex* char_class::to_trex() {
  bitset<MAX_SYMS> chars = chars_;
  
  if (negated_)
    chars.flip();
  if (g_flags.case_insensitive)
    for (unsigned int i=0; i < MAX_SYMS; i++) {
      //if (i == 64) {fprintf(stderr, "\nHAKKKKKKKKKKKKK\n"); break;}
      if (chars.test(i)) {
	chars.set(toupper(i));
	chars.set(tolower(i));
      }
    }

  trex *trx = NULL;
  
  if (!zop.dbgDefinedOption("TREX_EXPAND_DOTS") && (chars.count() > 1)) {
    trx = new trex_byte_set(chars);
  } else {
  for (unsigned int i=0; i < MAX_SYMS; i++) {
    //if (i == 64) {fprintf(stderr, "\nHAKKKKKKKKKKKKK\n"); break;}
    if (chars.test(i)) {
      trex *trx2 = new trex_byte(i);
      if (trx == NULL)
	trx = trx2;
      else
	trx = new trex_disjunction(trx, trx2);
    }
  }
  }
  assert(trx);//OK, not quite. Something like [^a-z] may cause this
  return trx;
}

char_class::~char_class(void) {}

char_class::char_class() : named_class_('\0'), negated_(false) 
{
   chars_.reset();
}

bool char_class::is_w() { return named_class_ == 'w'; }
bool char_class::is_W() { return named_class_ == 'W'; }
bool char_class::is_d() { return named_class_ == 'd'; }
bool char_class::is_D() { return named_class_ == 'D'; }
bool char_class::is_s() { return named_class_ == 's'; }
bool char_class::is_S() { return named_class_ == 'S'; }
bool char_class::is_dot() { return named_class_ == '.'; }


void char_class::add_chars(range *r, add_t mode)
{
   assert(r);

   if (r->low_ != -1 || r->high_ != -1)
   {
      if (mode == char_class::SET)
	 chars_.reset();

      for (int i=r->low_; i <= r->high_; i++)
	 chars_.set(i);
   }
   else
   {  
      set_chars(r->cclass_, mode);
   }
}

/*-----------------------------------------------------------------------------
 *set_chars
 *  Sets characters in the character class according to the supplied
 *  escaped class name (e.g., \s, \S, \d, \D, \w, \W).
 *  The 'mode' parameter specifies whether or not the escaped class clname
 *  should be the only thing in the instance, or whether it should be
 *  added to the instance.
 *---------------------------------------------------------------------------*/
void char_class::set_chars(unsigned char clname, add_t mode)
{
   bitset<256> tchars;

   switch(clname)
   {
      case 's':
	 tchars.reset();
	 tchars.set(' ');
	 tchars.set('\t');
	 tchars.set('\r');
	 tchars.set('\n');
	 tchars.set('\f');
	 break;

      case 'S':
	 tchars.reset();
	 tchars.set(' ');
	 tchars.set('\t');
	 tchars.set('\r');
	 tchars.set('\n');
	 tchars.set('\f');
	 tchars.flip();
	 break;

      case 'd':
	 tchars.reset();
	 for (int i='0'; i <= '9'; i++) tchars.set(i);
	 break;

      case 'D':
	 tchars.reset();
	 for (int i='0'; i <= '9'; i++) tchars.set(i);
	 tchars.flip();

	 break;
      case 'w':
	 tchars.reset();
	 for (int i='A'; i <= 'Z'; i++) tchars.set(i);
	 for (int i='a'; i <= 'z'; i++) tchars.set(i);
	 for (int i='0'; i <= '9'; i++) tchars.set(i);
	 break;

      case 'W':
	 tchars.reset();
	 for (int i='A'; i <= 'Z'; i++) tchars.set(i);
	 for (int i='a'; i <= 'z'; i++) tchars.set(i);
	 for (int i='0'; i <= '9'; i++) tchars.set(i);
	 tchars.flip();
	 break;

      default:
	 assert(0);
   }

   if (mode == SET)
      chars_ = tchars;
   else if (mode == ADD)
      chars_ |= tchars;
   else
      assert(0);
}


void char_class::unparse(FILE *out, int indent)
{
   UNUSED_ARG(indent);

   switch(named_class_)
   {
      case 'w': fprintf(out, "\\w"); break;
      case 'W': fprintf(out, "\\W"); break;
      case 'd': fprintf(out, "\\d"); break;
      case 'D': fprintf(out, "\\D"); break;
      case 's': fprintf(out, "\\s"); break;
      case 'S': fprintf(out, "\\S"); break;
      case '.': fprintf(out, "."); break;
      default:
      {
	 bool dot = false;

	 if (!negated_)
	 {
	    dot = true;
	    for (unsigned int i=0; i < 256; i++)
	    {
	       if (!chars_.test(i))
	       {
		  dot = false;
	       }
	    }
	    
	    if (dot)
	    {
	       fprintf(out, ".");
	    }   
	 }

	 if (!dot)
	 {
	    if (negated_)
	       fprintf(out, "[^");
	    else if (chars_.count() > 1)
	       fprintf(out, "[");
	    
	    for (unsigned int i=0; i < 256; i++)
	    {
	       if (chars_.test(i))
	       {
		  char outstr[10];
		  fprintf(out, "%s",output_char(outstr, 10, i,
						negated_||(chars_.count()>1))
		     );
	       }
	    }
	    
	    if (negated_ || (chars_.count() > 1))
	       fprintf(out, "]");
	 }
      }
   }
}


void char_class::printtree(FILE *out, int indent)
{
   indent_tree(out, indent);
   switch(named_class_)
   {
      case 'w': fprintf(out, "\\w"); break;
      case 'W': fprintf(out, "\\W"); break;
      case 'd': fprintf(out, "\\d"); break;
      case 'D': fprintf(out, "\\D"); break;
      case 's': fprintf(out, "\\s"); break;
      case 'S': fprintf(out, "\\S"); break;
      case '.': fprintf(out, "."); break;
      default:
	 if (negated_)
	    fprintf(out, "[^");
	 else if (chars_.count() > 1)
	    fprintf(out, "[");

	 for (unsigned int i=0; i < 256; i++)
	 {
	    if (chars_.test(i))
	    {
	       char outstr[10];
	       fprintf(out, "%s",output_char(outstr, 10, i,
					     negated_||(chars_.count()>1))
			  );
	    }
	 }

	 if (negated_ || (chars_.count() > 1))
	     fprintf(out, "]");
   }
   fprintf(out, "\n");
}


/*-----------------------------------------------------------------------------
 * range implementation
 *---------------------------------------------------------------------------*/
range::range(int low, int high) : low_(low), high_(high) {}

/*-----------------------------------------------------------------------------
 * modifiers implementation
 *---------------------------------------------------------------------------*/
modifiers::modifiers(void) {}

void modifiers::append(char x)  
{ 
  mods.push_back(x); 
}

void modifiers::process(void) 
{
  std::list<char>::iterator li;
  
  for (li = mods.begin(); li != mods.end(); li++)
    {
      switch(*li) 
	{
	case 'i':
	  g_flags.case_insensitive = 1; 
	  fprintf(stdout, "Setting case_insensitive g_flag\n");
	  break;

	case 'm':
	  g_flags.multi_line = 1; 
	  fprintf(stdout, "Setting multi_line g_flag\n");
	  break;

	case 't':
	  g_flags.implicit_star = 1; 
	  fprintf(stdout, "Setting implicit_star g_flag\n");
	  break;
	  
	case 'A':
	  g_flags.known_ambiguous = 1; 
	  fprintf(stdout, "Setting known_ambiguous g_flag\n");
	  break;

	default:
	  fprintf(stdout, "unrecognized modifier %c\n", *li);
	  break;
	}
    }
}

/*-----------------------------------------------------------------------------
 * parse_regex
 *   This is the intended public interface for parsing a regular expression
 *   into a tree.  This wraps regex_from_mem while also applying any modifiers
 * (dl: moved here from Randy's re.cc)
 *---------------------------------------------------------------------------*/
#include "parse_utils.h"
regex_tree_t* parse_regex(const char *data, int len, int line)
{
   disjunction *re;

   re = regex_from_mem(data, len, line);
   if (g_flags.implicit_star)
   {
      fprintf(stdout, "Adding implicit star\n");
      re->add_star();
   }

   return re;
}

trex* to_trex(regex_tree_t* re) {
  DbgTime tm("to_trex");
  trex* trx = re->to_trex();
  if (g_flags.implicit_star) {
    trx = new trex_conjunction(dot_star(), trx);
  }
  if (g_flags.known_ambiguous)
    trx->known_ambiguous = true;
  return trx;
}

trex* regex2trex(const std::string &regexp, //TBD:nov26: move
		 void* vdbg_tm_parse, void* vdbg_tm_convert) {
  DbgTime* dbg_tm_parse = (DbgTime*) vdbg_tm_parse;
  DbgTime* dbg_tm_convert = (DbgTime*) vdbg_tm_convert;
  if (dbg_tm_parse)   dbg_tm_parse->resume();
  regex_tree_t* re  = parse_regex(regexp.c_str(), regexp.length(), 1);
  if (dbg_tm_parse)   dbg_tm_parse->stop();

  if (dbg_tm_convert) dbg_tm_convert->resume();
  trex *trx = to_trex(re);
  if (dbg_tm_convert) dbg_tm_convert->stop();
  
  delete re;
  return trx;
}
