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

struct hyphen_list {
  bool is_hyphen;
  bool is_breakable;
  unsigned char hyphenation_code;
  hyphen_list *next;
  hyphen_list(unsigned char code,
	      hyphen_list * /* p */ = 0 /* nullptr */);
};

void hyphenate(hyphen_list *, unsigned);

enum hyphenation_type {
  HYPHEN_MIDDLE,
  HYPHEN_BOUNDARY,
  HYPHEN_INHIBIT
};

class ascii_output_file;

struct breakpoint;
struct vertical_size;
class charinfo;

class macro;

class troff_output_file;
class tfont;
class environment;

class glyph_node;
class diverted_space_node;
class token_node;

struct node {
  node *next;
  node *last;
  statem *state;
  statem *push_state;
  int div_nest_level;
  bool is_special;
  node();
  node(node *);
  node(node *, statem *, int);
  node *add_char(charinfo *, environment *, hunits *, int *,
		 node ** /* glyph_comp_np */ = 0 /* nullptr */);

  virtual ~node();
  virtual node *copy() = 0;
  virtual bool set_unformat_flag();
  virtual bool causes_tprint() = 0;
  virtual bool is_tag() = 0;
  virtual int get_break_code();
  virtual hunits width();
  virtual hunits subscript_correction();
  virtual hunits italic_correction();
  virtual hunits left_italic_correction();
  virtual hunits skew();
  virtual int nspaces();
  virtual bool did_space_merge(hunits, hunits, hunits);
  virtual vunits vertical_width();
  virtual node *last_char_node();
  virtual void vertical_extent(vunits *, vunits *);
  virtual int character_type();
  virtual void set_vertical_size(vertical_size *);
  virtual int ends_sentence();
  virtual node *merge_self(node *);
  virtual node *add_discretionary_hyphen();
  virtual node *add_self(node *, hyphen_list **);
  virtual hyphen_list *get_hyphen_list(hyphen_list *, int *);
  virtual void ascii_print(ascii_output_file *);
  virtual void asciify(macro *);
  virtual int discardable();
  virtual void spread_space(int *, hunits *);
  virtual void freeze_space();
  virtual void is_escape_colon();
  virtual breakpoint *get_breakpoints(hunits, int,
			      breakpoint * /* rest */ = 0 /* nullptr */,
			      bool /* is_inner */ = false);
  virtual int nbreaks();
  virtual void split(int, node **, node **);
  virtual hyphenation_type get_hyphenation_type();
  virtual int reread(int *);
  virtual token_node *get_token_node();
  virtual int overlaps_vertically();
  virtual int overlaps_horizontally();
  virtual units size();
  virtual int interpret(macro *);

  virtual node *merge_glyph_node(glyph_node *);
  virtual tfont *get_tfont();
  virtual color *get_stroke_color();
  virtual color *get_fill_color();
  virtual void tprint(troff_output_file *);
  virtual void zero_width_tprint(troff_output_file *);

  node *add_italic_correction(hunits *);

  virtual bool is_same_as(node *) = 0;
  virtual const char *type() = 0;
  virtual void dump_node();
  virtual void dump_node_list();
};

inline node::node()
: next(0 /* nullptr */), last(0 /* nullptr */),
  state(0 /* nullptr */), push_state(0 /* nullptr */),
  div_nest_level(0), is_special(false)
{
}

inline node::node(node *n)
: next(n), last(0 /* nullptr */),
  state(0 /* nullptr */), push_state(0 /* nullptr */),
  div_nest_level(0), is_special(false)
{
}

inline node::node(node *n, statem *s, int divlevel)
: next(n), last(0 /* nullptr */),
  push_state(0 /* nullptr */),
  div_nest_level(divlevel), is_special(false)
{
  if (s)
    state = new statem(s);
  else
    state = 0 /* nullptr */;
}

inline node::~node()
{
  if (state != 0 /* nullptr */)
    delete state;
  if (push_state != 0 /* nullptr */)
    delete push_state;
}

// three-valued Boolean :-|
// 0 means it doesn't, 1 means it does, 2 means it's transparent
int node_list_ends_sentence(node *);

struct breakpoint {
  breakpoint *next;
  hunits width;
  int nspaces;
  node *nd;
  int index;
  char hyphenated;
};

class line_start_node : public node {
public:
  line_start_node() {}
  node *copy() { return new line_start_node; }
  bool is_same_as(node *);
  bool causes_tprint();
  bool is_tag();
  const char *type();
  void asciify(macro *);
};

class space_node : public node {
private:
protected:
  hunits n;
  char set;
  char was_escape_colon;
  color *col;			/* for grotty */
  space_node(hunits, int, int, color *, statem *, int,
	     node * = 0 /* nullptr */);
public:
  space_node(hunits, color *, node * = 0 /* nullptr */);
  node *copy();
  int nspaces();
  hunits width();
  int discardable();
  bool did_space_merge(hunits, hunits, hunits);
  void freeze_space();
  void is_escape_colon();
  void spread_space(int *, hunits *);
  void tprint(troff_output_file *);
  breakpoint *get_breakpoints(hunits, int,
			      breakpoint * /* rest */ = 0 /* nullptr */,
			      bool /* is_inner */ = false);
  int nbreaks();
  void split(int, node **, node **);
  void ascii_print(ascii_output_file *);
  bool is_same_as(node *);
  void asciify(macro *);
  const char *type();
  bool causes_tprint();
  bool is_tag();
  hyphenation_type get_hyphenation_type();
};

struct width_list {
  hunits width;
  hunits sentence_width;
  width_list *next;
  width_list(hunits, hunits);
  width_list(width_list *);
};

class word_space_node : public space_node {
protected:
  width_list *orig_width;
  bool unformat;
  word_space_node(hunits /* d */, int /* s */, color * /* c */,
		  width_list * /* w */, bool /* flag */,
		  statem * /* st */, int /* divlevel */,
		  node * /* x */ = 0 /* nullptr */);
public:
  word_space_node(hunits, color *, width_list *,
		  node * /* x */ = 0 /* nullptr */);
  ~word_space_node();
  node *copy();
  int reread(int *);
  bool set_unformat_flag();
  void tprint(troff_output_file *);
  bool is_same_as(node *);
  void asciify(macro *);
  const char *type();
  bool did_space_merge(hunits, hunits, hunits);
  bool causes_tprint();
  bool is_tag();
};

class unbreakable_space_node : public word_space_node {
  unbreakable_space_node(hunits, int, color *, statem *, int,
			 node * /* x */ = 0 /* nullptr */);
public:
  unbreakable_space_node(hunits, color *,
			 node * /* x */ = 0 /* nullptr */);
  node *copy();
  int reread(int *);
  void tprint(troff_output_file *);
  bool is_same_as(node *);
  void asciify(macro *);
  const char *type();
  bool causes_tprint();
  bool is_tag();
  breakpoint *get_breakpoints(hunits, int,
			      breakpoint * /* rest */ = 0 /* nullptr */,
			      bool /* is_inner */ = false);
  int nbreaks();
  void split(int, node **, node **);
  bool did_space_merge(hunits, hunits, hunits);
  node *add_self(node *, hyphen_list **);
  hyphen_list *get_hyphen_list(hyphen_list *, int *);
  hyphenation_type get_hyphenation_type();
};

class diverted_space_node : public node {
public:
  vunits n;
  diverted_space_node(vunits, node * /* p */ = 0 /* nullptr */);
  diverted_space_node(vunits, statem *, int,
		      node * /* p */ = 0 /* nullptr */);
  node *copy();
  int reread(int *);
  bool is_same_as(node *);
  const char *type();
  bool causes_tprint();
  bool is_tag();
};

class diverted_copy_file_node : public node {
  symbol filename;
public:
  vunits n;
  diverted_copy_file_node(symbol, node * /* p */ = 0 /* nullptr */);
  diverted_copy_file_node(symbol, statem *, int,
			  node * /* p */ = 0 /* nullptr */);
  node *copy();
  int reread(int *);
  bool is_same_as(node *);
  const char *type();
  bool causes_tprint();
  bool is_tag();
};

class extra_size_node : public node {
  vunits n;
public:
  extra_size_node(vunits);
  extra_size_node(vunits, statem *, int);
  void set_vertical_size(vertical_size *);
  node *copy();
  bool is_same_as(node *);
  const char *type();
  bool causes_tprint();
  bool is_tag();
};

class vertical_size_node : public node {
  vunits n;
public:
  vertical_size_node(vunits, statem *, int);
  vertical_size_node(vunits);
  void set_vertical_size(vertical_size *);
  void asciify(macro *);
  node *copy();
  bool set_unformat_flag();
  bool is_same_as(node *);
  const char *type();
  bool causes_tprint();
  bool is_tag();
};

class hmotion_node : public node {
protected:
  hunits n;
  bool was_tab;
  bool unformat;
  color *col;			/* for grotty */
public:
  hmotion_node(hunits i, color *c, node *nxt = 0 /* nullptr */)
    : node(nxt), n(i), was_tab(false), unformat(false), col(c) {}
  hmotion_node(hunits i, color *c, statem *s, int divlevel,
	       node *nxt = 0 /* nullptr */)
    : node(nxt, s, divlevel), n(i), was_tab(false), unformat(false),
      col(c) {}
  hmotion_node(hunits i, bool flag1, bool flag2, color *c, statem *s,
	       int divlevel, node *nxt = 0 /* nullptr */)
    : node(nxt, s, divlevel), n(i), was_tab(flag1), unformat(flag2),
      col(c) {}
  hmotion_node(hunits i, bool flag1, bool flag2, color *c,
	       node *nxt = 0 /* nullptr */)
    : node(nxt), n(i), was_tab(flag1), unformat(flag2), col(c) {}
  node *copy();
  int reread(int *);
  bool set_unformat_flag();
  void asciify(macro *);
  void tprint(troff_output_file *);
  hunits width();
  void ascii_print(ascii_output_file *);
  bool is_same_as(node *);
  const char *type();
  bool causes_tprint();
  bool is_tag();
  node *add_self(node *, hyphen_list **);
  hyphen_list *get_hyphen_list(hyphen_list *, int *);
  hyphenation_type get_hyphenation_type();
};

class space_char_hmotion_node : public hmotion_node {
public:
  space_char_hmotion_node(hunits, color *,
			  node * /* nxt */ = 0 /* nullptr */);
  space_char_hmotion_node(hunits, color *, statem *, int,
			  node * /* nxt */ = 0 /* nullptr */);
  node *copy();
  void ascii_print(ascii_output_file *);
  void asciify(macro *);
  void tprint(troff_output_file *);
  bool is_same_as(node *);
  const char *type();
  bool causes_tprint();
  bool is_tag();
  node *add_self(node *, hyphen_list **);
  hyphen_list *get_hyphen_list(hyphen_list *, int *);
  hyphenation_type get_hyphenation_type();
};

class vmotion_node : public node {
  vunits n;
  color *col;			/* for grotty */
public:
  vmotion_node(vunits, color *);
  vmotion_node(vunits, color *, statem *, int);
  void tprint(troff_output_file *);
  node *copy();
  vunits vertical_width();
  bool is_same_as(node *);
  const char *type();
  bool causes_tprint();
  bool is_tag();
};

class hline_node : public node {
  hunits x;
  node *n;
public:
  hline_node(hunits, node *, node * /* nxt */ = 0 /* nullptr */);
  hline_node(hunits, node *, statem *, int,
	     node * /* nxt */ = 0 /* nullptr */);
  ~hline_node();
  node *copy();
  hunits width();
  void tprint(troff_output_file *);
  bool is_same_as(node *);
  const char *type();
  bool causes_tprint();
  bool is_tag();
};

class vline_node : public node {
  vunits x;
  node *n;
public:
  vline_node(vunits, node *, node * /* nxt */ = 0 /* nullptr */);
  vline_node(vunits, node *, statem *, int,
	     node * /* nxt */ = 0 /* nullptr */);
  ~vline_node();
  node *copy();
  void tprint(troff_output_file *);
  hunits width();
  vunits vertical_width();
  void vertical_extent(vunits *, vunits *);
  bool is_same_as(node *);
  const char *type();
  bool causes_tprint();
  bool is_tag();
};

class dummy_node : public node {
public:
  dummy_node(node * nd = 0 /* nullptr */) : node(nd) {}
  node *copy();
  bool is_same_as(node *);
  const char *type();
  bool causes_tprint();
  bool is_tag();
  hyphenation_type get_hyphenation_type();
};

class transparent_dummy_node : public node {
public:
  transparent_dummy_node(node * nd = 0 /* nullptr */) : node(nd) {}
  node *copy();
  bool is_same_as(node *);
  const char *type();
  bool causes_tprint();
  bool is_tag();
  int ends_sentence();
  hyphenation_type get_hyphenation_type();
};

class zero_width_node : public node {
  node *n;
public:
  zero_width_node(node *);
  zero_width_node(node *, statem *, int);
  ~zero_width_node();
  node *copy();
  void tprint(troff_output_file *);
  bool is_same_as(node *);
  const char *type();
  bool causes_tprint();
  bool is_tag();
  void append(node *);
  int character_type();
  void vertical_extent(vunits *, vunits *);
};

class left_italic_corrected_node : public node {
  node *n;
  hunits x;
public:
  left_italic_corrected_node(node * /* xx */ = 0 /* nullptr */);
  left_italic_corrected_node(statem *, int,
			     node * /* xx */ = 0 /* nullptr */);
  ~left_italic_corrected_node();
  void tprint(troff_output_file *);
  void ascii_print(ascii_output_file *);
  void asciify(macro *);
  node *copy();
  bool is_same_as(node *);
  const char *type();
  bool causes_tprint();
  bool is_tag();
  hunits width();
  node *last_char_node();
  void vertical_extent(vunits *, vunits *);
  int ends_sentence();
  int overlaps_horizontally();
  int overlaps_vertically();
  hyphenation_type get_hyphenation_type();
  tfont *get_tfont();
  int character_type();
  hunits skew();
  hunits italic_correction();
  hunits subscript_correction();
  hyphen_list *get_hyphen_list(hyphen_list *, int *);
  node *add_self(node *, hyphen_list **);
  node *merge_glyph_node(glyph_node *);
};

class overstrike_node : public node {
  node *list;
  hunits max_width;
public:
  overstrike_node();
  overstrike_node(statem *, int);
  ~overstrike_node();
  node *copy();
  void tprint(troff_output_file *);
  void overstrike(node *);	// add another node to be overstruck
  hunits width();
  bool is_same_as(node *);
  const char *type();
  bool causes_tprint();
  bool is_tag();
  node *add_self(node *, hyphen_list **);
  hyphen_list *get_hyphen_list(hyphen_list *, int *);
  hyphenation_type get_hyphenation_type();
};

class bracket_node : public node {
  node *list;
  hunits max_width;
public:
  bracket_node();
  bracket_node(statem *, int);
  ~bracket_node();
  node *copy();
  void tprint(troff_output_file *);
  void bracket(node *);	// add another node to be overstruck
  hunits width();
  bool is_same_as(node *);
  const char *type();
  bool causes_tprint();
  bool is_tag();
};

class device_extension_node : public node {
  macro mac;
  tfont *tf;
  color *gcol;
  color *fcol;
  bool lacks_command_prefix;
  void tprint_start(troff_output_file *);
  void tprint_char(troff_output_file *, unsigned char);
  void tprint_end(troff_output_file *);
public:
  device_extension_node(const macro & /* m */,
			bool /* lacks_command_prefix */ = false);
  device_extension_node(const macro & /* m */, tfont * /* tf */,
			color * /* gcol */, color * /* fcol */,
			statem * /* s */, int divlevel,
			bool /* lacks_command_prefix */ = false);
  node *copy();
  void tprint(troff_output_file *);
  bool is_same_as(node *);
  const char *type();
  bool causes_tprint();
  bool is_tag();
  int ends_sentence();
  tfont *get_tfont();
};

class suppress_node : public node {
  int is_on;
  int emit_limits;	// must we issue extent of the area written out?
  symbol filename;
  char position;
  int  image_id;
public:
  suppress_node(int, int);
  suppress_node(symbol, char, int);
  suppress_node(int, int, symbol, char, int, statem *, int);
  suppress_node(int, int, symbol, char, int);
  node *copy();
  void tprint(troff_output_file *);
  hunits width();
  bool is_same_as(node *);
  const char *type();
  bool causes_tprint();
  bool is_tag();
private:
  void put(troff_output_file *, const char *);
};

class tag_node : public node {
public:
  string tag_string;
  bool delayed;
  tag_node();
  tag_node(string, int);
  tag_node(string, statem *, int, int);
  node *copy();
  void tprint(troff_output_file *);
  bool is_same_as(node *);
  const char *type();
  bool causes_tprint();
  bool is_tag();
  int ends_sentence(); // three-valued Boolean :-|
};

struct hvpair {
  hunits h;
  vunits v;
  hvpair();
};

class draw_node : public node {
  int npoints;
  font_size sz;
  color *gcol;
  color *fcol;
  char code;
  hvpair *point;
public:
  draw_node(char, hvpair *, int, font_size, color *, color *);
  draw_node(char, hvpair *, int, font_size, color *, color *, statem *,
	    int);
  ~draw_node();
  hunits width();
  vunits vertical_width();
  node *copy();
  void tprint(troff_output_file *);
  bool is_same_as(node *);
  const char *type();
  bool causes_tprint();
  bool is_tag();
};

class charinfo;
node *make_node(charinfo *, environment *);
bool character_exists(charinfo *, environment *);

int same_node_list(node *, node *);
node *reverse_node_list(node *);
void delete_node_list(node *);
node *copy_node_list(node *);

int get_bold_fontno(int);

inline hyphen_list::hyphen_list(unsigned char code, hyphen_list *p)
: is_hyphen(false), is_breakable(false), hyphenation_code(code), next(p)
{
}

extern void read_desc();
extern bool mount_font(int, symbol,
		       symbol /* external_name */ = NULL_SYMBOL);
extern bool is_font_name(symbol, symbol);
extern bool is_abstract_style(symbol);
extern bool mount_style(int, symbol);
extern int is_good_fontno(int);
extern int symbol_fontno(symbol);
extern int next_available_font_position();
extern void init_size_table(int *);
extern int get_underline_fontno();

class output_file {
  char make_g_plus_plus_shut_up;
public:
  output_file();
  bool is_dying;
  virtual ~output_file();
  virtual void trailer(vunits);
  virtual void flush() = 0;
  virtual void transparent_char(unsigned char) = 0;
  virtual void print_line(hunits x, vunits y, node *n,
			  vunits before, vunits after,
			  hunits width) = 0;
  virtual void begin_page(int pageno, vunits page_length) = 0;
  virtual void copy_file(hunits x, vunits y, const char *filename) = 0;
  virtual int is_printing() = 0;
  virtual void put_filename(const char *, int);
  virtual void on();
  virtual void off();
#ifdef COLUMN
  virtual void vjustify(vunits, symbol);
#endif /* COLUMN */
  mtsm state;
};

extern char *pipe_command;

extern output_file *the_output;
extern void init_output();
bool in_output_page_list(int);

class font_family {
  int *map;
  int map_size;
public:
  const symbol nm;
  font_family(symbol);
  ~font_family();
  int make_definite(int);
  static void invalidate_fontno(int);
};

font_family *lookup_family(symbol);
symbol get_font_name(int, environment *);
symbol get_style_name(int);
extern search_path include_search_path;

// Local Variables:
// fill-column: 72
// mode: C++
// End:
// vim: set cindent noexpandtab shiftwidth=2 textwidth=72:
