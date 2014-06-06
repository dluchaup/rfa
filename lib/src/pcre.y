/*-----------------------------------------------------------------------------
 *pcre.y
 *  Defines the grammar used for pcre-style regular expressions, and
 *  constructs a parse tree.
 *
 *  The grammar used is adapted and slightly simplified from:
 *     http://www.mozilla.org/js/language/js20/formal/regexp-grammar.html
 *
 *  Another grammar, very similar to that pointed to by the link above, 
 *  is contained at the bottom of this file.
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

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "errmsg.h"             //warnf() and errf()
#include "nodes.h"
#include "globals.h"

extern int lineno;
extern disjunction *g_regex;

/* g++ requires these */
int yylex();
extern "C" int yyerror(const char *);


//#define dbgprintf printf
//#define dbgprintf (void)
#define dbgprintf if (0) printf

%}


%union {
      unsigned int  tokenval;
      unsigned int  intval;
      unsigned char character;
      unsigned char class_escape;
      class disjunction * disjunct_val;
      class conjunction * conjunct_val;
      class term *        term_val;
      unsigned int  assert_val;
      class quantifier *  quant_val;
      class atom *        atom_val;
      class char_class *  char_class_val;
      class range *       range_val;
      class modifiers *   modifier_val;
}

/* Terminal symbols: these are things passed to yacc from lexer.
   Each token must have its own type */
%token <tokenval>     ALT_SYM LPAREN_SYM RPAREN_SYM PLUS_SYM STAR_SYM DOT
%token <tokenval>     QM_SYM CIRCUM_SYM DOLLAR_SYM SLASH_SYM
%token <tokenval>     LCURLY_SYM RCURLY_SYM COMMA_SYM
%token <tokenval>     LSQUARE_SYM LSQUARE_NOT_SYM RSQUARE_SYM DASH_SYM
%token <character>    CHAR
%token <intval>       HEX UNSIGNED
%token <class_escape> CHAR_CLASS_ESCAPE

/* Declare types of non-terminals.  Not all non-terminals need
   type identifiers */
%type  <disjunct_val>   regex disjunction
%type  <conjunct_val>   conjunction
%type  <term_val>       term
%type  <quant_val>      quantifier quantifierpref
%type  <assert_val>     assertion
%type  <atom_val>       atom
%type  <char_class_val> set set_items
%type  <range_val>      set_item
%type  <modifier_val>   modifiers modifier_list

%%

regex:          SLASH_SYM disjunction SLASH_SYM modifier_list
                {
		   g_regex = $2;
		   dbgprintf("Created the regex.\n");
		   
		   g_flags.reset();
		   if ($4 != NULL)
		   {
		      $4->process();
		      delete $4;
		   }
		}
                ;

modifier_list:  modifiers
                {
		   $$ = $1;
                }
          |     {
                   $$ = NULL;
                };


modifiers:      modifiers CHAR
                {
		  /* modifiers give us the ability to specify global
		     settings associated with a reg exp such as
		     case sensitivity, the meaning of ^, and so forth */
		   $1->append($2);
		   $$ = $1;

		}
          |     CHAR
                {
		   $$ = new modifiers();
		   $$->append($1);
                }
                ;

disjunction:    disjunction ALT_SYM conjunction
                {
		   $1->alternatives.push_back($3);
		   $$ = $1;
		   dbgprintf("parsed <disjunction> <conjunction>.\n");
                }
                | conjunction
                {
		   $$ = new disjunction(); 
		   $$->alternatives.push_back($1);
		   dbgprintf("created <disjunction>, added <conjunction>.\n");
		}
                ;

conjunction:    conjunction term
                {
		   $1->terms.push_back($2);
		   $$ = $1;
		   dbgprintf("added <term> to <conjunction>>.\n");
                }
                | 
                {
		   $$ = new conjunction();
		   dbgprintf("created new <conjunction>.\n");
		}
                ;

term:           assertion
                {
		   if ($1 == CIRCUM_SYM)
		      $$ = new assertion(assertion::CIRCUM);
		   else if ($1 == DOLLAR_SYM)
		      $$ = new assertion(assertion::DOLLAR);
		   else
		      assert(0);
		   dbgprintf("created a new <assertion>.\n");
		}
                | atom
                {
		   $$ = new term($1, NULL);
		   dbgprintf("created a new term from <atom>.\n");
		}
                | atom quantifier
                {
		   $$ = new term($1, $2);
		   dbgprintf("created a new term from <atom> <qual>.\n");
		}
                ;

quantifier:     quantifierpref
                {
		   $$ = $1;
		   dbgprintf("parsed <quantifierpref>.\n");
		   
		}
                |    quantifierpref QM_SYM
                {
		   /* should NEVER get here */
		   yyerror("Don't support greedy operations!.\n");
		   $$ = $1;
		   $$->set_greedy(0);
		   dbgprintf("parsed <quantifierpref> <QM_SYM>.\n");
		}		
                ;

quantifierpref: STAR_SYM 
                {
		   $$ = new quantifier(quantifier::STAR);
		   dbgprintf("parsed <STAR_SYM>.\n");
		}
                |    PLUS_SYM 
                {
		   $$ = new quantifier(quantifier::PLUS);
		   dbgprintf("parsed <PLUS_SYM>.\n");
		}
                |    QM_SYM 
                {
		   $$ = new quantifier(quantifier::QM);
		   dbgprintf("parsed <QM_SYM>.\n");
		}
                |    LCURLY_SYM UNSIGNED RCURLY_SYM
                {
		   $$ = new quantifier(quantifier::RANGE);
		   $$->low_ = $$->high_ = $2;
		   dbgprintf("parsed {d}.\n");
		}
                |    LCURLY_SYM UNSIGNED COMMA_SYM RCURLY_SYM
                {
		   $$ = new quantifier(quantifier::RANGE);
		   $$->low_ = $2;
		   $$->high_ = -1;
		   dbgprintf("parsed <{d,}>.\n");
		}
                |    LCURLY_SYM UNSIGNED COMMA_SYM UNSIGNED RCURLY_SYM
                {
		   $$ = new quantifier(quantifier::RANGE);
		   $$->low_ = $2;
		   $$->high_ = $4;
		   dbgprintf("parsed {d,d}.\n");
		}
                |    LCURLY_SYM COMMA_SYM UNSIGNED RCURLY_SYM
                {
		   $$ = new quantifier(quantifier::RANGE);
		   $$->low_ = -1;
		   $$->high_ = $3;
		   dbgprintf("parsed {d,d}.\n");
		}
                ;

assertion:      CIRCUM_SYM
                {
		   dbgprintf("returning CIRCUM_SYM assertion.\n");
		   $$ = CIRCUM_SYM;
		}
                |    DOLLAR_SYM
                {
		   dbgprintf("returning DOLLAR_SYM assertion.\n");
		   $$ = DOLLAR_SYM;
		}
                ;


set:            LSQUARE_SYM       set_items   RSQUARE_SYM
                {
		   $$ = $2;
		}
          |     LSQUARE_NOT_SYM   set_items   RSQUARE_SYM
                {
		   $$ = $2;
		   $$->negated_ = true;
		}
                ;


set_items:      set_item
                {
		   $$ = new char_class();
		   $$->named_class_ = '\0';
		   $$->add_chars($1, char_class::ADD);
		   delete $1;//keep valgrind happy
		}
          |     set_items set_item
                {
		   $1->add_chars($2, char_class::ADD);
		   $$ = $1;

		   delete $2;
		}
                ;


set_item:       CHAR DASH_SYM CHAR
                { 
		   if ($3 < $1)
		      errf(lineno, "Invalid range");
		   $$ = new range($1,$3);
		}
          |     CHAR
                {
		   $$ = new range($1,$1);
		}
          |     CHAR_CLASS_ESCAPE
                {
		   /* perl allows this, so we should also */
		   $$ = new range(-1,-1);
		   $$->cclass_ = $1;
		}
                ;


atom:           CHAR
                {
		   char_class* cc = new char_class();

		   /* if case-insensitive, insert both cases */
		   //if (g_flags.case_insensitive && isalpha($1))
		   //{
		   //   cc->chars_.set(toupper($1));
		   //   cc->chars_.set(tolower($1));
		   //}
		   //else
		      cc->chars_.set($1);
		   

		   $$ = new atom(cc);
		   if (isprint($1))
		     {dbgprintf("parsed <CHAR>:%c\n", $1);}
		   else
		     {dbgprintf("parsed <CHAR>:%02x\n", $1);}
		}

           |    DOT
                {
		   char_class *cc = new char_class();
		   cc->chars_.set();
		   cc->named_class_ = '.';
		   $$ = new atom(cc);
		   dbgprintf("parsed DOT.\n");
		}

           |    LPAREN_SYM disjunction RPAREN_SYM
                {
		   $$ = new atom($2);
		   dbgprintf("parsed <disjunction> from with atom).\n");
		}

           |    set
                { 
		   $$ = new atom($1);
                } 

           |    CHAR_CLASS_ESCAPE
                {
		   char_class *cc = new char_class();
		   cc->named_class_ = $1;
		   cc->set_chars($1, char_class::SET);
		   $$ = new atom(cc);
		   dbgprintf("parsed \\%c\n", $1);
		}
                ;


/* ---------------------------------------------------------------------------
 * Alternate Grammar

Following the precedence rules given previously, a BNF grammar for 
Perl-style regular expressions can be constructed as follows.
<RE> 	::= 	<union> | <simple-RE>
<union> 	::=	<RE> "|" <simple-RE>
<simple-RE> 	::= 	<concatenation> | <basic-RE>
<concatenation> 	::=	<simple-RE> <basic-RE>
<basic-RE> 	::=	<star> | <plus> | <elementary-RE>
<star> 	::=	<elementary-RE> "*"
<plus> 	::=	<elementary-RE> "+"
<elementary-RE> 	::=	<group> | <any> | <eos> | <char> | <set>
<group> 	::= 	"(" <RE> ")"
<any> 	::= 	"."
<eos> 	::= 	"$"
<char> 	::= 	any non metacharacter | "\" metacharacter
<set> 	::= 	<positive-set> | <negative-set>
<positive-set> 	::= 	"[" <set-items> "]"
<negative-set> 	::= 	"[^" <set-items> "]"
<set-items> 	::= 	<set-item> | <set-item> <set-items>
<set-items> 	::= 	<range> | <char>
<range> 	::= 	<char> "-" <char>

 \ _

NullEscape => \ _
AtomEscape 


--------------------------------------------------------------------------- */

/*
Summary for Current Grammar (after I changed 'alternative' to 'conjunction'):

regex:          SLASH_SYM disjunction SLASH_SYM modifier_list;
modifier_list:  modifiers |     ;
modifiers:      modifiers CHAR | CHAR ;
disjunction:    disjunction ALT_SYM conjunction | conjunction              ;
conjunction:    conjunction term |   ;
term:           assertion        | atom         | atom quantifier          ;
quantifier:     quantifierpref   |    quantifierpref QM_SYM                ;
quantifierpref: STAR_SYM         |    PLUS_SYM  |    QM_SYM 
                |    LCURLY_SYM UNSIGNED RCURLY_SYM
                |    LCURLY_SYM UNSIGNED COMMA_SYM RCURLY_SYM
                |    LCURLY_SYM UNSIGNED COMMA_SYM UNSIGNED RCURLY_SYM
                |    LCURLY_SYM COMMA_SYM UNSIGNED RCURLY_SYM
                ;
assertion:      CIRCUM_SYM       |    DOLLAR_SYM ;
set:            LSQUARE_SYM       set_items   RSQUARE_SYM
          |     LSQUARE_NOT_SYM   set_items   RSQUARE_SYM
                ;
set_items:      set_item         |     set_items set_item                  ;
set_item:       CHAR DASH_SYM CHAR  |     CHAR    |     CHAR_CLASS_ESCAPE
                ;
atom:           CHAR | DOT | LPAREN_SYM disjunction RPAREN_SYM | set | CHAR_CLASS_ESCAPE ;

*/
