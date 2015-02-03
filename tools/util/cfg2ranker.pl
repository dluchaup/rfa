#!/usr/bin/perl
##########################################################################
# File:    cfg2ranker.pl
# Author:  Daniel Luchaup
# Date:    April 2014
# Copyright 2014 Daniel Luchaup luchaup@cs.wisc.edu
#
# This file contains unpublished confidential proprietary work of
# Daniel Luchaup, Department of Computer Sciences, University of
# Wisconsin--Madison.  No use of any sort, including execution, modification,
# copying, storage, distribution, or reverse engineering is permitted without
# the express written consent (for each kind of use) of Daniel Luchaup.
##########################################################################

use strict;
use File::Basename;
my $dirname = dirname(__FILE__);
my $REX_INTERSECT = "$dirname/../rex-intersect/bin/rex-intersect";

#################### globals    ########################
my $Gname;
my $start; #undefined
my %tokens;
my %nonterminals;
my @yrules;

my @code_class_def;
sub code2def {
    my ($msg) = @_;
    #print "CLASS: $msg";
    push (@code_class_def, $msg);
}

my @code_class_init;
sub code2init {
    my ($msg) = @_;
    #print "CLASS: $msg";
    push (@code_class_init, $msg);
}

### YACC ###
my @code_yacc;
sub code2yacc{
    my ($msg) = @_;
    #print "CODE YACC: $msg";
    push (@code_yacc, $msg);
}
### LEX ###
my @code_lex;
sub code2lex{
    my ($msg) = @_;
    #print "CODE lex: $msg";
    push (@code_lex, $msg);
}

sub reset_globals {
  undef $Gname;
  undef $start;
  undef %tokens;
  undef %nonterminals;
  undef @yrules;
  undef @code_class_def;
  undef @code_class_init;
  undef @code_yacc;
  undef @code_lex;
}
#################### globals    ########################


main(@ARGV);

exit(0);
    
sub dbgprint {
    my ($msg) = @_;
    print "DBG: $msg\n";
}

sub main {
  ####### check if needed tool exists #######
  unless (-e $REX_INTERSECT) {
    die "Required tool: $REX_INTERSECT doesn't exist!";
  }
  ###########################################
  @ARGV = @_;
  foreach (@ARGV) {
    handle_file ($_);
  }
}

sub handle_file {
    my $file = $_;

    reset_globals();
    $Gname = "exG";

    open my $fp, '<', $file or die "$! $file";

    my @gram = <$fp>;
    my @gram2 = @gram;
    handle_yacc(\@gram); #generates yacc code in @code_yacc;
    handle_lex(\@gram2); #generates lex  code in @code_lex;

    open my $fpyacc, '>', "$Gname.y" or die "$! $Gname.y";
    print $fpyacc @code_yacc;
    close $fpyacc;

    open my $fplex, '>', "$Gname.l" or die "$! $Gname.l";
    print $fplex @code_lex;
    close $fplex;

    #print "DEF CODE= \n",@code_class_def,"\n";
    #print "INI CODE= \n",@code_class_init,"\n";

    @code_class_def = (@code_class_def, @code_class_init);
    code2def("};\n");


    code2def("extern ${Gname} *g_${Gname};\n");
    code2def("#define g_${Gname}_ParseTree (g_${Gname}->pi->pt_result)\n");
    code2def("///////////// grammar parsing stuff ///////////////////\n");
    code2def("ParseTree *pt_from_file(FILE *fp);\n");
    code2def("ParseTree *pt_from_mem(const char *data, int len);\n");
    code2def("ParseTree *pt_from_mem(const std::string &data);\n");
    code2def("#endif\n");

    open my $fpdef, '>', "${Gname}_defs.h" or die "$! ${Gname}_defs.h";
    print $fpdef @code_class_def;
    close $fpdef;
  }

#######################################################
#################### lexer part #######################
#######################################################

# The LEX part allows definition of macros, that are them inlined in the rules.
# This function performs the inlining, so that our tool handles pure regexp
sub inline_macro {
  my ($macros, $replacement) = @_;
  my $inlined_replacement = $replacement;
  my $key;
  foreach $key (keys %$macros) {
    my $value = $macros->{$key};
    #dbgprint("key=$key .... value=$value");
    if ($inlined_replacement =~ /\\{$key}/) {
      die("Unhandled \ in macro substitution: $key in $inlined_replacement");
    }
    if ($inlined_replacement =~ s/{$key}/$value/g) {
      #dbgprint("MATCH2: key=$key in $inlined_replacement");
    }
  }
  return $inlined_replacement;
}

sub escape_C_strings {
  # $orig is some regular expression that we want to parse, say xy\.z
  # To parse it, we have to pass it as a constant string argument to a
  # C function, but then the funny characters may be missinterpreted by
  # the C compiler. So need to escape them, so that xy\.z gets passed to
  # a function as the string constant xy\\.z, which ends up as the
  # regular expression xy\.z
  my ($orig) = @_;
  my @arr = split(//,$orig);
  my @arr0 = @arr;
  my $dbg = join('',@arr0);
  my @res;
  my @strarr;

  my $state = "Init"; #"Q_init", "Range", "Q_range";
  my $c;

 STATES:
  while (@arr) {
    $c = shift (@arr);
    if ($state eq "Init") {
	if ($c eq '"') { push (@res, "\\"); }
	if ($c eq '\\') { push (@res, "\\"); $state = "Q_init"; }
	push (@res, $c);
	next STATES;
      }
    if ($state eq "Q_init") {
      if ($c eq '"') { push (@res, "\\\""); }
      else {
	  if ($c eq '\\') { push (@res, "\\\\"); }
	  else { push (@res, $c); }
      }
      $state = "Init";
      next STATES;
    }
  }
  my $result = join('',@res);
  if (! ($result eq $orig) ) {
    warn("Please check replacement of /$orig/ with /$result/ in .._defs.h\n");
  }
  return $result;
}

sub escape_literal_strings {#just those and nothing else ...
  # $orig is some LEX regular expression specification that may contain
  # a quoted string literal, such as "abc][.". LEX knows to remove the \"
  # and we do the same here, to turn it into a real regular expression.
  my ($orig) = @_;
  my @arr = split(//,$orig);
  my @arr0 = @arr;
  my $dbg = join('',@arr0);
  my @res;
  my @strarr;

  my $state = "Init"; #"Q_init", "Range", "Q_range", "String";
  my $c;

 STATES:
  while (@arr) {
    $c = shift (@arr);
    if ($state eq "Init") {
      if      ($c eq '"') { $state = "String"; }
      else {
	push (@res, $c);
	if ($c eq '[')  { $state = "Range"; }
	if ($c eq '\\') { $state = "Q_init"; }
      }
      next STATES;
    }
    if ($state eq "Range") {
      push (@res, $c);
      if ($c eq ']')  { $state = "Init"; }
      if ($c eq '\\') { $state = "Q_range"; }
      next STATES;
    }
    if ($state eq "Q_range") {
      push (@res, $c);
      $state = "Range";
      next STATES
    }
    if ($state eq "Q_init") {
      push (@res, $c);
      $state = "Init";
      next STATES;
    }
    if ($state eq "String") {
      if ($c eq '"') {
	@res = (@res, @strarr);
	undef @strarr;
	$state = "Init";
      } else {
	  if ($c =~ /[\.|\/\(\)\{\}\[\]\\\+\*\?\^\$]/) {
	    push (@strarr, "\\");
	  }
	  push (@strarr, $c);
      }
      next STATES;
    }
  }
  my $result = join('',@res);
  if (! ($result eq $orig) ) {
    warn("Please check replacement of /$orig/ with /$result/ in .._defs.h\n");
  }
  return $result;
}

# sub escape{
#   my ($orig) = @_;
#   my $tmp = escape_literal_strings($orig);
#   my $res = escape_C_strings($tmp);
#   return $res;
# }

sub check_conflicts {
  my ($rules2rex, $rule, $rex) = @_;
  my $rulekey;
  
  my $conflicts;
  
  foreach $rulekey (keys %$rules2rex) {
    my $otherrex = $rules2rex->{$rulekey};
    my $file  = "/tmp/tmp-$$.$rulekey.$rule"; # $$ is the pid
    open my $ftmp, '>', $file or die "$! $file";
    print $ftmp "/$otherrex/\n";
    print $ftmp "/$rex/\n";
    my @args = ($REX_INTERSECT, $file);#TBD: fix path
    my $res = system(@args);
    if ($res == 0) {
      print "YES-CONFLICT[$res:$file]: $otherrex with $rex\n";
      if (! defined $conflicts) {
	$conflicts = "($otherrex)";
      } else {
	$conflicts = $conflicts."|($otherrex)";
      }
    } else {
      print "NO-CONFLICT[$res:$file]: $otherrex with $rex\n";
    }
    close $ftmp;
    unlink $file or warn "Could not unlink $file: $!";
  }

  $rules2rex->{$rule} = $rex;
  return $conflicts;
}

sub handle_lex {
    my ($pgram) = @_;
    my $count = 0;
    my $macros = {};
    my %rules2rex;

    code2lex("%{\n");
    code2lex("#include <stdio.h>\n");
    code2lex("#include \"gram.h\"\n");
    code2lex("#include \"${Gname}_defs.h\"\n");
    code2lex("#include \"${Gname}_parse.hh\"\n");
    code2lex("#undef YY_INPUT\n");
    code2lex("#define YY_INPUT(buf, retval, maxlen)	(retval = g_${Gname}->pi->switch_getinput(buf, maxlen))\n");
    code2lex("/* this is solely to avoid an annoying compiler warning */\n");
    code2lex("static void yyunput( int c, register char *yy_bp ) __attribute((unused));\n");
    code2lex("extern \"C\" int yywrap(void) {  return 1; }   \n");
    code2lex("%}\n");
    
    my $line;
    while ($line = shift (@$pgram) ) {
      if ($line =~ /^%LEX/) {	last;   }
    }

    ###### preamble #####
    while ($_=shift(@$pgram)) {
      chomp($_);
      code2lex("$_\n");
      if (/^%%/) { last;   }
      if (/^([a-zA-Z]\S*)[ \t]+(\S*)\s*$/) {
	my $name = $1;
	my $replacement = $2;
	die("Redefined $name as $replacement") if (exists $macros->{$name});
	my $inlined_replacement = inline_macro($macros, $replacement);
	$macros->{$name}=$inlined_replacement;
      } else {
	if (/^([a-zA-Z]\S*)/) {
	  die("Cannot parse lex definition of $1: $_");
	}
      }
    }

    ###### rules #####
    my $num_lrules = 0;
    code2def("    	/******** lexical rules *********/\n");
    while ($_=shift(@$pgram)) {
      chomp($_);
      if (/(^\S*)\s*(return.*;)/) {
	my $pattern = $1;
	my $inlined_pattern = inline_macro($macros, $pattern);
	#my $inlined_pattern = escape($inlined_pattern);
	my $pure_regex = escape_literal_strings($inlined_pattern);
	my $escaped_regex = escape_C_strings($pure_regex);
	
	my $returnStmt = $2;
	my $comment = ($pattern eq $escaped_regex )? "": "/*inlined $1*/";#bug:dangerous comment interaction with pattern
	
	my $extra = "";
	if ($returnStmt =~ /return\s+(\S*)\s*;/) {
	  my $token_and_size = $1;
	  my $size_limit = "";
	  # Can be "return ID;" or "return ID@3;"
	  if ($token_and_size =~ /([^@]*)(@([0-9]+))?/) {
	    if (defined $3) {
	      $size_limit = ", $3";
	      my $replace = ($returnStmt =~ s/@[0-9]+//);
	      die "bad limit :$replace:$returnStmt:" unless ($replace == 1);
	      print "ok $_:$replace:$returnStmt:";
	    }
	  } else {
	    die "bad token:$token_and_size";
	  }
	  if ($tokens{$1} == 1) {
	    my $token = $1;
	    my $rule = "lr$num_lrules"; ++$num_lrules;
	    
	    code2def("	LRule *$rule;//$pattern  <==> $escaped_regex\n");
	    code2init("  $token = G.get_nonterminal(\"$token\");\n");

	    my $conflicts = check_conflicts(\%rules2rex, $rule, $pure_regex);
	    if (defined $conflicts) {
	      $conflicts = ", \"/".escape_C_strings($conflicts)."/\"";
	    }
	    else {
	      $conflicts = "";
	    }
	    code2init("  $rule = new LRule($token, new rank_ffa_tab_t(\"/$escaped_regex/\"$conflicts), \"$rule\"$size_limit);//$_\n");
	    $extra = " yylval._ParseTree = new LRule::LParseTree(g_${Gname}->$rule, strndup(yytext, yyleng), yyleng);\n\t\t";
	  }
	} else {
	  die("Cannot understand the return statement: $returnStmt\n");
	}
	
	code2lex("$pattern \t{$extra $returnStmt} $comment\n");
      } else {
	code2lex("$_\n");
      }
      if (/^%%/) { last; }
    }

    code2init("\n}\n");
  }

sub handle_lex_postamble {
    my ($fplex, $pgram) = @_;
    my $count = 0;
    dbgprint("IN handle_lex_postamble");
    
    while ($_=shift(@$pgram)) {
	if (/^%YACC/) {
	    dbgprint("LAST handle_lex_postamble");
	    last;
	}
	chomp($_);
	lexout($fplex,"$_\n");
    }
    dbgprint("DONE handle_lex_postamble");
}

sub lexout{
    my ($fplex, $msg) = @_;
    print "LEX: $msg";
    print $fplex $msg;
}

#######################################################
#################### yacc part ########################
#######################################################
sub handle_yacc {
    my ($pgram) = @_;
    my $count = 0;

    my $line;
    while ($line = shift (@$pgram) ) {
      if ($line =~ /^%YACC/) {	last;   }
    }



    code2def("#ifndef _${Gname}_DEFS_\n");
    code2def("#define _${Gname}_DEFS_\n");
    code2def("#include \"rank_cfg.h\"\n");
    code2def("#include <string>\n");
    code2def("#include \"ffa.h\"\n");
    code2def("#define yyparse ${Gname}_parse\n");
    code2def("#define yylval ${Gname}_lval\n");
    code2def("extern int yyparse();\n");

    code2def("class $Gname : public rank_cfg {\n");
    code2def("  public:\n");

    undef $start;
    my $index_start;
    # preamble

    code2yacc("%{\n");
    code2yacc("#include \"gram.h\"\n");
    code2yacc("#include \"${Gname}_defs.h\"\n");
    code2yacc("${Gname} *g_${Gname} = NULL;\n"); #TBD: turn into ex2G::G
    code2yacc("int yylex();\n");
    code2yacc("extern \"C\" int yyerror(const char *s)\n");
    code2yacc("{printf(\"error: %s\", s); exit(-1);}\n");
    
    code2yacc("/* g++ requires these */\n");
    code2yacc("int ${Gname}lex();\n");
    code2yacc("%}\n");

    code2yacc("%union { ParseTree*  _ParseTree;}\n");
    while ($_=shift(@$pgram)) {
      chomp($_);
      if (/^%%/)             {  last; }
      if (/^%start\s*(\S*)/) {
	code2yacc("$_\n");
	$start = $1;
      }
      if (/^%token\s*(.*)/)  {
	my @artoken = split(/\s+/, $1);
	map {$tokens{$_} = 1} @artoken;

	code2yacc("%token <_ParseTree> $1\n");
      }
    }
    code2yacc("//PLACEHOLDER\n");
    my $ref_placeholder = \$code_yacc[$#code_yacc];
    code2yacc("%%\n"); 

    # -1- rules
    code2init("$Gname(unsigned int min_slice, unsigned int max_slice)  {\n\
  init_gram();\n\
  (const_cast<Nonterminal*>(G.S()))->memoize(min_slice, max_slice);\n\
  //memoize(min_slice, max_slice);\n\
}\n");
    code2init("void init_gram() {\n");
    code2init("  pi = new ParserInterface(${Gname}_parse);\n");
    my @rules = @$pgram;
    my $num_yrules = 0;
    my $left;
    while ($_=shift(@$pgram)) {
      chomp($_);
      if (/^%%/)             {   last;	}
      if (/^\s*(\S+)\s*:(.*)/ || /^\s*\|(.*)/) {
	my $right;
	if (/^\s*(\S+)\s*:(.*)/) {
	  $left = $1; $right = $2;
	} else {
	  if (/^\s*\|(.*)/) {
	    $right = $1; #same $left
	  } else {
	    die("Weird");
	  }
	}
	$start = $left if (! defined $start);
	$nonterminals{$left} = 1;
	push (@yrules, $_);

	my $rule = "yr$num_yrules";
	++$num_yrules;
	
	code2init("  //$_\n");
	code2init("  $left = G.get_nonterminal(\"$left\");\n");
	if($start eq $left && !defined $index_start) {
	  $index_start = $num_yrules-1;
	  code2init("  G.set_S($left);\n");
	}
	code2init("  $rule = new GRule($left,\"$rule\");\n");
	my @symbols = split(/\s+/, $right);
	my $sym;
	my $numSym=0;
	
	my $front="";
	$front = "g_${Gname}_ParseTree =" if ($start eq $left);

	code2yacc("$_ \n\t\t{ $front \$\$ = mkgptr(g_$Gname->$rule");
	foreach $sym (@symbols) {
	  next if ($sym eq "");
	  $numSym++;
	  if ($sym =~ /'(.*)'/) {
	    code2init("  $rule->append_symbol(G.get_terminal(\"$1\"));\n");
	  } else {
	    code2init("  $rule->append_symbol(G.get_nonterminal(\"$sym\"));\n");
	    code2yacc(", \$$numSym");
	  }
	}
	code2yacc(");}\n");
	code2init("\n");	
      }
    }
    code2yacc("%%\n");

    $$ref_placeholder = "%type  <_ParseTree> ".join(" ", keys(%nonterminals))."\n";
      
    #print "start= $start\n";
    #print "TOKENS= ",%tokens,"\n";
    #print "nonterminals= ",%nonterminals,"\n";
    #print "yrules= ",@yrules,"\n";

    
    code2def("    	/********  normal Nonterminals *********/\n");
    map {code2def("	Nonterminal *$_;\n") } (keys %nonterminals);

    code2def("    	/******** lexical Nonterminals *********/\n");
    map {code2def("	Nonterminal *$_;\n") } (keys %tokens);

    code2def("    	/********  normal rules *********/\n");
    for (my $yr = 0; $yr < $num_yrules; ++$yr) {
      code2def("	GRule *yr$yr;\n");
    }
  }
