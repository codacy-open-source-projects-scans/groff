/* Copyright (C) 1989-2024 Free Software Foundation, Inc.
     Written by James Clark (jjc@jclark.com)

This file is part of groff.

groff is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation, either version 3 of the License, or
(at your option) any later version.

groff is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>. */

#include "table.h"

#define BAR_HEIGHT ".25m"
#define DOUBLE_LINE_SEP "2p"
#define HALF_DOUBLE_LINE_SEP "1p"
#define LINE_SEP "2p"
#define BODY_DEPTH ".25m"

const int DEFAULT_COLUMN_SEPARATION = 3;

#define DELIMITER_CHAR "\\[tbl]"
#define SEPARATION_FACTOR_REG PREFIX "sep"
#define LEFTOVER_FACTOR_REG PREFIX "leftover"
#define BOTTOM_REG PREFIX "bot"
#define RESET_MACRO_NAME PREFIX "init"
#define LINESIZE_REG PREFIX "lps"
#define TOP_REG PREFIX "top"
#define CURRENT_ROW_REG PREFIX "crow"
#define LAST_PASSED_ROW_REG PREFIX "passed"
#define TRANSPARENT_STRING_NAME PREFIX "trans"
#define QUOTE_STRING_NAME PREFIX "quote"
#define SECTION_DIVERSION_NAME PREFIX "section"
#define SECTION_DIVERSION_FLAG_REG PREFIX "sflag"
#define SAVED_VERTICAL_POS_REG PREFIX "vert"
#define NEED_BOTTOM_RULE_REG PREFIX "brule"
#define USE_KEEPS_REG PREFIX "usekeeps"
#define KEEP_MACRO_NAME PREFIX "keep"
#define RELEASE_MACRO_NAME PREFIX "release"
#define SAVED_FONT_REG PREFIX "fnt"
#define SAVED_SIZE_REG PREFIX "sz"
#define SAVED_FILL_REG PREFIX "fll"
#define SAVED_INDENT_REG PREFIX "ind"
#define SAVED_CENTER_REG PREFIX "cent"
#define SAVED_TABS_NAME PREFIX "tabs"
#define SAVED_INTER_WORD_SPACE_SIZE PREFIX "ss"
#define SAVED_INTER_SENTENCE_SPACE_SIZE PREFIX "sss"
#define TABLE_DIVERSION_NAME PREFIX "table"
#define TABLE_DIVERSION_FLAG_REG PREFIX "tflag"
#define TABLE_KEEP_MACRO_NAME PREFIX "tkeep"
#define TABLE_RELEASE_MACRO_NAME PREFIX "trelease"
#define NEEDED_REG PREFIX "needed"
#define REPEATED_MARK_MACRO PREFIX "rmk"
#define REPEATED_VPT_MACRO PREFIX "rvpt"
#define TEXT_BLOCK_STAGGERING_MACRO PREFIX "sp"
#define SUPPRESS_BOTTOM_REG PREFIX "supbot"
#define SAVED_DN_REG PREFIX "dn"
#define SAVED_HYPHENATION_MODE_REG PREFIX "hyphmode"
#define SAVED_HYPHENATION_LANG_NAME PREFIX "hyphlang"
#define SAVED_HYPHENATION_MAX_LINES_REG PREFIX "hyphmaxlines"
#define SAVED_HYPHENATION_MARGIN_REG PREFIX "hyphmargin"
#define SAVED_HYPHENATION_SPACE_REG PREFIX "hyphspace"
#define SAVED_NUMBERING_LINENO PREFIX "linenumber"
#define SAVED_NUMBERING_ENABLED PREFIX "linenumberingenabled"
#define SAVED_NUMBERING_SUPPRESSION_COUNT PREFIX "linenumbersuppresscnt"
#define STARTING_PAGE_REG PREFIX "starting-page"
#define IS_BOXED_REG PREFIX "is-boxed"
#define PREVIOUS_PAGE_REG PREFIX "previous-page"

// this must be one character
#define COMPATIBLE_REG PREFIX "c"

// for use with `ig` requests embedded inside macro definitions
#define NOP_NAME PREFIX "nop"

#define AVAILABLE_WIDTH_REG PREFIX "available-width"
#define EXPAND_REG PREFIX "expansion-amount"

#define LEADER_REG PREFIX LEADER

#define BLOCK_WIDTH_PREFIX PREFIX "tbw"
#define BLOCK_DIVERSION_PREFIX PREFIX "tbd"
#define BLOCK_HEIGHT_PREFIX PREFIX "tbh"
#define SPAN_WIDTH_PREFIX PREFIX "w"
#define SPAN_LEFT_NUMERIC_WIDTH_PREFIX PREFIX "lnw"
#define SPAN_RIGHT_NUMERIC_WIDTH_PREFIX PREFIX "rnw"
#define SPAN_ALPHABETIC_WIDTH_PREFIX PREFIX "aw"
#define COLUMN_SEPARATION_PREFIX PREFIX "cs"
#define ROW_START_PREFIX PREFIX "rs"
#define COLUMN_START_PREFIX PREFIX "cl"
#define COLUMN_END_PREFIX PREFIX "ce"
#define COLUMN_DIVIDE_PREFIX PREFIX "cd"
#define ROW_TOP_PREFIX PREFIX "rt"

string block_width_reg(int, int);
string block_diversion_name(int, int);
string block_height_reg(int, int);
string span_width_reg(int, int);
string span_left_numeric_width_reg(int, int);
string span_right_numeric_width_reg(int, int);
string span_alphabetic_width_reg(int, int);
string column_separation_reg(int);
string row_start_reg(int);
string column_start_reg(int);
string column_end_reg(int);
string column_divide_reg(int);
string row_top_reg(int);

void set_inline_modifier(const entry_modifier *);
void restore_inline_modifier(const entry_modifier *);
void set_modifier(const entry_modifier *);
int find_decimal_point(const char *, char, const char *);

string an_empty_string;
int location_force_filename = 0;

void printfs(const char *,
	     const string &arg1 = an_empty_string,
	     const string &arg2 = an_empty_string,
	     const string &arg3 = an_empty_string,
	     const string &arg4 = an_empty_string,
	     const string &arg5 = an_empty_string);

void prints(const string &);

inline void prints(char c)
{
  putchar(c);
}

inline void prints(const char *s)
{
  fputs(s, stdout);
}

void prints(const string &s)
{
  if (!s.empty())
    fwrite(s.contents(), 1, s.length(), stdout);
}

struct horizontal_span {
  horizontal_span *next;
  int start_col;
  int end_col;
  horizontal_span(int, int, horizontal_span *);
};

class single_line_entry;
class double_line_entry;
class simple_entry;

class table_entry {
friend class table;
  table_entry *next;
  int input_lineno;
  const char *input_filename;
protected:
  int start_row;
  int end_row;
  int start_col;
  int end_col;
  const table *parent;
  const entry_modifier *mod;
public:
  void set_location();
  table_entry(const table *, const entry_modifier *);
  virtual ~table_entry();
  virtual int divert(int, const string *, int *, int);
  virtual void do_width();
  virtual void do_depth();
  virtual void print() = 0;
  virtual void position_vertically() = 0;
  virtual single_line_entry *to_single_line_entry();
  virtual double_line_entry *to_double_line_entry();
  virtual simple_entry *to_simple_entry();
  virtual int line_type();
  virtual void note_double_vrule_on_right(int);
  virtual void note_double_vrule_on_left(int);
};

class simple_entry : public table_entry {
public:
  simple_entry(const table *, const entry_modifier *);
  void print();
  void position_vertically();
  simple_entry *to_simple_entry();
  virtual void add_tab();
  virtual void simple_print(int);
};

class empty_entry : public simple_entry {
public:
  empty_entry(const table *, const entry_modifier *);
  int line_type();
};

class text_entry : public simple_entry {
protected:
  char *contents;
  void print_contents();
public:
  text_entry(const table *, const entry_modifier *, char *);
  ~text_entry();
};

void text_entry::print_contents()
{
  set_inline_modifier(mod);
  prints(contents);
  restore_inline_modifier(mod);
}

class repeated_char_entry : public text_entry {
public:
  repeated_char_entry(const table *, const entry_modifier *, char *);
  void simple_print(int);
};

class simple_text_entry : public text_entry {
public:
  simple_text_entry(const table *, const entry_modifier *, char *);
  void do_width();
};

class left_text_entry : public simple_text_entry {
public:
  left_text_entry(const table *, const entry_modifier *, char *);
  void simple_print(int);
  void add_tab();
};

class right_text_entry : public simple_text_entry {
public:
  right_text_entry(const table *, const entry_modifier *, char *);
  void simple_print(int);
  void add_tab();
};

class center_text_entry : public simple_text_entry {
public:
  center_text_entry(const table *, const entry_modifier *, char *);
  void simple_print(int);
  void add_tab();
};

class numeric_text_entry : public text_entry {
  int dot_pos;
public:
  numeric_text_entry(const table *, const entry_modifier *, char *, int);
  void do_width();
  void simple_print(int);
};

class alphabetic_text_entry : public text_entry {
public:
  alphabetic_text_entry(const table *, const entry_modifier *, char *);
  void do_width();
  void simple_print(int);
  void add_tab();
};

class line_entry : public simple_entry {
protected:
  char double_vrule_on_right;
  char double_vrule_on_left;
public:
  line_entry(const table *, const entry_modifier *);
  void note_double_vrule_on_right(int);
  void note_double_vrule_on_left(int);
  void simple_print(int) = 0;
};

class single_line_entry : public line_entry {
public:
  single_line_entry(const table *, const entry_modifier *);
  void simple_print(int);
  single_line_entry *to_single_line_entry();
  int line_type();
};

class double_line_entry : public line_entry {
public:
  double_line_entry(const table *, const entry_modifier *);
  void simple_print(int);
  double_line_entry *to_double_line_entry();
  int line_type();
};

class short_line_entry : public simple_entry {
public:
  short_line_entry(const table *, const entry_modifier *);
  void simple_print(int);
  int line_type();
};

class short_double_line_entry : public simple_entry {
public:
  short_double_line_entry(const table *, const entry_modifier *);
  void simple_print(int);
  int line_type();
};

class block_entry : public table_entry {
  char *contents;
protected:
  void do_divert(int, int, const string *, int *, int);
public:
  block_entry(const table *, const entry_modifier *, char *);
  ~block_entry();
  int divert(int, const string *, int *, int);
  void do_depth();
  void position_vertically();
  void print() = 0;
};

class left_block_entry : public block_entry {
public:
  left_block_entry(const table *, const entry_modifier *, char *);
  void print();
};

class right_block_entry : public block_entry {
public:
  right_block_entry(const table *, const entry_modifier *, char *);
  void print();
};

class center_block_entry : public block_entry {
public:
  center_block_entry(const table *, const entry_modifier *, char *);
  void print();
};

class alphabetic_block_entry : public block_entry {
public:
  alphabetic_block_entry(const table *, const entry_modifier *, char *);
  void print();
  int divert(int, const string *, int *, int);
};

table_entry::table_entry(const table *p, const entry_modifier *m)
: next(0), input_lineno(-1), input_filename(0),
  start_row(-1), end_row(-1), start_col(-1), end_col(-1), parent(p), mod(m)
{
}

table_entry::~table_entry()
{
}

int table_entry::divert(int, const string *, int *, int)
{
  return 0;
}

void table_entry::do_width()
{
}

single_line_entry *table_entry::to_single_line_entry()
{
  return 0;
}

double_line_entry *table_entry::to_double_line_entry()
{
  return 0;
}

simple_entry *table_entry::to_simple_entry()
{
  return 0;
}

void table_entry::do_depth()
{
}

void table_entry::set_location()
{
  set_troff_location(input_filename, input_lineno);
}

int table_entry::line_type()
{
  return -1;
}

void table_entry::note_double_vrule_on_right(int)
{
}

void table_entry::note_double_vrule_on_left(int)
{
}

simple_entry::simple_entry(const table *p, const entry_modifier *m)
: table_entry(p, m)
{
}

void simple_entry::add_tab()
{
  // do nothing
}

void simple_entry::simple_print(int)
{
  // do nothing
}

void simple_entry::position_vertically()
{
  if (start_row != end_row)
    switch (mod->vertical_alignment) {
    case entry_modifier::TOP:
      printfs(".sp |\\n[%1]u\n", row_start_reg(start_row));
      break;
    case entry_modifier::CENTER:
      // Perform the motion in two stages so that the center is rounded
      // vertically upward even if net vertical motion is upward.
      printfs(".sp |\\n[%1]u\n", row_start_reg(start_row));
      printfs(".sp \\n[" BOTTOM_REG "]u-\\n[%1]u-1v/2u\n",
	      row_start_reg(start_row));
      break;
    case entry_modifier::BOTTOM:
      printfs(".sp |\\n[%1]u+\\n[" BOTTOM_REG "]u-\\n[%1]u-1v\n",
	      row_start_reg(start_row));
      break;
    default:
      assert(0 == "simple entry vertical position modifier not TOP,"
		  " CENTER, or BOTTOM");
    }
}

void simple_entry::print()
{
  prints(".ta");
  add_tab();
  prints('\n');
  set_location();
  prints("\\&");
  simple_print(0);
  prints('\n');
}

simple_entry *simple_entry::to_simple_entry()
{
  return this;
}

empty_entry::empty_entry(const table *p, const entry_modifier *m)
: simple_entry(p, m)
{
}

int empty_entry::line_type()
{
  return 0;
}

text_entry::text_entry(const table *p, const entry_modifier *m, char *s)
: simple_entry(p, m), contents(s)
{
}

text_entry::~text_entry()
{
  free(contents);
}

repeated_char_entry::repeated_char_entry(const table *p,
					 const entry_modifier *m, char *s)
: text_entry(p, m, s)
{
}

void repeated_char_entry::simple_print(int)
{
  printfs("\\h'|\\n[%1]u'", column_start_reg(start_col));
  set_inline_modifier(mod);
  printfs("\\l" DELIMITER_CHAR "\\n[%1]u\\&",
	  span_width_reg(start_col, end_col));
  prints(contents);
  prints(DELIMITER_CHAR);
  restore_inline_modifier(mod);
}

simple_text_entry::simple_text_entry(const table *p,
				     const entry_modifier *m, char *s)
: text_entry(p, m, s)
{
}

void simple_text_entry::do_width()
{
  set_location();
  printfs(".nr %1 \\n[%1]>?\\w" DELIMITER_CHAR,
	  span_width_reg(start_col, end_col));
  print_contents();
  prints(DELIMITER_CHAR "\n");
}

left_text_entry::left_text_entry(const table *p,
				 const entry_modifier *m, char *s)
: simple_text_entry(p, m, s)
{
}

void left_text_entry::simple_print(int)
{
  printfs("\\h'|\\n[%1]u'", column_start_reg(start_col));
  print_contents();
}

// The only point of this is to make '\a' "work" as in Unix tbl.  Grrr.

void left_text_entry::add_tab()
{
  printfs(" \\n[%1]u", column_end_reg(end_col));
}

right_text_entry::right_text_entry(const table *p,
				   const entry_modifier *m, char *s)
: simple_text_entry(p, m, s)
{
}

void right_text_entry::simple_print(int)
{
  printfs("\\h'|\\n[%1]u'", column_start_reg(start_col));
  prints("\002\003");
  print_contents();
  prints("\002");
}

void right_text_entry::add_tab()
{
  printfs(" \\n[%1]u", column_end_reg(end_col));
}

center_text_entry::center_text_entry(const table *p,
				     const entry_modifier *m, char *s)
: simple_text_entry(p, m, s)
{
}

void center_text_entry::simple_print(int)
{
  printfs("\\h'|\\n[%1]u'", column_start_reg(start_col));
  prints("\002\003");
  print_contents();
  prints("\003\002");
}

void center_text_entry::add_tab()
{
  printfs(" \\n[%1]u", column_end_reg(end_col));
}

numeric_text_entry::numeric_text_entry(const table *p,
				       const entry_modifier *m,
				       char *s, int pos)
: text_entry(p, m, s), dot_pos(pos)
{
}

void numeric_text_entry::do_width()
{
  if (dot_pos != 0) {
    set_location();
    printfs(".nr %1 0\\w" DELIMITER_CHAR,
	    block_width_reg(start_row, start_col));
    set_inline_modifier(mod);
    for (int i = 0; i < dot_pos; i++)
      prints(contents[i]);
    restore_inline_modifier(mod);
    prints(DELIMITER_CHAR "\n");
    printfs(".nr %1 \\n[%1]>?\\n[%2]\n",
	    span_left_numeric_width_reg(start_col, end_col),
	    block_width_reg(start_row, start_col));
  }
  else
    printfs(".nr %1 0\n", block_width_reg(start_row, start_col));
  if (contents[dot_pos] != '\0') {
    set_location();
    printfs(".nr %1 \\n[%1]>?\\w" DELIMITER_CHAR,
	    span_right_numeric_width_reg(start_col, end_col));
    set_inline_modifier(mod);
    prints(contents + dot_pos);
    restore_inline_modifier(mod);
    prints(DELIMITER_CHAR "\n");
  }
}

void numeric_text_entry::simple_print(int)
{
  printfs("\\h'|(\\n[%1]u-\\n[%2]u-\\n[%3]u/2u+\\n[%2]u+\\n[%4]u-\\n[%5]u)'",
	  span_width_reg(start_col, end_col),
	  span_left_numeric_width_reg(start_col, end_col),
	  span_right_numeric_width_reg(start_col, end_col),
	  column_start_reg(start_col),
	  block_width_reg(start_row, start_col));
  print_contents();
}

alphabetic_text_entry::alphabetic_text_entry(const table *p,
					     const entry_modifier *m,
					     char *s)
: text_entry(p, m, s)
{
}

void alphabetic_text_entry::do_width()
{
  set_location();
  printfs(".nr %1 \\n[%1]>?\\w" DELIMITER_CHAR,
	  span_alphabetic_width_reg(start_col, end_col));
  print_contents();
  prints(DELIMITER_CHAR "\n");
}

void alphabetic_text_entry::simple_print(int)
{
  printfs("\\h'|\\n[%1]u'", column_start_reg(start_col));
  printfs("\\h'\\n[%1]u-\\n[%2]u/2u'",
	  span_width_reg(start_col, end_col),
	  span_alphabetic_width_reg(start_col, end_col));
  print_contents();
}

// The only point of this is to make '\a' "work" as in Unix tbl.  Grrr.

void alphabetic_text_entry::add_tab()
{
  printfs(" \\n[%1]u", column_end_reg(end_col));
}

block_entry::block_entry(const table *p, const entry_modifier *m, char *s)
: table_entry(p, m), contents(s)
{
}

block_entry::~block_entry()
{
  delete[] contents;
}

void block_entry::position_vertically()
{
  if (start_row != end_row)
    switch(mod->vertical_alignment) {
    case entry_modifier::TOP:
      printfs(".sp |\\n[%1]u\n", row_start_reg(start_row));
      break;
    case entry_modifier::CENTER:
      // Perform the motion in two stages so that the center is rounded
      // vertically upward even if net vertical motion is upward.
      printfs(".sp |\\n[%1]u\n", row_start_reg(start_row));
      printfs(".sp \\n[" BOTTOM_REG "]u-\\n[%1]u-\\n[%2]u/2u\n",
	      row_start_reg(start_row),
	      block_height_reg(start_row, start_col));
      break;
    case entry_modifier::BOTTOM:
      printfs(".sp |\\n[%1]u+\\n[" BOTTOM_REG "]u-\\n[%1]u-\\n[%2]u\n",
	      row_start_reg(start_row),
	      block_height_reg(start_row, start_col));
      break;
    default:
      assert(0 == "block entry vertical position modifier not TOP,"
		  " CENTER, or BOTTOM");
    }
  if (mod->stagger)
    prints("." TEXT_BLOCK_STAGGERING_MACRO " -.5v\n");
}

int block_entry::divert(int ncols, const string *mw, int *sep, int do_expand)
{
  do_divert(0, ncols, mw, sep, do_expand);
  return 1;
}

void block_entry::do_divert(int alphabetic, int ncols, const string *mw,
			    int *sep, int do_expand)
{
  int i;
  for (i = start_col; i <= end_col; i++)
    if (parent->expand[i])
      break;
  if (i > end_col) {
    if (do_expand)
      return;
  }
  else {
    if (!do_expand)
      return;
  }
  printfs(".di %1\n", block_diversion_name(start_row, start_col));
  prints(".if \\n[" SAVED_FILL_REG "] .fi\n"
	 ".in 0\n");
  prints(".ll ");
  for (i = start_col; i <= end_col; i++)
    if (mw[i].empty() && !parent->expand[i])
      break;
  if (i > end_col) {
    // Every column spanned by this entry has a minimum width.
    for (int j = start_col; j <= end_col; j++) {
      if (j > start_col) {
	if (sep)
	  printfs("+%1n", as_string(sep[j - 1]));
	prints('+');
      }
      if (parent->expand[j])
	prints("\\n[" EXPAND_REG "]u");
      else
	printfs("(n;%1)", mw[j]);
    }
    printfs(">?\\n[%1]u", span_width_reg(start_col, end_col));
  }
  else
    // Assign each column with a block entry 1/(n+1) of the line
    // width, where n is the column count.
    printfs("(u;\\n[%1]>?(\\n[.l]*%2/%3))",
	    span_width_reg(start_col, end_col),
	    as_string(end_col - start_col + 1),
	    as_string(ncols + 1));
  if (alphabetic)
    prints("-2n");
  prints("\n");
  prints(".ss \\n[" SAVED_INTER_WORD_SPACE_SIZE "]"
      " \\n[" SAVED_INTER_SENTENCE_SPACE_SIZE "]\n");
  prints(".cp \\n(" COMPATIBLE_REG "\n");
  set_modifier(mod);
  set_location();
  prints(contents);
  prints(".br\n.di\n.cp 0\n");
  if (!mod->zero_width) {
    if (alphabetic) {
      printfs(".nr %1 \\n[%1]>?(\\n[dl]+2n)\n",
	      span_width_reg(start_col, end_col));
      printfs(".nr %1 \\n[%1]>?\\n[dl]\n",
	      span_alphabetic_width_reg(start_col, end_col));
    }
    else
      printfs(".nr %1 \\n[%1]>?\\n[dl]\n",
	      span_width_reg(start_col, end_col));
  }
  printfs(".nr %1 \\n[dn]\n", block_height_reg(start_row, start_col));
  printfs(".nr %1 \\n[dl]\n", block_width_reg(start_row, start_col));
  prints("." RESET_MACRO_NAME "\n"
	 ".in \\n[" SAVED_INDENT_REG "]u\n"
	 ".nf\n");
  // the block might have contained .lf commands
  location_force_filename = 1;
}

void block_entry::do_depth()
{
  printfs(".nr " BOTTOM_REG " \\n[" BOTTOM_REG "]>?(\\n[%1]+\\n[%2])\n",
	  row_start_reg(start_row),
	  block_height_reg(start_row, start_col));
}

left_block_entry::left_block_entry(const table *p,
				   const entry_modifier *m, char *s)
: block_entry(p, m, s)
{
}

void left_block_entry::print()
{
  printfs(".in +\\n[%1]u\n", column_start_reg(start_col));
  printfs(".%1\n", block_diversion_name(start_row, start_col));
  if (mod->stagger)
    prints("." TEXT_BLOCK_STAGGERING_MACRO " .5v\n");
  prints(".in\n");
}

right_block_entry::right_block_entry(const table *p,
				     const entry_modifier *m, char *s)
: block_entry(p, m, s)
{
}

void right_block_entry::print()
{
  printfs(".in +\\n[%1]u+\\n[%2]u-\\n[%3]u\n",
	  column_start_reg(start_col),
	  span_width_reg(start_col, end_col),
	  block_width_reg(start_row, start_col));
  printfs(".%1\n", block_diversion_name(start_row, start_col));
  if (mod->stagger)
    prints("." TEXT_BLOCK_STAGGERING_MACRO " .5v\n");
  prints(".in\n");
}

center_block_entry::center_block_entry(const table *p,
				       const entry_modifier *m, char *s)
: block_entry(p, m, s)
{
}

void center_block_entry::print()
{
  printfs(".in +\\n[%1]u+(\\n[%2]u-\\n[%3]u/2u)\n",
	  column_start_reg(start_col),
	  span_width_reg(start_col, end_col),
	  block_width_reg(start_row, start_col));
  printfs(".%1\n", block_diversion_name(start_row, start_col));
  if (mod->stagger)
    prints("." TEXT_BLOCK_STAGGERING_MACRO " .5v\n");
  prints(".in\n");
}

alphabetic_block_entry::alphabetic_block_entry(const table *p,
					       const entry_modifier *m,
					       char *s)
: block_entry(p, m, s)
{
}

int alphabetic_block_entry::divert(int ncols, const string *mw, int *sep,
				   int do_expand)
{
  do_divert(1, ncols, mw, sep, do_expand);
  return 1;
}

void alphabetic_block_entry::print()
{
  printfs(".in +\\n[%1]u+(\\n[%2]u-\\n[%3]u/2u)\n",
	  column_start_reg(start_col),
	  span_width_reg(start_col, end_col),
	  span_alphabetic_width_reg(start_col, end_col));
  printfs(".%1\n", block_diversion_name(start_row, start_col));
  if (mod->stagger)
    prints("." TEXT_BLOCK_STAGGERING_MACRO " .5v\n");
  prints(".in\n");
}

line_entry::line_entry(const table *p, const entry_modifier *m)
: simple_entry(p, m), double_vrule_on_right(0), double_vrule_on_left(0)
{
}

void line_entry::note_double_vrule_on_right(int is_corner)
{
  double_vrule_on_right = is_corner ? 1 : 2;
}

void line_entry::note_double_vrule_on_left(int is_corner)
{
  double_vrule_on_left = is_corner ? 1 : 2;
}

single_line_entry::single_line_entry(const table *p, const entry_modifier *m)
: line_entry(p, m)
{
}

int single_line_entry::line_type()
{
  return 1;
}

void single_line_entry::simple_print(int dont_move)
{
  printfs("\\h'|\\n[%1]u",
	  column_divide_reg(start_col));
  if (double_vrule_on_left) {
    prints(double_vrule_on_left == 1 ? "-" : "+");
    prints(HALF_DOUBLE_LINE_SEP);
  }
  prints("'");
  if (!dont_move)
    prints("\\v'-" BAR_HEIGHT "'");
  printfs("\\s[\\n[" LINESIZE_REG "]]" "\\D'l |\\n[%1]u",
	  column_divide_reg(end_col+1));
  if (double_vrule_on_right) {
    prints(double_vrule_on_left == 1 ? "+" : "-");
    prints(HALF_DOUBLE_LINE_SEP);
  }
  prints("0'\\s0");
  if (!dont_move)
    prints("\\v'" BAR_HEIGHT "'");
}

single_line_entry *single_line_entry::to_single_line_entry()
{
  return this;
}

double_line_entry::double_line_entry(const table *p,
				     const entry_modifier *m)
: line_entry(p, m)
{
}

int double_line_entry::line_type()
{
  return 2;
}

void double_line_entry::simple_print(int dont_move)
{
  if (!dont_move)
    prints("\\v'-" BAR_HEIGHT "'");
  printfs("\\h'|\\n[%1]u",
	  column_divide_reg(start_col));
  if (double_vrule_on_left) {
    prints(double_vrule_on_left == 1 ? "-" : "+");
    prints(HALF_DOUBLE_LINE_SEP);
  }
  prints("'");
  printfs("\\v'-" HALF_DOUBLE_LINE_SEP "'"
	  "\\s[\\n[" LINESIZE_REG "]]"
	  "\\D'l |\\n[%1]u",
	  column_divide_reg(end_col+1));
  if (double_vrule_on_right)
    prints("-" HALF_DOUBLE_LINE_SEP);
  prints(" 0'");
  printfs("\\v'" DOUBLE_LINE_SEP "'"
	  "\\D'l |\\n[%1]u",
	  column_divide_reg(start_col));
  if (double_vrule_on_right) {
    prints(double_vrule_on_left == 1 ? "+" : "-");
    prints(HALF_DOUBLE_LINE_SEP);
  }
  prints(" 0'");
  prints("\\s0"
	 "\\v'-" HALF_DOUBLE_LINE_SEP "'");
  if (!dont_move)
    prints("\\v'" BAR_HEIGHT "'");
}

double_line_entry *double_line_entry::to_double_line_entry()
{
  return this;
}

short_line_entry::short_line_entry(const table *p, const entry_modifier *m)
: simple_entry(p, m)
{
}

int short_line_entry::line_type()
{
  return 1;
}

void short_line_entry::simple_print(int dont_move)
{
  if (mod->stagger)
    prints("\\v'-.5v'");
  if (!dont_move)
    prints("\\v'-" BAR_HEIGHT "'");
  printfs("\\h'|\\n[%1]u'", column_start_reg(start_col));
  printfs("\\s[\\n[" LINESIZE_REG "]]"
	  "\\D'l \\n[%1]u 0'"
	  "\\s0",
	  span_width_reg(start_col, end_col));
  if (!dont_move)
    prints("\\v'" BAR_HEIGHT "'");
  if (mod->stagger)
    prints("\\v'.5v'");
}

short_double_line_entry::short_double_line_entry(const table *p,
						 const entry_modifier *m)
: simple_entry(p, m)
{
}

int short_double_line_entry::line_type()
{
  return 2;
}

void short_double_line_entry::simple_print(int dont_move)
{
  if (mod->stagger)
    prints("\\v'-.5v'");
  if (!dont_move)
    prints("\\v'-" BAR_HEIGHT "'");
  printfs("\\h'|\\n[%2]u'"
	  "\\v'-" HALF_DOUBLE_LINE_SEP "'"
	  "\\s[\\n[" LINESIZE_REG "]]"
	  "\\D'l \\n[%1]u 0'"
	  "\\v'" DOUBLE_LINE_SEP "'"
	  "\\D'l |\\n[%2]u 0'"
	  "\\s0"
	  "\\v'-" HALF_DOUBLE_LINE_SEP "'",
	  span_width_reg(start_col, end_col),
	  column_start_reg(start_col));
  if (!dont_move)
    prints("\\v'" BAR_HEIGHT "'");
  if (mod->stagger)
    prints("\\v'.5v'");
}

void set_modifier(const entry_modifier *m)
{
  if (!m->font.empty())
    printfs(".ft %1\n", m->font);
  if (m->type_size.whole != 0) {
    prints(".ps ");
    if (m->type_size.relativity == size_expression::INCREMENT)
      prints('+');
    else if (m->type_size.relativity == size_expression::DECREMENT)
      prints('-');
    printfs("%1\n", as_string(m->type_size.whole));
  }
  if (m->vertical_spacing.whole != 0) {
    prints(".vs ");
    if (m->vertical_spacing.relativity == size_expression::INCREMENT)
      prints('+');
    else if (m->vertical_spacing.relativity
	     == size_expression::DECREMENT)
      prints('-');
    printfs("%1\n", as_string(m->vertical_spacing.whole));
  }
  if (!m->macro.empty())
    printfs(".%1\n", m->macro);
}

void set_inline_modifier(const entry_modifier *m)
{
  if (!m->font.empty())
    printfs("\\f[%1]", m->font);
  if (m->type_size.whole != 0) {
    prints("\\s[");
    if (m->type_size.relativity == size_expression::INCREMENT)
      prints('+');
    else if (m->type_size.relativity == size_expression::DECREMENT)
      prints('-');
    printfs("%1]", as_string(m->type_size.whole));
  }
  if (m->stagger)
    prints("\\v'-.5v'");
}

void restore_inline_modifier(const entry_modifier *m)
{
  if (!m->font.empty())
    prints("\\f[\\n[" SAVED_FONT_REG "]]");
  if (m->type_size.whole != 0)
    prints("\\s[\\n[" SAVED_SIZE_REG "]]");
  if (m->stagger)
    prints("\\v'.5v'");
}

struct stuff {
  stuff *next;
  int row;			// occurs before row 'row'
  char printed;			// has it been printed?

  stuff(int);
  virtual void print(table *) = 0;
  virtual ~stuff();
  virtual int is_single_line() { return 0; };
  virtual int is_double_line() { return 0; };
};

stuff::stuff(int r) : next(0), row(r), printed(0)
{
}

stuff::~stuff()
{
}

struct text_stuff : public stuff {
  string contents;
  const char *filename;
  int lineno;

  text_stuff(const string &, int, const char *, int);
  ~text_stuff();
  void print(table *);
};

text_stuff::text_stuff(const string &s, int r, const char *fn, int ln)
: stuff(r), contents(s), filename(fn), lineno(ln)
{
}

text_stuff::~text_stuff()
{
}

void text_stuff::print(table *)
{
  printed = 1;
  prints(".cp \\n(" COMPATIBLE_REG "\n");
  set_troff_location(filename, lineno);
  prints(contents);
  prints(".cp 0\n");
  location_force_filename = 1;	// it might have been a .lf command
}

struct single_hrule_stuff : public stuff {
  single_hrule_stuff(int);
  void print(table *);
  int is_single_line();
};

single_hrule_stuff::single_hrule_stuff(int r) : stuff(r)
{
}

void single_hrule_stuff::print(table *tbl)
{
  printed = 1;
  tbl->print_single_hrule(row);
}

int single_hrule_stuff::is_single_line()
{
  return 1;
}

struct double_hrule_stuff : stuff {
  double_hrule_stuff(int);
  void print(table *);
  int is_double_line();
};

double_hrule_stuff::double_hrule_stuff(int r) : stuff(r)
{
}

void double_hrule_stuff::print(table *tbl)
{
  printed = 1;
  tbl->print_double_hrule(row);
}

int double_hrule_stuff::is_double_line()
{
  return 1;
}

struct vertical_rule {
  vertical_rule *next;
  int start_row;
  int end_row;
  int col;
  char is_double;
  string top_adjust;
  string bot_adjust;

  vertical_rule(int, int, int, int, vertical_rule *);
  ~vertical_rule();
  void contribute_to_bottom_macro(table *);
  void print();
};

vertical_rule::vertical_rule(int sr, int er, int c, int dbl,
			     vertical_rule *p)
: next(p), start_row(sr), end_row(er), col(c), is_double(dbl)
{
}

vertical_rule::~vertical_rule()
{
}

void vertical_rule::contribute_to_bottom_macro(table *tbl)
{
  printfs(".if \\n[" CURRENT_ROW_REG "]>=%1",
	  as_string(start_row));
  if (end_row != tbl->get_nrows() - 1)
    printfs("&(\\n[" CURRENT_ROW_REG "]<%1)",
	    as_string(end_row));
  prints(" \\{\\\n");
  printfs(".  if %1<=\\n[" LAST_PASSED_ROW_REG "] .nr %2 \\n[#T]\n",
	  as_string(start_row),
	  row_top_reg(start_row));
  const char *offset_table[3];
  if (is_double) {
    offset_table[0] = "-" HALF_DOUBLE_LINE_SEP;
    offset_table[1] = "+" HALF_DOUBLE_LINE_SEP;
    offset_table[2] = 0;
  }
  else {
    offset_table[0] = "";
    offset_table[1] = 0;
  }
  for (const char **offsetp = offset_table; *offsetp; offsetp++) {
    prints(".  sp -1\n"
	   "\\v'" BODY_DEPTH);
    if (!bot_adjust.empty())
      printfs("+%1", bot_adjust);
    prints("'");
    printfs("\\h'\\n[%1]u%3'\\s[\\n[" LINESIZE_REG "]]\\D'l 0 |\\n[%2]u-1v",
	    column_divide_reg(col),
	    row_top_reg(start_row),
	    *offsetp);
    if (!bot_adjust.empty())
      printfs("-(%1)", bot_adjust);
    // don't perform the top adjustment if the top is actually #T
    if (!top_adjust.empty())
      printfs("+((%1)*(%2>\\n[" LAST_PASSED_ROW_REG "]))",
	      top_adjust,
	      as_string(start_row));
    prints("'\\s0\n");
  }
  prints(".\\}\n");
}

void vertical_rule::print()
{
  printfs("\\*[" TRANSPARENT_STRING_NAME "]"
	  ".if %1<=\\*[" QUOTE_STRING_NAME "]\\n[" LAST_PASSED_ROW_REG "] "
	  ".nr %2 \\*[" QUOTE_STRING_NAME "]\\n[#T]\n",
	  as_string(start_row),
	  row_top_reg(start_row));
  const char *offset_table[3];
  if (is_double) {
    offset_table[0] = "-" HALF_DOUBLE_LINE_SEP;
    offset_table[1] = "+" HALF_DOUBLE_LINE_SEP;
    offset_table[2] = 0;
  }
  else {
    offset_table[0] = "";
    offset_table[1] = 0;
  }
  for (const char **offsetp = offset_table; *offsetp; offsetp++) {
    prints("\\*[" TRANSPARENT_STRING_NAME "].sp -1\n"
	   "\\*[" TRANSPARENT_STRING_NAME "]\\v'" BODY_DEPTH);
    if (!bot_adjust.empty())
      printfs("+%1", bot_adjust);
    prints("'");
    printfs("\\h'\\n[%1]u%3'"
	    "\\s[\\n[" LINESIZE_REG "]]"
	    "\\D'l 0 |\\*[" QUOTE_STRING_NAME "]\\n[%2]u-1v",
	    column_divide_reg(col),
	    row_top_reg(start_row),
	    *offsetp);
    if (!bot_adjust.empty())
      printfs("-(%1)", bot_adjust);
    // don't perform the top adjustment if the top is actually #T
    if (!top_adjust.empty())
      printfs("+((%1)*(%2>\\*[" QUOTE_STRING_NAME "]\\n["
	      LAST_PASSED_ROW_REG "]))",
	      top_adjust,
	      as_string(start_row));
    prints("'"
	   "\\s0\n");
  }
}

table::table(int nc, unsigned f, int ls, char dpc)
: nrows(0), ncolumns(nc), linesize(ls), decimal_point_char(dpc),
  vrule_list(0), stuff_list(0), span_list(0),
  entry_list(0), entry_list_tailp(&entry_list), entry(0),
  vrule(0), row_is_all_lines(0), left_separation(0),
  right_separation(0), total_separation(0), allocated_rows(0), flags(f)
{
  minimum_width = new string[ncolumns];
  column_separation = ncolumns > 1 ? new int[ncolumns - 1] : 0;
  equal = new char[ncolumns];
  expand = new char[ncolumns];
  int i;
  for (i = 0; i < ncolumns; i++) {
    equal[i] = 0;
    expand[i] = 0;
  }
  for (i = 0; i < ncolumns - 1; i++)
    column_separation[i] = DEFAULT_COLUMN_SEPARATION;
  delim[0] = delim[1] = '\0';
}

table::~table()
{
  for (int i = 0; i < nrows; i++) {
    delete[] entry[i];
    delete[] vrule[i];
  }
  delete[] entry;
  delete[] vrule;
  while (entry_list) {
    table_entry *tem = entry_list;
    entry_list = entry_list->next;
    delete tem;
  }
  delete[] minimum_width;
  delete[] column_separation;
  delete[] equal;
  delete[] expand;
  while (stuff_list) {
    stuff *tem = stuff_list;
    stuff_list = stuff_list->next;
    delete tem;
  }
  while (vrule_list) {
    vertical_rule *tem = vrule_list;
    vrule_list = vrule_list->next;
    delete tem;
  }
  delete[] row_is_all_lines;
  while (span_list) {
    horizontal_span *tem = span_list;
    span_list = span_list->next;
    delete tem;
  }
}

void table::set_delim(char c1, char c2)
{
  delim[0] = c1;
  delim[1] = c2;
}

void table::set_minimum_width(int c, const string &w)
{
  assert(c >= 0 && c < ncolumns);
  minimum_width[c] = w;
}

void table::set_column_separation(int c, int n)
{
  assert(c >= 0 && c < ncolumns - 1);
  column_separation[c] = n;
}

void table::set_equal_column(int c)
{
  assert(c >= 0 && c < ncolumns);
  equal[c] = 1;
}

void table::set_expand_column(int c)
{
  assert(c >= 0 && c < ncolumns);
  expand[c] = 1;
}

void table::add_stuff(stuff *p)
{
  stuff **pp;
  for (pp = &stuff_list; *pp; pp = &(*pp)->next)
    ;
  *pp = p;
}

void table::add_text_line(int r, const string &s, const char *filename,
			  int lineno)
{
  add_stuff(new text_stuff(s, r, filename, lineno));
}

void table::add_single_hrule(int r)
{
  add_stuff(new single_hrule_stuff(r));
}

void table::add_double_hrule(int r)
{
  add_stuff(new double_hrule_stuff(r));
}

void table::allocate(int r)
{
  if (r >= nrows) {
    typedef table_entry **PPtable_entry; // work around g++ 1.36.1 bug
    if (r >= allocated_rows) {
      if (allocated_rows == 0) {
	allocated_rows = 16;
	if (allocated_rows <= r)
	  allocated_rows = r + 1;
	entry = new PPtable_entry[allocated_rows];
	vrule = new char*[allocated_rows];
      }
      else {
	table_entry ***old_entry = entry;
	int old_allocated_rows = allocated_rows;
	allocated_rows *= 2;
	if (allocated_rows <= r)
	  allocated_rows = r + 1;
	entry = new PPtable_entry[allocated_rows];
	memcpy(entry, old_entry, sizeof(table_entry**)*old_allocated_rows);
	delete[] old_entry;
	char **old_vrule = vrule;
	vrule = new char*[allocated_rows];
	memcpy(vrule, old_vrule, sizeof(char*)*old_allocated_rows);
	delete[] old_vrule;
      }
    }
    assert(allocated_rows > r);
    while (nrows <= r) {
      entry[nrows] = new table_entry*[ncolumns];
      int i;
      for (i = 0; i < ncolumns; i++)
	entry[nrows][i] = 0;
      vrule[nrows] = new char[ncolumns+1];
      for (i = 0; i < ncolumns+1; i++)
	vrule[nrows][i] = 0;
      nrows++;
    }
  }
}

void table::do_hspan(int r, int c)
{
  assert(r >= 0 && c >= 0 && r < nrows && c < ncolumns);
  if (c == 0) {
    error("first column cannot be horizontally spanned");
    return;
  }
  table_entry *e = entry[r][c];
  if (e) {
    assert(e->start_row <= r && r <= e->end_row
	   && e->start_col <= c && c <= e->end_col
	   && e->end_row - e->start_row > 0
	   && e->end_col - e->start_col > 0);
    return;
  }
  e = entry[r][c-1];
  // e can be 0 if we had an empty entry or an error
  if (e == 0)
    return;
  if (e->start_row != r) {
    /*
      l l
      ^ s */
    error("impossible horizontal span at row %1, column %2", r + 1,
	  c + 1);
  }
  else {
    e->end_col = c;
    entry[r][c] = e;
  }
}

void table::do_vspan(int r, int c)
{
  assert(r >= 0 && c >= 0 && r < nrows && c < ncolumns);
  if (0 == r) {
    error("first row cannot be vertically spanned");
    return;
  }
  table_entry *e = entry[r][c];
  if (e) {
    assert(e->start_row <= r);
    assert(r <= e->end_row);
    assert(e->start_col <= c);
    assert(c <= e->end_col);
    assert((e->end_row - e->start_row) > 0);
    assert((e->end_col - e->start_col) > 0);
    return;
  }
  e = entry[r-1][c];
  // e can be a null pointer if we had an empty entry or an error
  if (0 == e)
    return;
  if (e->start_col != c) {
    /* l s
       l ^ */
    error("impossible vertical span at row %1, column %2", r + 1,
	  c + 1);
  }
  else {
    for (int i = c; i <= e->end_col; i++) {
      assert(entry[r][i] == 0);
      entry[r][i] = e;
    }
    e->end_row = r;
  }
}

int find_decimal_point(const char *s, char decimal_point_char,
		       const char *delim)
{
  if (s == 0 || *s == '\0')
    return -1;
  const char *p;
  int in_delim = 0;		// is p within eqn delimiters?
  // tbl recognises \& even within eqn delimiters; I don't
  for (p = s; *p; p++)
    if (in_delim) {
      if (*p == delim[1])
	in_delim = 0;
    }
    else if (*p == delim[0])
      in_delim = 1;
    else if (p[0] == '\\' && p[1] == '&')
      return p - s;
  int possible_pos = -1;
  in_delim = 0;
  for (p = s; *p; p++)
    if (in_delim) {
      if (*p == delim[1])
	in_delim = 0;
    }
    else if (*p == delim[0])
      in_delim = 1;
    else if (p[0] == decimal_point_char && csdigit(p[1]))
      possible_pos = p - s;
  if (possible_pos >= 0)
    return possible_pos;
  in_delim = 0;
  for (p = s; *p; p++)
    if (in_delim) {
      if (*p == delim[1])
	in_delim = 0;
    }
    else if (*p == delim[0])
      in_delim = 1;
    else if (csdigit(*p))
      possible_pos = p + 1 - s;
  return possible_pos;
}

void table::add_entry(int r, int c, const string &str,
		      const entry_format *f, const char *fn, int ln)
{
  allocate(r);
  table_entry *e = 0 /* nullptr */;
  int len = str.length();
  char *s = str.extract();
  // Diagnose escape sequences that can wreak havoc in generated output.
  if (len > 1) {
    // A comment on a control line or in a text block is okay.
    int commentpos = str.find("\\\"");
    if (commentpos != -1) {
      int controlpos = str.search('.');
      if ((-1 == str.search('\n')) // not a text block, AND
	  && ((-1 == controlpos) // (no control character in line OR
	      || (0 == controlpos))) // control character at line start)
	warning_with_file_and_line(fn, ln, "comment escape sequence"
				   " '\\\"' in entry \"%1\"", s);
    }
    int gcommentpos = str.find("\\#");
    // If both types of comment are present, the first is what matters.
    if ((gcommentpos != -1) && (gcommentpos < commentpos))
      commentpos = gcommentpos;
    if (commentpos != -1) {
      int controlpos = str.search('.');
      if ((-1 == str.search('\n')) // not a text block, AND
	  && ((-1 == controlpos) // (no control character in line OR
	      || (0 == controlpos))) // control character at line start)
	warning_with_file_and_line(fn, ln, "comment escape sequence"
				   " '\\#' in entry \"%1\"", s);
    }
    // A \! escape sequence after a comment has started is okay.
    int exclpos = str.find("\\!");
    if ((exclpos != -1)
	&& ((-1 == commentpos)
	    || (exclpos < commentpos))) {
      if (-1 == str.search('\n')) // not a text block
	warning_with_file_and_line(fn, ln, "transparent throughput"
				   " escape sequence '\\!' in entry"
				   " \"%1\"", s);
      else
	warning_with_file_and_line(fn, ln, "transparent throughput"
				   " escape sequence '\\!' in text"
				   " block entry");
    }
    // An incomplete \z sequence at the entry's end causes problems.
    if (str.find("\\z") == (len - 2)) { // max valid index is (len - 1)
      if (-1 == str.search('\n')) // not a text block
	error_with_file_and_line(fn, ln, "zero-motion escape sequence"
				 " '\\z' at end of entry \"%1\"", s);
      else
	error_with_file_and_line(fn, ln, "zero-motion escape sequence"
				 " '\\z' at end of text block entry");
    }
  }
  if (str.search('\n') != -1) { // if it's a text block
    bool was_changed = false;
    int repeatpos = str.find("\\R");
    if (repeatpos != -1) {
	s[++repeatpos] = '&';
	was_changed = true;
      }
    if (was_changed)
      error_with_file_and_line(fn, ln, "repeating a glyph with '\\R'"
			       " is not allowed in a text block");
  }
  if (str == "\\_") {
    e = new short_line_entry(this, f);
  }
  else if (str == "\\=") {
    e = new short_double_line_entry(this, f);
  }
  else if (str == "_") {
    single_line_entry *lefte;
    if (c > 0 && entry[r][c-1] != 0 &&
	(lefte = entry[r][c-1]->to_single_line_entry()) != 0
	&& lefte->start_row == r
	&& lefte->mod->stagger == f->stagger) {
      lefte->end_col = c;
      entry[r][c] = lefte;
    }
    else
      e = new single_line_entry(this, f);
  }
  else if (str == "=") {
    double_line_entry *lefte;
    if (c > 0 && entry[r][c-1] != 0 &&
	(lefte = entry[r][c-1]->to_double_line_entry()) != 0
	&& lefte->start_row == r
	&& lefte->mod->stagger == f->stagger) {
      lefte->end_col = c;
      entry[r][c] = lefte;
    }
    else
      e = new double_line_entry(this, f);
  }
  else if (str == "\\^") {
    if (r == 0) {
      error("first row cannot contain a vertical span entry '\\^'");
      e = new empty_entry(this, f);
    }
    else
      do_vspan(r, c);
  }
  else if (strncmp(s, "\\R", 2) == 0) {
    if (len < 3) {
      error("an ordinary or special character must follow '\\R'");
      e = new empty_entry(this, f);
    }
    else {
      char *glyph = str.substring(2, len - 2).extract();
      e = new repeated_char_entry(this, f, glyph);
    }
  }
  else {
    int is_block = str.search('\n') >= 0;
    switch (f->type) {
    case FORMAT_SPAN:
      assert(str.empty());
      do_hspan(r, c);
      break;
    case FORMAT_LEFT:
      if (!str.empty()) {
	if (is_block)
	  e = new left_block_entry(this, f, s);
	else
	  e = new left_text_entry(this, f, s);
      }
      else
	e = new empty_entry(this, f);
      break;
    case FORMAT_CENTER:
      if (!str.empty()) {
	if (is_block)
	  e = new center_block_entry(this, f, s);
	else
	  e = new center_text_entry(this, f, s);
      }
      else
	e = new empty_entry(this, f);
      break;
    case FORMAT_RIGHT:
      if (!str.empty()) {
	if (is_block)
	  e = new right_block_entry(this, f, s);
	else
	  e = new right_text_entry(this, f, s);
      }
      else
	e = new empty_entry(this, f);
      break;
    case FORMAT_NUMERIC:
      if (!str.empty()) {
	if (is_block) {
	  warning_with_file_and_line(fn, ln, "treating text block in"
				     " table entry with numeric format"
				     " as left-aligned");
	  e = new left_block_entry(this, f, s);
	}
	else {
	  int pos = find_decimal_point(s, decimal_point_char, delim);
	  if (pos < 0)
	    e = new center_text_entry(this, f, s);
	  else
	    e = new numeric_text_entry(this, f, s, pos);
	}
      }
      else
	e = new empty_entry(this, f);
      break;
    case FORMAT_ALPHABETIC:
      if (!str.empty()) {
	if (is_block)
	  e = new alphabetic_block_entry(this, f, s);
	else
	  e = new alphabetic_text_entry(this, f, s);
      }
      else
	e = new empty_entry(this, f);
      break;
    case FORMAT_VSPAN:
      do_vspan(r, c);
      break;
    case FORMAT_HRULE:
      if ((str.length() != 0) && (str != "\\&"))
	error_with_file_and_line(fn, ln,
				 "ignoring non-empty data entry using"
				 " '_' column classifier");
      e = new single_line_entry(this, f);
      break;
    case FORMAT_DOUBLE_HRULE:
      if ((str.length() != 0) && (str != "\\&"))
	error_with_file_and_line(fn, ln,
				 "ignoring non-empty data entry using"
				 " '=' column classifier");
      e = new double_line_entry(this, f);
      break;
    default:
      assert(0 == "table column format not in FORMAT_{SPAN,LEFT,CENTER,"
		  "RIGHT,NUMERIC,ALPHABETIC,VSPAN,HRULE,DOUBLE_HRULE}");
    }
  }
  if (e) {
    table_entry *preve = entry[r][c];
    if (preve) {
      /* c s
         ^ l */
      error_with_file_and_line(fn, ln, "row %1, column %2 already"
				       " spanned",
			       r + 1, c + 1);
      delete e;
    }
    else {
      e->input_lineno = ln;
      e->input_filename = fn;
      e->start_row = e->end_row = r;
      e->start_col = e->end_col = c;
      *entry_list_tailp = e;
      entry_list_tailp = &e->next;
      entry[r][c] = e;
    }
  }
}

// add vertical lines for row r

void table::add_vrules(int r, const char *v)
{
  allocate(r);
  bool lwarned = false;
  bool twarned = false;
  for (int i = 0; i < ncolumns+1; i++) {
    assert(v[i] < 3);
    if (v[i] && (flags & (BOX | ALLBOX | DOUBLEBOX)) && (i == 0)
	&& (!lwarned)) {
      error("ignoring vertical line at leading edge of boxed table");
      lwarned = true;
    }
    else if (v[i] && (flags & (BOX | ALLBOX | DOUBLEBOX))
	     && (i == ncolumns) && (!twarned)) {
      error("ignoring vertical line at trailing edge of boxed table");
      twarned = true;
    }
    else
      vrule[r][i] = v[i];
  }
}

void table::check()
{
  table_entry *p = entry_list;
  int i, j;
  while (p) {
    for (i = p->start_row; i <= p->end_row; i++)
      for (j = p->start_col; j <= p->end_col; j++)
	assert(entry[i][j] == p);
    p = p->next;
  }
}

void table::print()
{
  location_force_filename = 1;
  check();
  init_output();
  determine_row_type();
  compute_widths();
  if (!(flags & CENTER))
    prints(".if \\n[" SAVED_CENTER_REG "] \\{\\\n");
  prints(".  in +(u;\\n[.l]-\\n[.i]-\\n[TW]/2>?-\\n[.i])\n"
	 ".  nr " SAVED_INDENT_REG " \\n[.i]\n");
  if (!(flags & CENTER))
    prints(".\\}\n");
  build_vrule_list();
  define_bottom_macro();
  do_top();
  for (int i = 0; i < nrows; i++)
    do_row(i);
  do_bottom();
}

void table::determine_row_type()
{
  row_is_all_lines = new char[nrows];
  for (int i = 0; i < nrows; i++) {
    bool had_single = false;
    bool had_double = false;
    bool had_non_line = false;
    for (int c = 0; c < ncolumns; c++) {
      table_entry *e = entry[i][c];
      if (e != 0) {
	if (e->start_row == e->end_row) {
	  int t = e->line_type();
	  switch (t) {
	  case -1:
	    had_non_line = true;
	    break;
	  case 0:
	    // empty
	    break;
	  case 1:
	    had_single = true;
	    break;
	  case 2:
	    had_double = true;
	    break;
	  default:
	    assert(0 == "table entry line type not in {-1, 0, 1, 2}");
	  }
	  if (had_non_line)
	    break;
	}
	c = e->end_col;
      }
    }
    if (had_non_line)
      row_is_all_lines[i] = 0;
    else if (had_double)
      row_is_all_lines[i] = 2;
    else if (had_single)
      row_is_all_lines[i] = 1;
    else
      row_is_all_lines[i] = 0;
  }
}

int table::count_expand_columns()
{
  int count = 0;
  for (int i = 0; i < ncolumns; i++)
    if (expand[i])
      count++;
  return count;
}

void table::init_output()
{
  prints(".\\\" initialize output\n");
  prints(".nr " COMPATIBLE_REG " \\n(.C\n"
	 ".cp 0\n");
  if (linesize > 0)
    printfs(".nr " LINESIZE_REG " %1\n", as_string(linesize));
  else
    prints(".nr " LINESIZE_REG " \\n[.s]\n");
  if (!(flags & CENTER))
    prints(".nr " SAVED_CENTER_REG " \\n[.ce]\n");
  if (compatible_flag)
    prints(".ds " LEADER_REG " \\a\n");
  if (!(flags & NOKEEP))
    prints(".if !r " USE_KEEPS_REG " .nr " USE_KEEPS_REG " 1\n");
  prints(".de " RESET_MACRO_NAME "\n"
	 ".  ft \\n[.f]\n"
	 ".  ps \\n[.s]\n"
	 ".  vs \\n[.v]u\n"
	 ".  in \\n[.i]u\n"
	 ".  ll \\n[.l]u\n"
	 ".  ls \\n[.L]\n"
	 ".  hy \\\\n[" SAVED_HYPHENATION_MODE_REG "]\n"
	 ".  hla \\\\*[" SAVED_HYPHENATION_LANG_NAME "]\n"
	 ".  hlm \\\\n[" SAVED_HYPHENATION_MAX_LINES_REG "]\n"
	 ".  hym \\\\n[" SAVED_HYPHENATION_MARGIN_REG "]u\n"
	 ".  hys \\\\n[" SAVED_HYPHENATION_SPACE_REG "]u\n"
	 ".  ad \\n[.j]\n"
	 ".  ie \\n[.u] .fi\n"
	 ".  el .nf\n"
	 ".  ce \\n[.ce]\n"
	 ".  ta \\\\*[" SAVED_TABS_NAME "]\n"
	 ".  ss \\\\n[" SAVED_INTER_WORD_SPACE_SIZE "]"
	 " \\\\n[" SAVED_INTER_SENTENCE_SPACE_SIZE "]\n"
	 "..\n"
	 ".nr " SAVED_INDENT_REG " \\n[.i]\n"
	 ".nr " SAVED_FONT_REG " \\n[.f]\n"
	 ".nr " SAVED_SIZE_REG " \\n[.s]\n"
	 ".nr " SAVED_FILL_REG " \\n[.u]\n"
	 ".ds " SAVED_TABS_NAME " \\n[.tabs]\n"
	 ".nr " SAVED_INTER_WORD_SPACE_SIZE " \\n[.ss]\n"
	 ".nr " SAVED_INTER_SENTENCE_SPACE_SIZE " \\n[.sss]\n"
	 ".nr " SAVED_HYPHENATION_MODE_REG " \\n[.hy]\n"
	 ".ds " SAVED_HYPHENATION_LANG_NAME " \\n[.hla]\n"
	 ".nr " SAVED_HYPHENATION_MAX_LINES_REG " (\\n[.hlm])\n"
	 ".nr " SAVED_HYPHENATION_MARGIN_REG " \\n[.hym]\n"
	 ".nr " SAVED_HYPHENATION_SPACE_REG " \\n[.hys]\n"
	 ".nr T. 0\n"
	 ".nr " CURRENT_ROW_REG " 0-1\n"
	 ".nr " LAST_PASSED_ROW_REG " 0-1\n"
	 ".nr " SECTION_DIVERSION_FLAG_REG " 0\n"
	 ".ds " TRANSPARENT_STRING_NAME "\n"
	 ".ds " QUOTE_STRING_NAME "\n"
	 ".nr " NEED_BOTTOM_RULE_REG " 1\n"
	 ".nr " SUPPRESS_BOTTOM_REG " 0\n"
	 ".eo\n"
	 ".de " TEXT_BLOCK_STAGGERING_MACRO "\n"
	 ".  ie !'\\n(.z'' \\!.3sp \"\\$1\"\n"
	 ".  el .sp \\$1\n"
	 "..\n"
	 ".de " REPEATED_MARK_MACRO "\n"
	 ".  mk \\$1\n"
	 ".  if !'\\n(.z'' \\!." REPEATED_MARK_MACRO " \"\\$1\"\n"
	 "..\n"
	 ".de " REPEATED_VPT_MACRO "\n"
	 ".  vpt \\$1\n"
	 ".  if !'\\n(.z'' \\!." REPEATED_VPT_MACRO " \"\\$1\"\n"
	 "..\n");
  if (!(flags & NOKEEP)) {
    prints(".de " KEEP_MACRO_NAME "\n"
	   ".  if '\\n[.z]'' \\{\\\n"
	   ".    ds " QUOTE_STRING_NAME " \\\\\n"
	   ".    ds " TRANSPARENT_STRING_NAME " \\!\n"
	   ".    di " SECTION_DIVERSION_NAME "\n"
	   ".    nr " SECTION_DIVERSION_FLAG_REG " 1\n"
	   ".    in 0\n"
	   ".  \\}\n"
	   "..\n"
	   // Protect '#' in macro name from being interpreted by eqn.
	   ".ig\n"
	   ".EQ\n"
	   "delim off\n"
	   ".EN\n"
	   "..\n"
	   ".de " RELEASE_MACRO_NAME "\n"
	   ".  if \\n[" SECTION_DIVERSION_FLAG_REG "] \\{\\\n"
	   ".    di\n"
	   ".    in \\n[" SAVED_INDENT_REG "]u\n"
	   ".    nr " SAVED_DN_REG " \\n[dn]\n"
	   ".    ds " QUOTE_STRING_NAME "\n"
	   ".    ds " TRANSPARENT_STRING_NAME "\n"
	   ".    nr " SECTION_DIVERSION_FLAG_REG " 0\n"
	   ".    if \\n[.t]<=\\n[dn] \\{\\\n"
	   ".      nr T. 1\n"
	   ".      T#\n"
	   ".      nr " SUPPRESS_BOTTOM_REG " 1\n"
	   ".      sp \\n[.t]u\n"
	   ".      nr " SUPPRESS_BOTTOM_REG " 0\n"
	   ".      mk #T\n"
	   ".    \\}\n");
    if (!(flags & NOWARN)) {
      prints(".    if \\n[.t]<=\\n[" SAVED_DN_REG "] \\{\\\n");
      // eqn(1) delimiters have already been switched off.
      entry_list->set_location();
      // Since we turn off traps, troff won't go into an infinite loop
      // when we output the table row; it will just flow off the bottom
      // of the page.
      prints(".      tmc \\n[.F]:\\n[.c]: warning:\n"
	     ".      tm1 \" table row does not fit on page \\n%\n");
      prints(".    \\}\n");
    }
    prints(".    nf\n"
	   ".    ls 1\n"
	   ".    " SECTION_DIVERSION_NAME "\n"
	   ".    ls\n"
	   ".    rm " SECTION_DIVERSION_NAME "\n"
	   ".  \\}\n"
	   "..\n"
	   ".ig\n"
	   ".EQ\n"
	   "delim on\n"
	   ".EN\n"
	   "..\n"
	   ".nr " TABLE_DIVERSION_FLAG_REG " 0\n"
	   ".de " TABLE_KEEP_MACRO_NAME "\n"
	   ".  if '\\n[.z]'' \\{\\\n"
	   ".    di " TABLE_DIVERSION_NAME "\n"
	   ".    nr " TABLE_DIVERSION_FLAG_REG " 1\n"
	   ".  \\}\n"
	   "..\n"
	   ".de " TABLE_RELEASE_MACRO_NAME "\n"
	   ".  if \\n[" TABLE_DIVERSION_FLAG_REG "] \\{\\\n"
	   ".    br\n"
	   ".    di\n"
	   ".    nr " SAVED_DN_REG " \\n[dn]\n"
	   ".    ne \\n[dn]u+\\n[.V]u\n"
	   ".    ie \\n[.t]<=\\n[" SAVED_DN_REG "] \\{\\\n");
    // Protect characters in diagnostic message (especially :, [, ])
    // from being interpreted by eqn.
    prints(".      ds " NOP_NAME " \\\" empty\n");
    prints(".      ig " NOP_NAME "\n"
	   ".EQ\n"
	   "delim off\n"
	   ".EN\n"
	   ".      " NOP_NAME "\n");
    entry_list->set_location();
    prints(".      nr " PREVIOUS_PAGE_REG " (\\n% - 1)\n"
	   ".      tmc \\n[.F]:\\n[.c]: error:\n"
	   ".      tmc \" boxed table does not fit on page"
	   " \\n[" PREVIOUS_PAGE_REG "];\n"
	   ".      tm1 \" use .TS H/.TH with a supporting macro package"
	   "\n"
	   ".      rr " PREVIOUS_PAGE_REG "\n");
    prints(".      ig " NOP_NAME "\n"
	   ".EQ\n"
	   "delim on\n"
	   ".EN\n"
	   ".      " NOP_NAME "\n");
    prints(".    \\}\n"
	   ".  el \\{\\\n"
	   ".    in 0\n"
	   ".    ls 1\n"
	   ".    nf\n"
	   ".    " TABLE_DIVERSION_NAME "\n"
	   ".  \\}\n"
	   ".  rm " TABLE_DIVERSION_NAME "\n"
	   ".  \\}\n"
	   "..\n");
  }
  prints(".ec\n"
	 ".ce 0\n");
  prints(".nr " SAVED_NUMBERING_LINENO " \\n[ln]\n"
	 ".nr ln 0\n"
	 ".nr " SAVED_NUMBERING_ENABLED " \\n[.nm]\n"
	 ".nr " SAVED_NUMBERING_SUPPRESSION_COUNT " \\n[.nn]\n"
	 ".nn \\n[.R]\n"); // INT_MAX as of groff 1.24
  prints(".nf\n");
}

string block_width_reg(int r, int c)
{
  static char name[sizeof(BLOCK_WIDTH_PREFIX)+INT_DIGITS+1+INT_DIGITS];
  sprintf(name, BLOCK_WIDTH_PREFIX "%d,%d", r, c);
  return string(name);
}

string block_diversion_name(int r, int c)
{
  static char name[sizeof(BLOCK_DIVERSION_PREFIX)+INT_DIGITS+1+INT_DIGITS];
  sprintf(name, BLOCK_DIVERSION_PREFIX "%d,%d", r, c);
  return string(name);
}

string block_height_reg(int r, int c)
{
  static char name[sizeof(BLOCK_HEIGHT_PREFIX)+INT_DIGITS+1+INT_DIGITS];
  sprintf(name, BLOCK_HEIGHT_PREFIX "%d,%d", r, c);
  return string(name);
}

string span_width_reg(int start_col, int end_col)
{
  static char name[sizeof(SPAN_WIDTH_PREFIX)+INT_DIGITS+1+INT_DIGITS];
  sprintf(name, SPAN_WIDTH_PREFIX "%d", start_col);
  if (end_col != start_col)
    sprintf(strchr(name, '\0'), ",%d", end_col);
  return string(name);
}

string span_left_numeric_width_reg(int start_col, int end_col)
{
  static char name[sizeof(SPAN_LEFT_NUMERIC_WIDTH_PREFIX)+INT_DIGITS+1+INT_DIGITS];
  sprintf(name, SPAN_LEFT_NUMERIC_WIDTH_PREFIX "%d", start_col);
  if (end_col != start_col)
    sprintf(strchr(name, '\0'), ",%d", end_col);
  return string(name);
}

string span_right_numeric_width_reg(int start_col, int end_col)
{
  static char name[sizeof(SPAN_RIGHT_NUMERIC_WIDTH_PREFIX)+INT_DIGITS+1+INT_DIGITS];
  sprintf(name, SPAN_RIGHT_NUMERIC_WIDTH_PREFIX "%d", start_col);
  if (end_col != start_col)
    sprintf(strchr(name, '\0'), ",%d", end_col);
  return string(name);
}

string span_alphabetic_width_reg(int start_col, int end_col)
{
  static char name[sizeof(SPAN_ALPHABETIC_WIDTH_PREFIX)+INT_DIGITS+1+INT_DIGITS];
  sprintf(name, SPAN_ALPHABETIC_WIDTH_PREFIX "%d", start_col);
  if (end_col != start_col)
    sprintf(strchr(name, '\0'), ",%d", end_col);
  return string(name);
}

string column_separation_reg(int col)
{
  static char name[sizeof(COLUMN_SEPARATION_PREFIX)+INT_DIGITS];
  sprintf(name, COLUMN_SEPARATION_PREFIX "%d", col);
  return string(name);
}

string row_start_reg(int row)
{
  static char name[sizeof(ROW_START_PREFIX)+INT_DIGITS];
  sprintf(name, ROW_START_PREFIX "%d", row);
  return string(name);
}

string column_start_reg(int col)
{
  static char name[sizeof(COLUMN_START_PREFIX)+INT_DIGITS];
  sprintf(name, COLUMN_START_PREFIX "%d", col);
  return string(name);
}

string column_end_reg(int col)
{
  static char name[sizeof(COLUMN_END_PREFIX)+INT_DIGITS];
  sprintf(name, COLUMN_END_PREFIX "%d", col);
  return string(name);
}

string column_divide_reg(int col)
{
  static char name[sizeof(COLUMN_DIVIDE_PREFIX)+INT_DIGITS];
  sprintf(name, COLUMN_DIVIDE_PREFIX "%d", col);
  return string(name);
}

string row_top_reg(int row)
{
  static char name[sizeof(ROW_TOP_PREFIX)+INT_DIGITS];
  sprintf(name, ROW_TOP_PREFIX "%d", row);
  return string(name);
}

void init_span_reg(int start_col, int end_col)
{
  printfs(".nr %1 \\n(.H\n.nr %2 0\n.nr %3 0\n.nr %4 0\n",
	  span_width_reg(start_col, end_col),
	  span_alphabetic_width_reg(start_col, end_col),
	  span_left_numeric_width_reg(start_col, end_col),
	  span_right_numeric_width_reg(start_col, end_col));
}

void compute_span_width(int start_col, int end_col)
{
  printfs(".nr %1 \\n[%1]>?(\\n[%2]+\\n[%3])\n"
	  ".if \\n[%4] .nr %1 \\n[%1]>?(\\n[%4]+2n)\n",
	  span_width_reg(start_col, end_col),
	  span_left_numeric_width_reg(start_col, end_col),
	  span_right_numeric_width_reg(start_col, end_col),
	  span_alphabetic_width_reg(start_col, end_col));
}

// Increase the widths of columns so that the width of any spanning
// entry is not greater than the sum of the widths of the columns that
// it spans.  Ensure that the widths of columns remain equal.

void table::divide_span(int start_col, int end_col)
{
  assert(end_col > start_col);
  printfs(".nr " NEEDED_REG " \\n[%1]-(\\n[%2]",
	  span_width_reg(start_col, end_col),
	  span_width_reg(start_col, start_col));
  int i;
  for (i = start_col + 1; i <= end_col; i++) {
    // The column separation may shrink with the expand option.
    if (!(flags & EXPAND))
      printfs("+%1n", as_string(column_separation[i - 1]));
    printfs("+\\n[%1]", span_width_reg(i, i));
  }
  prints(")\n");
  printfs(".nr " NEEDED_REG " \\n[" NEEDED_REG "]/%1\n",
	  as_string(end_col - start_col + 1));
  prints(".if \\n[" NEEDED_REG "] \\{\\\n");
  for (i = start_col; i <= end_col; i++)
    printfs(".  nr %1 +\\n[" NEEDED_REG "]\n",
	    span_width_reg(i, i));
  int equal_flag = 0;
  for (i = start_col; i <= end_col && !equal_flag; i++)
    if (equal[i] || expand[i])
      equal_flag = 1;
  if (equal_flag) {
    for (i = 0; i < ncolumns; i++)
      if (i < start_col || i > end_col)
	printfs(".  nr %1 +\\n[" NEEDED_REG "]\n",
	    span_width_reg(i, i));
  }
  prints(".\\}\n");
}

void table::sum_columns(int start_col, int end_col, int do_expand)
{
  assert(end_col > start_col);
  int i;
  for (i = start_col; i <= end_col; i++)
    if (expand[i])
      break;
  if (i > end_col) {
    if (do_expand)
      return;
  }
  else {
    if (!do_expand)
      return;
  }
  printfs(".nr %1 \\n[%2]",
	  span_width_reg(start_col, end_col),
	  span_width_reg(start_col, start_col));
  for (i = start_col + 1; i <= end_col; i++)
    printfs("+(%1*\\n[" SEPARATION_FACTOR_REG "])+\\n[%2]",
	    as_string(column_separation[i - 1]),
	    span_width_reg(i, i));
  prints('\n');
}

horizontal_span::horizontal_span(int sc, int ec, horizontal_span *p)
: next(p), start_col(sc), end_col(ec)
{
}

void table::build_span_list()
{
  span_list = 0;
  table_entry *p = entry_list;
  while (p) {
    if (p->end_col != p->start_col) {
      horizontal_span *q;
      for (q = span_list; q; q = q->next)
	if (q->start_col == p->start_col
	    && q->end_col == p->end_col)
	  break;
      if (!q)
	span_list = new horizontal_span(p->start_col, p->end_col, span_list);
    }
    p = p->next;
  }
  // Now sort span_list primarily by order of end_row, and secondarily
  // by reverse order of start_row. This ensures that if we divide
  // spans using the order in span_list, we will get reasonable results.
  horizontal_span *unsorted = span_list;
  span_list = 0;
  while (unsorted) {
    horizontal_span **pp;
    for (pp = &span_list; *pp; pp = &(*pp)->next)
      if (unsorted->end_col < (*pp)->end_col
	  || (unsorted->end_col == (*pp)->end_col
	      && (unsorted->start_col > (*pp)->start_col)))
	break;
    horizontal_span *tem = unsorted->next;
    unsorted->next = *pp;
    *pp = unsorted;
    unsorted = tem;
  }
}

void table::compute_overall_width()
{
  prints(".\\\" compute overall width\n");
  if (!(flags & GAP_EXPAND)) {
    if (left_separation)
      printfs(".if n .ll -%1n \\\" left separation\n",
	      as_string(left_separation));
    if (right_separation)
      printfs(".if n .ll -%1n \\\" right separation\n",
	      as_string(right_separation));
  }
  if (!(flags & (ALLBOX | BOX | DOUBLEBOX)) && (flags & HAS_DATA_HRULE))
    prints(".if n .ll -1n \\\" horizontal rule compensation\n");
  // Compute the amount of horizontal space available for expansion,
  // measuring every column _including_ those eligible for expansion.
  // This is the minimum required to set the table without compression.
  prints(".nr " EXPAND_REG " 0\n");
  prints(".nr " AVAILABLE_WIDTH_REG " \\n[.l]-\\n[.i]");
  for (int i = 0; i < ncolumns; i++)
    printfs("-\\n[%1]", span_width_reg(i, i));
  if (total_separation)
    printfs("-%1n", as_string(total_separation));
  prints("\n");
  // If the "expand" region option was given, a different warning will
  // be issued later (if "nowarn" was not also specified).
  if ((!(flags & NOWARN)) && (!(flags & EXPAND))) {
    prints(".if \\n[" AVAILABLE_WIDTH_REG "]<0 \\{\\\n");
    // Protect characters in diagnostic message (especially :, [, ])
    // from being interpreted by eqn.
    prints(".  ig\n"
	   ".EQ\n"
	   "delim off\n"
	   ".EN\n"
	   ".  .\n");
    entry_list->set_location();
    prints(".  tmc \\n[.F]:\\n[.c]: warning:\n"
	   ".  tm1 \" table wider than line length minus indentation"
	   "\n");
    prints(".  ig\n"
	   ".EQ\n"
	   "delim on\n"
	   ".EN\n"
	   ".  .\n");
    prints(".  nr " AVAILABLE_WIDTH_REG " 0\n");
    prints(".\\}\n");
  }
  // Now do a similar computation, this time omitting columns that
  // _aren't_ undergoing expansion.  The difference is the amount of
  // space we have to distribute among the expanded columns.
  bool do_expansion = false;
  for (int i = 0; i < ncolumns; i++)
    if (expand[i]) {
      do_expansion = true;
      break;
    }
  if (do_expansion) {
    prints(".if \\n[" AVAILABLE_WIDTH_REG "] \\\n");
    prints(".  nr " EXPAND_REG " \\n[.l]-\\n[.i]");
    for (int i = 0; i < ncolumns; i++)
      if (!expand[i])
	printfs("-\\n[%1]", span_width_reg(i, i));
    if (total_separation)
      printfs("-%1n", as_string(total_separation));
    prints("\n");
    int colcount = count_expand_columns();
    if (colcount > 1)
      printfs(".nr " EXPAND_REG " \\n[" EXPAND_REG "]/%1\n",
	      as_string(colcount));
    for (int i = 0; i < ncolumns; i++)
      if (expand[i])
	printfs(".nr %1 \\n[%1]>?\\n[" EXPAND_REG "]\n",
		span_width_reg(i, i));
  }
}

void table::compute_total_separation()
{
  if (flags & (ALLBOX | BOX | DOUBLEBOX))
    left_separation = right_separation = 1;
  else {
    for (int r = 0; r < nrows; r++) {
      if (vrule[r][0] > 0)
	left_separation = 1;
      if (vrule[r][ncolumns] > 0)
	right_separation = 1;
    }
  }
  total_separation = left_separation + right_separation;
  for (int c = 0; c < ncolumns - 1; c++)
    total_separation += column_separation[c];
}

void table::compute_separation_factor()
{
  prints(".\\\" compute column separation factor\n");
  // Don't let the separation factor be negative.
  prints(".nr " SEPARATION_FACTOR_REG " \\n[.l]-\\n[.i]");
  for (int i = 0; i < ncolumns; i++)
    printfs("-\\n[%1]", span_width_reg(i, i));
  printfs("/%1\n", as_string(total_separation));
  // Store the remainder for use in compute_column_positions().
  if (flags & GAP_EXPAND) {
    prints(".if n \\\n");
    prints(".  nr " LEFTOVER_FACTOR_REG " \\n[.l]-\\n[.i]");
    for (int i = 0; i < ncolumns; i++)
      printfs("-\\n[%1]", span_width_reg(i, i));
    printfs("%%%1\n", as_string(total_separation));
  }
  prints(".ie \\n[" SEPARATION_FACTOR_REG "]<=0 \\{\\\n");
  if (!(flags & NOWARN)) {
    // Protect characters in diagnostic message (especially :, [, ])
    // from being interpreted by eqn.
    prints(".ig\n"
	   ".EQ\n"
	   "delim off\n"
	   ".EN\n"
	   "..\n");
    entry_list->set_location();
    prints(".tmc \\n[.F]:\\n[.c]: warning:\n"
	   ".tm1 \" table column separation reduced to zero\n"
	   ".nr " SEPARATION_FACTOR_REG " 0\n");
  }
  prints(".\\}\n"
	 ".el .if \\n[" SEPARATION_FACTOR_REG "]<1n \\{\\\n");
  if (!(flags & NOWARN)) {
    entry_list->set_location();
    prints(".tmc \\n[.F]:\\n[.c]: warning:\n"
	   ".tm1 \" table column separation reduced to fit line"
	   " length\n");
    prints(".ig\n"
	   ".EQ\n"
	   "delim on\n"
	   ".EN\n"
	   "..\n");
  }
  prints(".\\}\n");
}

void table::compute_column_positions()
{
  prints(".\\\" compute column positions\n");
  printfs(".nr %1 0\n", column_divide_reg(0));
  printfs(".nr %1 %2n\n", column_start_reg(0),
	  as_string(left_separation));
  // In nroff mode, compensate for width of vertical rule.
  if (left_separation)
    printfs(".if n .nr %1 +1n\n", column_start_reg(0));
  int i;
  for (i = 1;; i++) {
    printfs(".nr %1 \\n[%2]+\\n[%3]\n",
	    column_end_reg(i-1),
	    column_start_reg(i-1),
	    span_width_reg(i-1, i-1));
    if (i >= ncolumns)
      break;
    printfs(".nr %1 \\n[%2]+(%3*\\n[" SEPARATION_FACTOR_REG "])\n",
	    column_start_reg(i),
	    column_end_reg(i-1),
	    as_string(column_separation[i-1]));
    // If we have leftover expansion room in a table using the "expand"
    // region option, put it prior to the last column so that the table
    // looks as if expanded to the available line length.
    if ((ncolumns > 2) && (flags & GAP_EXPAND) && (i == (ncolumns - 1)))
      printfs(".if n .if \\n[" LEFTOVER_FACTOR_REG "] .nr %1 +(1n>?\\n["
	      LEFTOVER_FACTOR_REG "])\n",
	      column_start_reg(i));
    printfs(".nr %1 \\n[%2]+\\n[%3]/2\n",
	    column_divide_reg(i),
	    column_end_reg(i-1),
	    column_start_reg(i));
  }
  printfs(".nr %1 \\n[%2]+%3n\n",
	  column_divide_reg(ncolumns),
	  column_end_reg(i-1),
	  as_string(right_separation));
  printfs(".nr TW \\n[%1]\n",
	  column_divide_reg(ncolumns));
  if (flags & DOUBLEBOX) {
    printfs(".nr %1 +" DOUBLE_LINE_SEP "\n", column_divide_reg(0));
    printfs(".nr %1 -" DOUBLE_LINE_SEP "\n", column_divide_reg(ncolumns));
  }
}

void table::make_columns_equal()
{
  int first = -1;		// index of first equal column
  int i;
  for (i = 0; i < ncolumns; i++)
    if (equal[i]) {
      if (first < 0) {
	printfs(".nr %1 \\n[%1]", span_width_reg(i, i));
	first = i;
      }
      else
	printfs(">?\\n[%1]", span_width_reg(i, i));
    }
  if (first >= 0) {
    prints('\n');
    for (i = first + 1; i < ncolumns; i++)
      if (equal[i])
	printfs(".nr %1 \\n[%2]\n",
		span_width_reg(i, i),
		span_width_reg(first, first));
  }
}

void table::compute_widths()
{
  prints(".\\\" compute column widths\n");
  build_span_list();
  int i;
  horizontal_span *p;
  // These values get refined later.
  prints(".nr " SEPARATION_FACTOR_REG " 1n\n");
  for (i = 0; i < ncolumns; i++) {
    init_span_reg(i, i);
    if (!minimum_width[i].empty())
      printfs(".nr %1 (n;%2)\n", span_width_reg(i, i), minimum_width[i]);
  }
  for (p = span_list; p; p = p->next)
    init_span_reg(p->start_col, p->end_col);
  // Compute all field widths except for blocks.
  table_entry *q;
  for (q = entry_list; q; q = q->next)
    if (!q->mod->zero_width)
      q->do_width();
  // Compute all span widths, not handling blocks yet.
  for (i = 0; i < ncolumns; i++)
    compute_span_width(i, i);
  for (p = span_list; p; p = p->next)
    compute_span_width(p->start_col, p->end_col);
  // Making columns equal normally increases the width of some columns.
  make_columns_equal();
  // Note that divide_span keeps equal width columns equal.
  // This function might increase the width of some columns, too.
  for (p = span_list; p; p = p->next)
    divide_span(p->start_col, p->end_col);
  compute_total_separation();
  for (p = span_list; p; p = p->next)
    sum_columns(p->start_col, p->end_col, 0);
  // Now handle unexpanded blocks.
  bool had_spanning_block = false;
  bool had_equal_block = false;
  for (q = entry_list; q; q = q->next)
    if (q->divert(ncolumns, minimum_width,
		  (flags & EXPAND) ? column_separation : 0, 0)) {
      if (q->end_col > q->start_col)
	had_spanning_block = true;
      for (i = q->start_col; i <= q->end_col && !had_equal_block; i++)
	if (equal[i])
	  had_equal_block = true;
    }
  // Adjust widths.
  if (had_equal_block)
    make_columns_equal();
  if (had_spanning_block)
    for (p = span_list; p; p = p->next)
      divide_span(p->start_col, p->end_col);
  compute_overall_width();
  if ((flags & EXPAND) && total_separation != 0) {
    compute_separation_factor();
    for (p = span_list; p; p = p->next)
      sum_columns(p->start_col, p->end_col, 0);
  }
  else {
    // Handle expanded blocks.
    for (p = span_list; p; p = p->next)
      sum_columns(p->start_col, p->end_col, 1);
    for (q = entry_list; q; q = q->next)
      if (q->divert(ncolumns, minimum_width, 0, 1)) {
	if (q->end_col > q->start_col)
	  had_spanning_block = true;
      }
    // Adjust widths again.
    if (had_spanning_block)
      for (p = span_list; p; p = p->next)
	divide_span(p->start_col, p->end_col);
  }
  compute_column_positions();
}

void table::print_single_hrule(int r)
{
  prints(".vs " LINE_SEP ">?\\n[.V]u\n"
	 ".ls 1\n"
	 "\\v'" BODY_DEPTH "'"
	 "\\s[\\n[" LINESIZE_REG "]]");
  if (r > nrows - 1)
    prints("\\D'l |\\n[TW]u 0'");
  else {
    int start_col = 0;
    for (;;) {
      while (start_col < ncolumns
	     && entry[r][start_col] != 0
	     && entry[r][start_col]->start_row != r)
	start_col++;
      int end_col;
      for (end_col = start_col;
	   end_col < ncolumns
	   && (entry[r][end_col] == 0
	       || entry[r][end_col]->start_row == r);
	   end_col++)
	;
      if (end_col <= start_col)
	break;
      printfs("\\h'|\\n[%1]u",
	      column_divide_reg(start_col));
      if ((r > 0 && vrule[r-1][start_col] == 2)
	  || (r < nrows && vrule[r][start_col] == 2))
	prints("-" HALF_DOUBLE_LINE_SEP);
      prints("'");
      printfs("\\D'l |\\n[%1]u",
	      column_divide_reg(end_col));
      if ((r > 0 && vrule[r-1][end_col] == 2)
	  || (r < nrows && vrule[r][end_col] == 2))
	prints("+" HALF_DOUBLE_LINE_SEP);
      prints(" 0'");
      start_col = end_col;
    }
  }
  prints("\\s0\n");
  prints(".ls\n"
	 ".vs\n");
}

void table::print_double_hrule(int r)
{
  prints(".vs " LINE_SEP "+" DOUBLE_LINE_SEP
	 ">?\\n[.V]u\n"
	 ".ls 1\n"
	 "\\v'" BODY_DEPTH "'"
	 "\\s[\\n[" LINESIZE_REG "]]");
  if (r > nrows - 1)
    prints("\\v'-" DOUBLE_LINE_SEP "'"
	   "\\D'l |\\n[TW]u 0'"
	   "\\v'" DOUBLE_LINE_SEP "'"
	   "\\h'|0'"
	   "\\D'l |\\n[TW]u 0'");
  else {
    int start_col = 0;
    for (;;) {
      while (start_col < ncolumns
	     && entry[r][start_col] != 0
	     && entry[r][start_col]->start_row != r)
	start_col++;
      int end_col;
      for (end_col = start_col;
	   end_col < ncolumns
	   && (entry[r][end_col] == 0
	       || entry[r][end_col]->start_row == r);
	   end_col++)
	;
      if (end_col <= start_col)
	break;
      const char *left_adjust = 0;
      if ((r > 0 && vrule[r-1][start_col] == 2)
	  || (r < nrows && vrule[r][start_col] == 2))
	left_adjust = "-" HALF_DOUBLE_LINE_SEP;
      const char *right_adjust = 0;
      if ((r > 0 && vrule[r-1][end_col] == 2)
	  || (r < nrows && vrule[r][end_col] == 2))
	right_adjust = "+" HALF_DOUBLE_LINE_SEP;
      printfs("\\v'-" DOUBLE_LINE_SEP "'"
	      "\\h'|\\n[%1]u",
	      column_divide_reg(start_col));
      if (left_adjust)
	prints(left_adjust);
      prints("'");
      printfs("\\D'l |\\n[%1]u",
	      column_divide_reg(end_col));
      if (right_adjust)
	prints(right_adjust);
      prints(" 0'");
      printfs("\\v'" DOUBLE_LINE_SEP "'"
	      "\\h'|\\n[%1]u",
	      column_divide_reg(start_col));
      if (left_adjust)
	prints(left_adjust);
      prints("'");
      printfs("\\D'l |\\n[%1]u",
	      column_divide_reg(end_col));
      if (right_adjust)
	prints(right_adjust);
      prints(" 0'");
      start_col = end_col;
    }
  }
  prints("\\s0\n"
	 ".ls\n"
	 ".vs\n");
}

void table::compute_vrule_top_adjust(int start_row, int col, string &result)
{
  if (row_is_all_lines[start_row] && start_row < nrows - 1) {
    if (row_is_all_lines[start_row] == 2)
      result = LINE_SEP ">?\\n[.V]u" "+" DOUBLE_LINE_SEP;
    else
      result = LINE_SEP ">?\\n[.V]u";
    start_row++;
  }
  else {
    result = "";
    if (start_row == 0)
      return;
    for (stuff *p = stuff_list; p && p->row <= start_row; p = p->next)
      if (p->row == start_row
	  && (p->is_single_line() || p->is_double_line()))
	return;
  }
  int left = 0;
  if (col > 0) {
    table_entry *e = entry[start_row-1][col-1];
    if (e && e->start_row == e->end_row) {
      if (e->to_double_line_entry() != 0)
	left = 2;
      else if (e->to_single_line_entry() != 0)
	left = 1;
    }
  }
  int right = 0;
  if (col < ncolumns) {
    table_entry *e = entry[start_row-1][col];
    if (e && e->start_row == e->end_row) {
      if (e->to_double_line_entry() != 0)
	right = 2;
      else if (e->to_single_line_entry() != 0)
	right = 1;
    }
  }
  if (row_is_all_lines[start_row-1] == 0) {
    if (left > 0 || right > 0) {
      result += "-" BODY_DEPTH "-" BAR_HEIGHT;
      if ((left == 2 && right != 2) || (right == 2 && left != 2))
	result += "-" HALF_DOUBLE_LINE_SEP;
      else if (left == 2 && right == 2)
	result += "+" HALF_DOUBLE_LINE_SEP;
    }
  }
  else if (row_is_all_lines[start_row-1] == 2) {
    if ((left == 2 && right != 2) || (right == 2 && left != 2))
      result += "-" DOUBLE_LINE_SEP;
    else if (left == 1 || right == 1)
      result += "-" HALF_DOUBLE_LINE_SEP;
  }
}

void table::compute_vrule_bot_adjust(int end_row, int col, string &result)
{
  if (row_is_all_lines[end_row] && end_row > 0) {
    end_row--;
    result = "";
  }
  else {
    stuff *p;
    for (p = stuff_list; p && p->row < end_row + 1; p = p->next)
      ;
    if (p && p->row == end_row + 1 && p->is_double_line()) {
      result = "-" DOUBLE_LINE_SEP;
      return;
    }
    if ((p != 0 && p->row == end_row + 1)
	|| end_row == nrows - 1) {
      result = "";
      return;
    }
    if (row_is_all_lines[end_row+1] == 1)
      result = LINE_SEP;
    else if (row_is_all_lines[end_row+1] == 2)
      result = LINE_SEP "+" DOUBLE_LINE_SEP;
    else
      result = "";
  }
  int left = 0;
  if (col > 0) {
    table_entry *e = entry[end_row+1][col-1];
    if (e && e->start_row == e->end_row) {
      if (e->to_double_line_entry() != 0)
	left = 2;
      else if (e->to_single_line_entry() != 0)
	left = 1;
    }
  }
  int right = 0;
  if (col < ncolumns) {
    table_entry *e = entry[end_row+1][col];
    if (e && e->start_row == e->end_row) {
      if (e->to_double_line_entry() != 0)
	right = 2;
      else if (e->to_single_line_entry() != 0)
	right = 1;
    }
  }
  if (row_is_all_lines[end_row+1] == 0) {
    if (left > 0 || right > 0) {
      result = "1v-" BODY_DEPTH "-" BAR_HEIGHT;
      if ((left == 2 && right != 2) || (right == 2 && left != 2))
	result += "+" HALF_DOUBLE_LINE_SEP;
      else if (left == 2 && right == 2)
	result += "-" HALF_DOUBLE_LINE_SEP;
    }
  }
  else if (row_is_all_lines[end_row+1] == 2) {
    if (left == 2 && right == 2)
      result += "-" DOUBLE_LINE_SEP;
    else if (left != 2 && right != 2 && (left == 1 || right == 1))
      result += "-" HALF_DOUBLE_LINE_SEP;
  }
}

void table::add_vertical_rule(int start_row, int end_row,
			      int col, int is_double)
{
  vrule_list = new vertical_rule(start_row, end_row, col, is_double,
				 vrule_list);
  compute_vrule_top_adjust(start_row, col, vrule_list->top_adjust);
  compute_vrule_bot_adjust(end_row, col, vrule_list->bot_adjust);
}

void table::build_vrule_list()
{
  int col;
  if (flags & ALLBOX) {
    for (col = 1; col < ncolumns; col++) {
      int start_row = 0;
      for (;;) {
	while (start_row < nrows && vrule_spanned(start_row, col))
	  start_row++;
	if (start_row >= nrows)
	  break;
	int end_row = start_row;
	while (end_row < nrows && !vrule_spanned(end_row, col))
	  end_row++;
	end_row--;
	add_vertical_rule(start_row, end_row, col, 0);
	start_row = end_row + 1;
      }
    }
  }
  if (flags & (BOX | ALLBOX | DOUBLEBOX)) {
    add_vertical_rule(0, nrows - 1, 0, 0);
    add_vertical_rule(0, nrows - 1, ncolumns, 0);
  }
  for (int end_row = 0; end_row < nrows; end_row++)
    for (col = 0; col < ncolumns+1; col++)
      if (vrule[end_row][col] > 0
	  && !vrule_spanned(end_row, col)
	  && (end_row == nrows - 1
	      || vrule[end_row+1][col] != vrule[end_row][col]
	      || vrule_spanned(end_row+1, col))) {
	int start_row;
	for (start_row = end_row - 1;
	     start_row >= 0
	     && vrule[start_row][col] == vrule[end_row][col]
	     && !vrule_spanned(start_row, col);
	     start_row--)
	  ;
	start_row++;
	add_vertical_rule(start_row, end_row, col, vrule[end_row][col] > 1);
      }
  for (vertical_rule *p = vrule_list; p; p = p->next)
    if (p->is_double)
      for (int r = p->start_row; r <= p->end_row; r++) {
	if (p->col > 0 && entry[r][p->col-1] != 0
	    && entry[r][p->col-1]->end_col == p->col-1) {
	  int is_corner = r == p->start_row || r == p->end_row;
	  entry[r][p->col-1]->note_double_vrule_on_right(is_corner);
	}
	if (p->col < ncolumns && entry[r][p->col] != 0
	    && entry[r][p->col]->start_col == p->col) {
	  int is_corner = r == p->start_row || r == p->end_row;
	  entry[r][p->col]->note_double_vrule_on_left(is_corner);
	}
      }
}

void table::define_bottom_macro()
{
  prints(".\\\" define bottom macro\n");
  prints(".eo\n"
	 // protect # in macro name against eqn
	 ".ig\n"
	 ".EQ\n"
	 "delim off\n"
	 ".EN\n"
	 "..\n"
	 ".de T#\n"
	 ".  if !\\n[" SUPPRESS_BOTTOM_REG "] \\{\\\n"
	 ".    " REPEATED_VPT_MACRO " 0\n"
	 ".    mk " SAVED_VERTICAL_POS_REG "\n");
  if (flags & (BOX | ALLBOX | DOUBLEBOX)) {
    prints(".    if \\n[T.]&\\n[" NEED_BOTTOM_RULE_REG "] \\{\\\n");
    print_single_hrule(0);
    prints(".    \\}\n");
  }
  prints(".    ls 1\n");
  for (vertical_rule *p = vrule_list; p; p = p->next)
    p->contribute_to_bottom_macro(this);
  if (flags & DOUBLEBOX)
    prints(".  if \\n[T.] \\{\\\n"
	   ".    vs " DOUBLE_LINE_SEP ">?\\n[.V]u\n"
	   "\\v'" BODY_DEPTH "'\\s[\\n[" LINESIZE_REG "]]"
	   "\\D'l \\n[TW]u 0'\\s0\n"
	   ".    vs\n"
	   ".  \\}\n"
	   ".  if \\n[" LAST_PASSED_ROW_REG "]>=0 "
	   ".nr " TOP_REG " \\n[#T]-" DOUBLE_LINE_SEP "\n"
	   ".  sp -1\n"
	   "\\v'" BODY_DEPTH "'\\s[\\n[" LINESIZE_REG "]]"
	   "\\D'l 0 |\\n[" TOP_REG "]u-1v'\\s0\n"
	   ".  sp -1\n"
	   "\\v'" BODY_DEPTH "'\\h'|\\n[TW]u'\\s[\\n[" LINESIZE_REG "]]"
	   "\\D'l 0 |\\n[" TOP_REG "]u-1v'\\s0\n");
  prints(".    ls\n");
  prints(".    nr " LAST_PASSED_ROW_REG " \\n[" CURRENT_ROW_REG "]\n"
	 ".    sp |\\n[" SAVED_VERTICAL_POS_REG "]u\n"
	 ".    " REPEATED_VPT_MACRO " 1\n");
  if ((flags & NOKEEP) && (flags & (BOX | DOUBLEBOX | ALLBOX)))
    prints(".    if (\\n% > \\n[" STARTING_PAGE_REG "]) \\{\\\n"
	   ".      tmc \\n[.F]:\\n[.c]: warning:\n"
	   ".      tmc \" boxed, unkept table does not fit on page\n"
	   ".      tm1 \" \\n[" STARTING_PAGE_REG "]\n"
	   ".    \\}\n");
  prints(".  \\}\n"
	 "..\n"
	 ".ig\n"
	 ".EQ\n"
	 "delim on\n"
	 ".EN\n"
	 "..\n"
	 ".ec\n");
}

// is the vertical line before column c in row r horizontally spanned?

int table::vrule_spanned(int r, int c)
{
  assert(r >= 0 && r < nrows && c >= 0 && c < ncolumns + 1);
  return (c != 0 && c != ncolumns && entry[r][c] != 0
	  && entry[r][c]->start_col != c
	  // horizontally spanning lines don't count
	  && entry[r][c]->to_double_line_entry() == 0
	  && entry[r][c]->to_single_line_entry() == 0);
}

int table::row_begins_section(int r)
{
  assert(r >= 0 && r < nrows);
  for (int i = 0; i < ncolumns; i++)
    if (entry[r][i] && entry[r][i]->start_row != r)
      return 0;
  return 1;
}

int table::row_ends_section(int r)
{
  assert(r >= 0 && r < nrows);
  for (int i = 0; i < ncolumns; i++)
    if (entry[r][i] && entry[r][i]->end_row != r)
      return 0;
  return 1;
}

void table::do_row(int r)
{
  printfs(".\\\" do row %1\n", i_to_a(r));
  if (!(flags & NOKEEP) && row_begins_section(r))
    prints(".if \\n[" USE_KEEPS_REG "] ." KEEP_MACRO_NAME "\n");
  bool had_line = false;
  stuff *p;
  for (p = stuff_list; p && p->row < r; p = p->next)
    ;
  for (stuff *p1 = p; p1 && p1->row == r; p1 = p1->next)
    if (!p1->printed && (p1->is_single_line() || p1->is_double_line())) {
      had_line = true;
      break;
    }
  if (!had_line && !row_is_all_lines[r])
    printfs("." REPEATED_MARK_MACRO " %1\n", row_top_reg(r));
  had_line = false;
  for (; p && p->row == r; p = p->next)
    if (!p->printed) {
      p->print(this);
      if (!had_line && (p->is_single_line() || p->is_double_line())) {
	printfs("." REPEATED_MARK_MACRO " %1\n", row_top_reg(r));
	had_line = true;
      }
    }
  // change the row *after* printing the stuff list (which might contain .TH)
  printfs("\\*[" TRANSPARENT_STRING_NAME "].nr " CURRENT_ROW_REG " %1\n",
	  as_string(r));
  if (!had_line && row_is_all_lines[r])
    printfs("." REPEATED_MARK_MACRO " %1\n", row_top_reg(r));
  // we might have had a .TH, for example,  since we last tried
  if (!(flags & NOKEEP) && row_begins_section(r))
    prints(".if \\n[" USE_KEEPS_REG "] ." KEEP_MACRO_NAME "\n");
  printfs(".mk %1\n", row_start_reg(r));
  prints(".mk " BOTTOM_REG "\n"
	 "." REPEATED_VPT_MACRO " 0\n");
  int c;
  int row_is_blank = 1;
  int first_start_row = r;
  for (c = 0; c < ncolumns; c++) {
    table_entry *e = entry[r][c];
    if (e) {
      if (e->end_row == r) {
	e->do_depth();
	if (e->start_row < first_start_row)
	  first_start_row = e->start_row;
	row_is_blank = 0;
      }
      c = e->end_col;
    }
  }
  if (row_is_blank)
    prints(".nr " BOTTOM_REG " +1v\n");
  if (row_is_all_lines[r]) {
    prints(".vs " LINE_SEP);
    if (row_is_all_lines[r] == 2)
      prints("+" DOUBLE_LINE_SEP);
    prints(">?\\n[.V]u\n.ls 1\n");
    prints("\\&");
    prints("\\v'" BODY_DEPTH);
    if (row_is_all_lines[r] == 2)
      prints("-" HALF_DOUBLE_LINE_SEP);
    prints("'");
    for (c = 0; c < ncolumns; c++) {
      table_entry *e = entry[r][c];
      if (e) {
	if (e->end_row == e->start_row)
	  e->to_simple_entry()->simple_print(1);
	c = e->end_col;
      }
    }
    prints("\n");
    prints(".ls\n"
	   ".vs\n");
    prints(".nr " BOTTOM_REG " \\n[" BOTTOM_REG "]>?\\n[.d]\n");
    printfs(".sp |\\n[%1]u\n", row_start_reg(r));
  }
  for (int i = row_is_all_lines[r] ? r - 1 : r;
       i >= first_start_row;
       i--) {
    simple_entry *first = 0;
    for (c = 0; c < ncolumns; c++) {
      table_entry *e = entry[r][c];
      if (e) {
	if (e->end_row == r && e->start_row == i) {
	  simple_entry *simple = e->to_simple_entry();
	  if (simple) {
	    if (!first) {
	      prints(".ta");
	      first = simple;
	    }
	    simple->add_tab();
	  }
	}
	c = e->end_col;
      }
    }
    if (first) {
      prints('\n');
      first->position_vertically();
      first->set_location();
      prints("\\&");
      first->simple_print(0);
      for (c = first->end_col + 1; c < ncolumns; c++) {
	table_entry *e = entry[r][c];
	if (e) {
	  if (e->end_row == r && e->start_row == i) {
	    simple_entry *simple = e->to_simple_entry();
	    if (simple) {
	      if (e->end_row != e->start_row) {
		prints('\n');
		simple->position_vertically();
		prints("\\&");
	      }
	      simple->simple_print(0);
	    }
	  }
	  c = e->end_col;
	}
      }
      prints('\n');
      prints(".nr " BOTTOM_REG " \\n[" BOTTOM_REG "]>?\\n[.d]\n");
      printfs(".sp |\\n[%1]u\n", row_start_reg(r));
    }
  }
  for (c = 0; c < ncolumns; c++) {
    table_entry *e = entry[r][c];
    if (e) {
      if (e->end_row == r && e->to_simple_entry() == 0) {
	e->position_vertically();
	e->print();
	prints(".nr " BOTTOM_REG " \\n[" BOTTOM_REG "]>?\\n[.d]\n");
	printfs(".sp |\\n[%1]u\n", row_start_reg(r));
      }
      c = e->end_col;
    }
  }
  prints("." REPEATED_VPT_MACRO " 1\n"
	 ".sp |\\n[" BOTTOM_REG "]u\n"
	 "\\*[" TRANSPARENT_STRING_NAME "].nr " NEED_BOTTOM_RULE_REG " 1\n");
  if (r != nrows - 1 && (flags & ALLBOX)) {
    print_single_hrule(r + 1);
    prints("\\*[" TRANSPARENT_STRING_NAME "].nr " NEED_BOTTOM_RULE_REG " 0\n");
  }
  if (r != nrows - 1) {
    if (p && p->row == r + 1
	&& (p->is_single_line() || p->is_double_line())) {
      p->print(this);
      prints("\\*[" TRANSPARENT_STRING_NAME "].nr " NEED_BOTTOM_RULE_REG
	     " 0\n");
    }
    int printed_one = 0;
    for (vertical_rule *vr = vrule_list; vr; vr = vr->next)
      if (vr->end_row == r) {
	if (!printed_one) {
	  prints("." REPEATED_VPT_MACRO " 0\n");
	  printed_one = 1;
	}
	vr->print();
      }
    if (printed_one)
      prints("." REPEATED_VPT_MACRO " 1\n");
    if (!(flags & NOKEEP) && row_ends_section(r))
      prints(".if \\n[" USE_KEEPS_REG "] ." RELEASE_MACRO_NAME "\n");
  }
}

void table::do_top()
{
  prints(".\\\" do top\n");
  prints(".ss \\n[" SAVED_INTER_WORD_SPACE_SIZE "]\n");
  prints(".fc \002\003\n");
  if (flags & (BOX | DOUBLEBOX | ALLBOX))
    prints(".nr " IS_BOXED_REG " 1\n");
  else
    prints(".nr " IS_BOXED_REG " 0\n");
  if (!(flags & NOKEEP) && (flags & (BOX | DOUBLEBOX | ALLBOX)))
    prints("." TABLE_KEEP_MACRO_NAME "\n");
  if (flags & DOUBLEBOX) {
    prints(".ls 1\n"
	   ".vs " LINE_SEP ">?\\n[.V]u\n"
	   "\\v'" BODY_DEPTH "'\\s[\\n[" LINESIZE_REG "]]\\D'l \\n[TW]u 0'\\s0\n"
	   ".vs\n"
	   "." REPEATED_MARK_MACRO " " TOP_REG "\n"
	   ".vs " DOUBLE_LINE_SEP ">?\\n[.V]u\n");
    printfs("\\v'" BODY_DEPTH "'"
	    "\\s[\\n[" LINESIZE_REG "]]"
	    "\\h'\\n[%1]u'"
	    "\\D'l |\\n[%2]u 0'"
	    "\\s0"
	    "\n",
	    column_divide_reg(0),
	    column_divide_reg(ncolumns));
    prints(".ls\n"
	   ".vs\n");
  }
  else if (flags & (ALLBOX | BOX))
    print_single_hrule(0);
  // On terminal devices, a vertical rule on the first row of the table
  // will stick out 1v above it if it the table is unboxed or lacks a
  // horizontal rule on the first row.  This is necessary for grotty's
  // rule intersection detection.  We must make room for it so that the
  // vertical rule is not drawn above the top of the page.
  else if ((flags & HAS_TOP_VRULE) && !(flags & HAS_TOP_HRULE)) {
    prints(".if n \\{\\\n");
    prints(".  \\\" Compensate for vertical rule at top of table.\n");
    prints(".  rs\n.  sp\n.\\}\n");
  }
  prints(".nr " STARTING_PAGE_REG " \\n%\n");
  //printfs(".mk %1\n", row_top_reg(0));
}

void table::do_bottom()
{
  prints(".\\\" do bottom\n");
  // print stuff after last row
  for (stuff *p = stuff_list; p; p = p->next)
    if (p->row > nrows - 1)
      p->print(this);
  if (!(flags & NOKEEP))
    prints(".if \\n[" USE_KEEPS_REG "] ." RELEASE_MACRO_NAME "\n");
  printfs(".mk %1\n", row_top_reg(nrows));
  prints(".nr " NEED_BOTTOM_RULE_REG " 1\n"
	 ".nr T. 1\n"
	 // protect # in macro name against eqn
	 ".ig\n"
	 ".EQ\n"
	 "delim off\n"
	 ".EN\n"
	 "..\n"
	 ".T#\n"
	 ".ig\n"
	 ".EQ\n"
	 "delim on\n"
	 ".EN\n"
	 "..\n");
  if (!(flags & NOKEEP) && (flags & (BOX | DOUBLEBOX | ALLBOX)))
    prints("." TABLE_RELEASE_MACRO_NAME "\n");
  if (flags & DOUBLEBOX)
    prints(".sp " DOUBLE_LINE_SEP "\n");
  // Horizontal box lines take up an entire row on nroff devices (maybe
  // a half-row if we ever support [emulators of] devices like the
  // Teletype Model 37 with half-line motions).
  if (flags & (BOX | DOUBLEBOX | ALLBOX))
    prints(".if n .sp\n");
  // Space again for the doublebox option, until we can draw that more
  // attractively; see Savannah #43637.
  if (flags & DOUBLEBOX)
    prints(".if n .sp\n");
  prints("." RESET_MACRO_NAME "\n"
	 ".nn \\n[" SAVED_NUMBERING_SUPPRESSION_COUNT "]\n"
	 ".ie \\n[" SAVED_NUMBERING_ENABLED "] "
	 ".nm \\n[" SAVED_NUMBERING_LINENO "]\n"
	 ".el .nm\n"
	 ".fc\n"
	 ".cp \\n(" COMPATIBLE_REG "\n");
}

int table::get_nrows()
{
  return nrows;
}

const char *last_filename = 0;

void set_troff_location(const char *fn, int ln)
{
  if (!location_force_filename && last_filename != 0
      && strcmp(fn, last_filename) == 0)
    printfs(".lf %1\n", as_string(ln));
  else {
    string filename(fn);
    filename += '\0';
    normalize_for_lf(filename);
    printfs(".lf %1 %2\n", as_string(ln), filename.contents());
    last_filename = fn;
    location_force_filename = 0;
  }
}

void printfs(const char *s, const string &arg1, const string &arg2,
	     const string &arg3, const string &arg4, const string &arg5)
{
  if (s) {
    char c;
    while ((c = *s++) != '\0') {
      if (c == '%') {
	switch (*s++) {
	case '1':
	  prints(arg1);
	  break;
	case '2':
	  prints(arg2);
	  break;
	case '3':
	  prints(arg3);
	  break;
	case '4':
	  prints(arg4);
	  break;
	case '5':
	  prints(arg5);
	  break;
	case '6':
	case '7':
	case '8':
	case '9':
	  break;
	case '%':
	  prints('%');
	  break;
	default:
	  assert(0 == "printfs format character not in [1-9%]");
	}
      }
      else
	prints(c);
    }
  }
}

// Local Variables:
// fill-column: 72
// mode: C++
// End:
// vim: set cindent noexpandtab shiftwidth=2 textwidth=72:
