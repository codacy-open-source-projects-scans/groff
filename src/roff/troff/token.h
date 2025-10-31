/* Copyright (C) 1989-2025 Free Software Foundation, Inc.
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

class charinfo;
struct node;
class vunits;

class token {
  symbol nm;
  node *nd;
  unsigned char c;
  int val;
  units dim;
  enum token_type {
    TOKEN_BACKSPACE,		// ^H
    TOKEN_BEGIN_TRAP,
    TOKEN_CHAR,			// ordinary character
    TOKEN_DUMMY,		// dummy character: \&
    TOKEN_EMPTY,		// this is the initial value
    TOKEN_END_TRAP,
    TOKEN_ESCAPE,		// \e
    TOKEN_HYPHEN_INDICATOR,	// \%
    TOKEN_INTERRUPT,		// \c
    TOKEN_ITALIC_CORRECTION,	// \/
    TOKEN_LEADER,		// ^A
    TOKEN_LEFT_BRACE,		// \{
    TOKEN_MARK_INPUT,		// \k
    TOKEN_NEWLINE,		// ^J
    TOKEN_NODE,
    TOKEN_INDEXED_CHAR,		// \N
    TOKEN_PAGE_EJECTOR,
    TOKEN_REQUEST,
    TOKEN_RIGHT_BRACE,		// \}
    TOKEN_SPACE,		// ' ' -- ordinary space
    TOKEN_SPECIAL_CHAR,	// \(, \[
    TOKEN_SPREAD,		// \p -- break and spread output line
    TOKEN_STRETCHABLE_SPACE,	// \~
    TOKEN_UNSTRETCHABLE_SPACE,	// '\ '
    TOKEN_HORIZONTAL_SPACE,	// horizontal motion: \|, \^, \0, \h
    TOKEN_TAB,			// ^I
    TOKEN_TRANSPARENT,		// \!
    TOKEN_TRANSPARENT_DUMMY,	// \)
    TOKEN_ZERO_WIDTH_BREAK,	// \:
    TOKEN_EOF			// end of file
  } type;
public:
  token();
  ~token();
  token(const token &);
  void operator=(const token &);
  void next();
  void process();
  void skip();
  int nspaces();		// is_space() as integer
  bool is_eof();
  bool is_space();
  bool is_stretchable_space();
  bool is_unstretchable_space();
  bool is_horizontal_space();
  bool is_white_space();
  bool is_character();
  bool is_special_character();
  bool is_indexed_character();
  bool is_newline();
  bool is_tab();
  bool is_leader();
  bool is_backspace();
  bool is_usable_as_delimiter(bool /* report_error */ = false);
  bool is_dummy();
  bool is_transparent_dummy();
  bool is_transparent();
  bool is_left_brace();
  bool is_right_brace();
  bool is_page_ejector();
  bool is_hyphen_indicator();
  bool is_zero_width_break();
  bool operator==(const token &); // for delimiters & conditional exprs
  bool operator!=(const token &); // ditto
  unsigned char ch();
  int character_index();
  charinfo *get_char(bool /* required */ = false,
		     bool /* suppress_creation */ = false);
  bool add_to_zero_width_node_list(node **);
  void make_space();
  void make_newline();
  const char *description();

  friend void process_input_stack();
  friend node *do_overstrike();
};

extern token tok;		// the current token

extern symbol get_name(bool /* required */ = false);
extern symbol get_long_name(bool /* required */ = false);
extern charinfo *read_character(); // TODO?: bool /* required */ = false
extern char *read_rest_of_line_as_argument();
extern void check_missing_character();
extern void skip_line();
extern void handle_initial_title();

enum char_mode {
  CHAR_NORMAL,
  CHAR_FALLBACK,
  CHAR_FONT_SPECIFIC_FALLBACK,
  CHAR_SPECIAL_FALLBACK
};

extern void define_character(char_mode,
			     const char * /* font_name */ = 0 /* nullptr */);

class hunits;
extern void read_title_parts(node **part, hunits *part_width);

extern bool get_number_rigidly(units *result, unsigned char si);

extern bool read_measurement(units *result, unsigned char si);
extern bool get_integer(int *result);

extern bool read_measurement(units *result, unsigned char si,
			     units prev_value);
extern bool get_integer(int *result, int prev_value);

extern void interpolate_register(symbol, int);

const char *asciify(int c);

inline bool token::is_newline()
{
  return type == TOKEN_NEWLINE;
}

inline bool token::is_space()
{
  return type == TOKEN_SPACE;
}

inline bool token::is_stretchable_space()
{
  return type == TOKEN_STRETCHABLE_SPACE;
}

inline bool token::is_unstretchable_space()
{
  return type == TOKEN_UNSTRETCHABLE_SPACE;
}

inline bool token::is_horizontal_space()
{
  return type == TOKEN_HORIZONTAL_SPACE;
}

inline bool token::is_special_character()
{
  return type == TOKEN_SPECIAL_CHAR;
}

inline int token::nspaces()
{
  return (int)(type == TOKEN_SPACE);
}

inline bool token::is_white_space()
{
  return type == TOKEN_SPACE || type == TOKEN_TAB;
}

inline bool token::is_transparent()
{
  return type == TOKEN_TRANSPARENT;
}

inline bool token::is_page_ejector()
{
  return type == TOKEN_PAGE_EJECTOR;
}

inline unsigned char token::ch()
{
  return type == TOKEN_CHAR ? c : '\0';
}

inline bool token::is_character()
{
  return (TOKEN_CHAR == type) || (TOKEN_SPECIAL_CHAR == type)
	  || (TOKEN_INDEXED_CHAR == type);
}

inline bool token::is_indexed_character()
{
  return TOKEN_INDEXED_CHAR == type;
}

inline int token::character_index()
{
  assert(TOKEN_INDEXED_CHAR == type);
  return val;
}

inline bool token::is_eof()
{
  return type == TOKEN_EOF;
}

inline bool token::is_dummy()
{
  return type == TOKEN_DUMMY;
}

inline bool token::is_transparent_dummy()
{
  return type == TOKEN_TRANSPARENT_DUMMY;
}

inline bool token::is_left_brace()
{
  return type == TOKEN_LEFT_BRACE;
}

inline bool token::is_right_brace()
{
  return type == TOKEN_RIGHT_BRACE;
}

inline bool token::is_tab()
{
  return type == TOKEN_TAB;
}

inline bool token::is_leader()
{
  return type == TOKEN_LEADER;
}

inline bool token::is_backspace()
{
  return type == TOKEN_BACKSPACE;
}

inline bool token::is_hyphen_indicator()
{
  return type == TOKEN_HYPHEN_INDICATOR;
}

inline bool token::is_zero_width_break()
{
  return type == TOKEN_ZERO_WIDTH_BREAK;
}

bool has_arg(bool /* want_peek */ = false);

// Local Variables:
// fill-column: 72
// mode: C++
// End:
// vim: set cindent noexpandtab shiftwidth=2 textwidth=72:
