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

typedef void (*REQUEST_FUNCP)();

class macro;

class request_or_macro : public object {
public:
  request_or_macro();
  virtual void invoke(symbol, bool) = 0;
  virtual macro *to_macro();
};

class request : public request_or_macro {
  REQUEST_FUNCP p;
public:
  void invoke(symbol, bool);
  request(REQUEST_FUNCP);
};

void delete_request_or_macro(request_or_macro *);

extern object_dictionary request_dictionary;

class macro_header;
struct node;

class macro : public request_or_macro {
  const char *filename;		// where was it defined?
  int lineno;
  int len;
  bool is_empty_macro;
  bool is_a_diversion;
  bool is_a_string;		// if it contains no newline
public:
  macro_header *p;
  macro();
  ~macro();
  macro(const macro &);
  macro(bool /* is_div */);
  macro &operator=(const macro &);
  void append(unsigned char);
  void append(node *);
  void append_unsigned(unsigned int);
  void append_int(int);
  void append_str(const char *);
  void set(unsigned char, int);
  unsigned char get(int);
  int length();
  void invoke(symbol, bool);
  macro *to_macro();
  void print_size();
  bool is_empty();
  bool is_diversion();
  bool is_string();
  void clear_string_flag();
  friend class string_iterator;
  friend void chop_macro();
  friend void substring_request();
  friend bool operator==(const macro &, const macro &);
};

extern void init_input_requests();
extern void init_markup_requests();
extern void init_div_requests();
extern void init_node_requests();
extern void init_reg_requests();
extern void init_env_requests();
extern void init_hyphenation_pattern_requests();
extern void init_request(const char *, REQUEST_FUNCP);

class charinfo;
class environment;

node *charinfo_to_node_list(charinfo *, const environment *);

// Local Variables:
// fill-column: 72
// mode: C++
// End:
// vim: set cindent noexpandtab shiftwidth=2 textwidth=72:
