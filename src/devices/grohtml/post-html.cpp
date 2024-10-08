/* Copyright (C) 2000-2020 Free Software Foundation, Inc.
 *
 *  Gaius Mulley (gaius@glam.ac.uk) wrote post-html.cpp
 *  but it owes a huge amount of ideas and raw code from
 *  James Clark (jjc@jclark.com) grops/ps.cpp.
 */

/*
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

#include "driver.h"
#include "stringclass.h"
#include "cset.h"
#include "html.h"
#include "html-text.h"
#include "html-table.h"
#include "curtime.h"

#include <time.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <stdio.h>
#include <fcntl.h>
#include <string.h>

extern "C" const char *Version_string;

#if !defined(TRUE)
#   define TRUE  (1==1)
#endif
#if !defined(FALSE)
#   define FALSE (1==0)
#endif

#define MAX_LINE_LENGTH    60        /* maximum characters we want in a line      */
#define SIZE_INCREMENT     2         /* font size increment <big> = +2            */
#define CENTER_TOLERANCE   2         /* how many pixels off center do we allow    */
#define ANCHOR_TEMPLATE    "heading" /* if simple anchor is set we use this       */
#define UNICODE_DESC_START 0x80      /* all character entities above this are     */
                                     /* either encoded by their glyph names or if */
                                     /* there is no name then we use &#nnn;       */
typedef enum {CENTERED, LEFT, RIGHT, INLINE} TAG_ALIGNMENT;
typedef enum {col_tag, tab_tag, tab0_tag, none} colType;

#undef DEBUG_TABLES
// #define DEBUG_TABLES

/*
 *  prototypes
 */

const char *get_html_translation (font *f, const string &name);
static const char *get_html_entity(unsigned int code);
int char_translate_to_html (font *f, char *buf, int buflen, unsigned char ch, int b, int and_single);


static int auto_links = TRUE;               /* by default we enable automatic links at  */
                                            /* top of the document.                     */
static int auto_rule  = TRUE;               /* by default we enable an automatic rule   */
                                            /* at the top and bottom of the document    */
static int simple_anchors = FALSE;          /* default to anchors with heading text     */
static int manufacture_headings = FALSE;    /* default is to use the Hn html headings,  */
                                            /* rather than manufacture our own.         */
static int do_write_creator_comment = TRUE; /* write Creator HTML comment               */
static int do_write_date_comment = TRUE;    /* write CreationDate HTML comment          */
static color *default_background = 0;       /* has user requested initial bg color?     */
static string job_name;                     /* if set then the output is split into     */
                                            /* multiple files with 'job_name'-%d.html   */
static int multiple_files = FALSE;          /* must we the output be divided into       */
                                            /* multiple html files, one for each        */
                                            /* heading?                                 */
static int base_point_size = 0;             /* which troff font size maps onto html     */
                                            /* size 3?                                  */
static int split_level = 2;                 /* what heading level to split at?          */
static string head_info;                    /* user supplied information to be placed   */
                                            /* into <head> </head>                      */
static int valid_flag = FALSE;              /* has user requested a valid flag at the   */
                                            /* end of each page?                        */
static int groff_sig = FALSE;               /* "This document was produced using"       */
html_dialect dialect = html4;               /* which html dialect should grohtml output */


/*
 *  start with a few favorites
 */

void stop () {}

static int min (int a, int b)
{
  if (a < b)
    return a;
  else
    return b;
}

static int max (int a, int b)
{
  if (a > b)
    return a;
  else
    return b;
}

/*
 *  is_intersection - returns TRUE if range a1..a2 intersects with
 *                    b1..b2
 */

static int is_intersection (int a1, int a2, int b1, int b2)
{
  // easier to prove NOT outside limits
  return ! ((a1 > b2) || (a2 < b1));
}

/*
 *  is_digit - returns TRUE if character, ch, is a digit.
 */

static int is_digit (char ch)
{
  return (ch >= '0') && (ch <= '9');
}

/*
 *  the classes and methods for maintaining a list of files.
 */

struct file {
  FILE    *fp;
  file    *next;
  int      new_output_file;
  int      require_links;
  string   output_file_name;

  file     (FILE *f);
};

/*
 *  file - initialize all fields to null pointers
 */

file::file (FILE *f)
  : fp(f), next(0), new_output_file(FALSE),
    require_links(FALSE), output_file_name("")
{
}

class files {
public:
              files              ();
  FILE       *get_file           (void);
  void        start_of_list      (void);
  void        move_next          (void);
  void        add_new_file       (FILE *f);
  void        set_file_name      (string name);
  void        set_links_required (void);
  int         are_links_required (void);
  int         is_new_output_file (void);
  string      file_name          (void);
  string      next_file_name     (void);
private:
  file       *head;
  file       *tail;
  file       *ptr;
};

/*
 *  files - create an empty list of files.
 */

files::files ()
  : head(0), tail(0), ptr(0)
{
}

/*
 *  get_file - returns the FILE associated with ptr.
 */

FILE *files::get_file (void)
{
  if (ptr)
    return ptr->fp;
  else
    return 0;
}

/*
 *  start_of_list - reset the ptr to the start of the list.
 */

void files::start_of_list (void)
{
  ptr = head;
}

/*
 *  move_next - moves the ptr to the next element on the list.
 */

void files::move_next (void)
{
  if (ptr != 0)
    ptr = ptr->next;
}

/*
 *  add_new_file - adds a new file, f, to the list.
 */

void files::add_new_file (FILE *f)
{
  if (0 /* nullptr */ == head) {
    head = new file(f);
    tail = head;
  } else {
    tail->next = new file(f);
    tail       = tail->next;
  }
  ptr = tail;
}

/*
 *  set_file_name - sets the final file name to contain the html
 *                  data to name.
 */

void files::set_file_name (string name)
{
  if (ptr != 0) {
    ptr->output_file_name = name;
    ptr->new_output_file = TRUE;
  }
}

/*
 *  set_links_required - issue links when processing this component
 *                       of the file.
 */

void files::set_links_required (void)
{
  if (ptr != 0)
    ptr->require_links = TRUE;
}

/*
 *  are_links_required - returns TRUE if this section of the file
 *                       requires that links should be issued.
 */

int files::are_links_required (void)
{
  if (ptr != 0)
    return ptr->require_links;
  return FALSE;
}

/*
 *  is_new_output_file - returns TRUE if this component of the file
 *                       is the start of a new output file.
 */

int files::is_new_output_file (void)
{
  if (ptr != 0)
    return ptr->new_output_file;
  return FALSE;
}

/*
 *  file_name - returns the name of the file.
 */

string files::file_name (void)
{
  if (ptr != 0)
    return ptr->output_file_name;
  return string("");
}

/*
 *  next_file_name - returns the name of the next file.
 */

string files::next_file_name (void)
{
  if (ptr != 0 && ptr->next != 0)
    return ptr->next->output_file_name;
  return string("");
}

/*
 *  the class and methods for styles
 */

struct style {
  font        *f;
  int          point_size;
  int          font_no;
  int          height;
  int          slant;
  color        col;
               style       ();
               style       (font *, int, int, int, int, color);
  int          operator == (const style &) const;
  int          operator != (const style &) const;
};

style::style()
  : f(0), point_size(-1)
{
}

style::style(font *p, int sz, int h, int sl, int no, color c)
  : f(p), point_size(sz), font_no(no), height(h), slant(sl), col(c)
{
}

int style::operator==(const style &s) const
{
  return (f == s.f && point_size == s.point_size
	  && height == s.height && slant == s.slant && col == s.col);
}

int style::operator!=(const style &s) const
{
  return !(*this == s);
}

/*
 *  the class and methods for retaining ascii text
 */

struct char_block {
  enum { SIZE = 256 };
  char         *buffer;
  int           used;
  char_block   *next;

  char_block();
  char_block(int length);
  ~char_block();
};

char_block::char_block()
: buffer(0), used(0), next(0)
{
}

char_block::char_block(int length)
: used(0), next(0)
{
  buffer = new char[max(length, char_block::SIZE)];
  if (0 /* nullptr */ == buffer)
    fatal("out of memory error");
}

char_block::~char_block()
{
  if (buffer != 0)
    delete[] buffer;
}

class char_buffer {
public:
  char_buffer();
  ~char_buffer();
  char  *add_string(const char *, unsigned int);
  char  *add_string(const string &);
private:
  char_block *head;
  char_block *tail;
};

char_buffer::char_buffer()
: head(0), tail(0)
{
}

char_buffer::~char_buffer()
{
  while (head != 0) {
    char_block *temp = head;
    head = head->next;
    delete temp;
  }
}

char *char_buffer::add_string (const char *s, unsigned int length)
{
  int i = 0;
  unsigned int old_used;

  if (0 /* nullptr */ == s|| length == 0)
    return 0;

  if (0 /* nullptr */ == tail) {
    tail = new char_block(length+1);
    head = tail;
  } else {
    if (tail->used + length+1 > char_block::SIZE) {
      tail->next  = new char_block(length+1);
      tail        = tail->next;
    }
  }

  old_used = tail->used;
  do {
    tail->buffer[tail->used] = s[i];
    tail->used++;
    i++;
    length--;
  } while (length>0);

  // add terminating nul character

  tail->buffer[tail->used] = '\0';
  tail->used++;

  // and return start of new string

  return &tail->buffer[old_used];
}

char *char_buffer::add_string (const string &s)
{
  return add_string(s.contents(), s.length());
}

/*
 *  the classes and methods for maintaining glyph positions.
 */

class text_glob {
public:
  void text_glob_html      (style *s, char *str, int length,
			    int min_vertical, int min_horizontal,
			    int max_vertical, int max_horizontal);
  void text_glob_special   (style *s, char *str, int length,
			    int min_vertical, int min_horizontal,
			    int max_vertical, int max_horizontal);
  void text_glob_line      (style *s,
			    int min_vertical, int min_horizontal,
			    int max_vertical, int max_horizontal,
			    int thickness);
  void text_glob_auto_image(style *s, char *str, int length,
			    int min_vertical, int min_horizontal,
			    int max_vertical, int max_horizontal);
  void text_glob_tag       (style *s, char *str, int length,
			    int min_vertical, int min_horizontal,
			    int max_vertical, int max_horizontal);

  text_glob                (void);
  ~text_glob               (void);
  int  is_a_line           (void);
  int  is_a_tag            (void);
  int  is_eol              (void);
  int  is_auto_img         (void);
  int  is_br               (void);
  int  is_in               (void);
  int  is_po               (void);
  int  is_ti               (void);
  int  is_ll               (void);
  int  is_ce               (void);
  int  is_tl               (void);
  int  is_eo_tl            (void);
  int  is_eol_ce           (void);
  int  is_col              (void);
  int  is_tab              (void);
  int  is_tab0             (void);
  int  is_ta               (void);
  int  is_tab_ts           (void);
  int  is_tab_te           (void);
  int  is_nf               (void);
  int  is_fi               (void);
  int  is_eo_h             (void);
  int  get_arg             (void);
  int  get_tab_args        (char *align);

  void        remember_table (html_table *t);
  html_table *get_table      (void);

  style           text_style;
  const char     *text_string;
  unsigned int    text_length;
  int             minv, minh, maxv, maxh;
  int             is_tag;               // is this a .br, .sp, .tl etc
  int             is_img_auto;          // image created by eqn delim
  int             is_special;           // text has come via 'x X html:'
  int             is_line;              // is the command a <line>?
  int             thickness;            // the thickness of a line
  html_table     *tab;                  // table description

private:
  text_glob           (style *s, const char *str, int length,
		       int min_vertical , int min_horizontal,
		       int max_vertical , int max_horizontal,
		       bool is_troff_command,
		       bool is_auto_image, bool is_special_command,
		       bool is_a_line    , int  thickness);
};

text_glob::text_glob (style *s, const char *str, int length,
		      int min_vertical, int min_horizontal,
		      int max_vertical, int max_horizontal,
		      bool is_troff_command,
		      bool is_auto_image, bool is_special_command,
		      bool is_a_line_flag, int line_thickness)
  : text_style(*s), text_string(str), text_length(length),
    minv(min_vertical), minh(min_horizontal), maxv(max_vertical),
    maxh(max_horizontal), is_tag(is_troff_command),
    is_img_auto(is_auto_image), is_special(is_special_command),
    is_line(is_a_line_flag), thickness(line_thickness), tab(0)
{
}

text_glob::text_glob ()
  : text_string(0), text_length(0), minv(-1), minh(-1), maxv(-1),
    maxh(-1), is_tag(FALSE), is_special(FALSE), is_line(FALSE),
    thickness(0), tab(0)
{
}

text_glob::~text_glob ()
{
  if (tab != 0)
    delete tab;
}

/*
 *  text_glob_html - used to place html text into the glob buffer.
 */

void text_glob::text_glob_html (style *s, char *str, int length,
				int min_vertical , int min_horizontal,
				int max_vertical , int max_horizontal)
{
  text_glob *g = new text_glob(s, str, length,
			       min_vertical, min_horizontal,
			       max_vertical, max_horizontal,
			       FALSE, FALSE, FALSE, FALSE, 0);
  *this = *g;
  delete g;
}

/*
 *  text_glob_html - used to place html specials into the glob buffer.
 *                   This text is essentially html commands coming
 *                   through from the macro sets, with special
 *                   designated sequences of characters translated into
 *                   html. See add_and_encode.
 */

void text_glob::text_glob_special (style *s, char *str, int length,
				   int min_vertical, int min_horizontal,
				   int max_vertical, int max_horizontal)
{
  text_glob *g = new text_glob(s, str, length,
			       min_vertical, min_horizontal,
			       max_vertical, max_horizontal,
			       FALSE, FALSE, TRUE, FALSE, 0);
  *this = *g;
  delete g;
}

/*
 *  text_glob_line - record horizontal draw line commands.
 */

void text_glob::text_glob_line (style *s,
				int min_vertical, int min_horizontal,
				int max_vertical, int max_horizontal,
				int thickness_value)
{
  text_glob *g = new text_glob(s, "", 0,
			       min_vertical, min_horizontal,
			       max_vertical, max_horizontal,
			       FALSE, FALSE, FALSE, TRUE,
			       thickness_value);
  *this = *g;
  delete g;
}

/*
 *  text_glob_auto_image - record the presence of a .auto-image tag
 *                         command.  Used to mark that an image has been
 *                         created automatically by a preprocessor and
 *                         (pre-grohtml/troff) combination.  Under some
 *                         circumstances images may not be created.
 *                         (consider .EQ
 *                                   delim $$
 *                                   .EN
 *                                   .TS
 *                                   tab(!), center;
 *                                   l!l.
 *                                   $1 over x$!recripical of x
 *                                   .TE
 *                          the first auto-image marker is created via
 *                          .EQ/.EN pair and no image is created.  The
 *                          second auto-image marker occurs at $1 over
 *                          x$ Currently this image will not be created
 *                          as the whole of the table is created as an
 *                          image.  (Once html tables are handled by
 *                          grohtml this will change.  Shortly this will
 *                          be the case).
 */

void text_glob::text_glob_auto_image(style *s, char *str, int length,
				     int min_vertical,
				     int min_horizontal,
				     int max_vertical,
				     int max_horizontal)
{
  text_glob *g = new text_glob(s, str, length,
			       min_vertical, min_horizontal,
			       max_vertical, max_horizontal,
			       TRUE, TRUE, FALSE, FALSE, 0);
  *this = *g;
  delete g;
}

/*
 *  text_glob_tag - records a troff tag.
 */

void text_glob::text_glob_tag (style *s, char *str, int length,
			       int min_vertical, int min_horizontal,
			       int max_vertical, int max_horizontal)
{
  text_glob *g = new text_glob(s, str, length,
			       min_vertical, min_horizontal,
			       max_vertical, max_horizontal,
			       TRUE, FALSE, FALSE, FALSE, 0);
  *this = *g;
  delete g;
}

/*
 *  is_a_line - returns TRUE if glob should be converted into an <hr>
 */

int text_glob::is_a_line (void)
{
  return is_line;
}

/*
 *  is_a_tag - returns TRUE if glob contains a troff directive.
 */

int text_glob::is_a_tag (void)
{
  return is_tag;
}

/*
 *  is_eol - returns TRUE if glob contains the tag eol
 */

int text_glob::is_eol (void)
{
  return is_tag && (strcmp(text_string, "devtag:.eol") == 0);
}

/*
 *  is_eol_ce - returns TRUE if glob contains the tag eol.ce
 */

int text_glob::is_eol_ce (void)
{
  return is_tag && (strcmp(text_string, "devtag:eol.ce") == 0);
}

/*
 *  is_tl - returns TRUE if glob contains the tag .tl
 */

int text_glob::is_tl (void)
{
  return is_tag && (strcmp(text_string, "devtag:.tl") == 0);
}

/*
 *  is_eo_tl - returns TRUE if glob contains the tag eo.tl
 */

int text_glob::is_eo_tl (void)
{
  return is_tag && (strcmp(text_string, "devtag:.eo.tl") == 0);
}

/*
 *  is_nf - returns TRUE if glob contains the tag .fi 0
 */

int text_glob::is_nf (void)
{
  return is_tag && (strncmp(text_string, "devtag:.fi",
			    strlen("devtag:.fi")) == 0) &&
         (get_arg() == 0);
}

/*
 *  is_fi - returns TRUE if glob contains the tag .fi 1
 */

int text_glob::is_fi (void)
{
  return (is_tag && (strncmp(text_string, "devtag:.fi",
			     strlen("devtag:.fi")) == 0) &&
	  (get_arg() == 1));
}

/*
 *  is_eo_h - returns TRUE if glob contains the tag .eo.h
 */

int text_glob::is_eo_h (void)
{
  return is_tag && (strcmp(text_string, "devtag:.eo.h") == 0);
}

/*
 *  is_ce - returns TRUE if glob contains the tag .ce
 */

int text_glob::is_ce (void)
{
  return is_tag && (strncmp(text_string, "devtag:.ce",
			    strlen("devtag:.ce")) == 0);
}

/*
 *  is_in - returns TRUE if glob contains the tag .in
 */

int text_glob::is_in (void)
{
  return is_tag && (strncmp(text_string, "devtag:.in ",
			    strlen("devtag:.in ")) == 0);
}

/*
 *  is_po - returns TRUE if glob contains the tag .po
 */

int text_glob::is_po (void)
{
  return is_tag && (strncmp(text_string, "devtag:.po ",
			    strlen("devtag:.po ")) == 0);
}

/*
 *  is_ti - returns TRUE if glob contains the tag .ti
 */

int text_glob::is_ti (void)
{
  return is_tag && (strncmp(text_string, "devtag:.ti ",
			    strlen("devtag:.ti ")) == 0);
}

/*
 *  is_ll - returns TRUE if glob contains the tag .ll
 */

int text_glob::is_ll (void)
{
  return is_tag && (strncmp(text_string, "devtag:.ll ",
			    strlen("devtag:.ll ")) == 0);
}

/*
 *  is_col - returns TRUE if glob contains the tag .col
 */

int text_glob::is_col (void)
{
  return is_tag && (strncmp(text_string, "devtag:.col",
			    strlen("devtag:.col")) == 0);
}

/*
 *  is_tab_ts - returns TRUE if glob contains the tag .tab_ts
 */

int text_glob::is_tab_ts (void)
{
  return is_tag && (strcmp(text_string, "devtag:.tab-ts") == 0);
}

/*
 *  is_tab_te - returns TRUE if glob contains the tag .tab_te
 */

int text_glob::is_tab_te (void)
{
  return is_tag && (strcmp(text_string, "devtag:.tab-te") == 0);
}

/*
 *  is_ta - returns TRUE if glob contains the tag .ta
 */

int text_glob::is_ta (void)
{
  return is_tag && (strncmp(text_string, "devtag:.ta ",
			    strlen("devtag:.ta ")) == 0);
}

/*
 *  is_tab - returns TRUE if glob contains the tag tab
 */

int text_glob::is_tab (void)
{
  return is_tag && (strncmp(text_string, "devtag:tab ",
			    strlen("devtag:tab ")) == 0);
}

/*
 *  is_tab0 - returns TRUE if glob contains the tag tab0
 */

int text_glob::is_tab0 (void)
{
  return is_tag && (strncmp(text_string, "devtag:tab0",
			    strlen("devtag:tab0")) == 0);
}

/*
 *  is_auto_img - returns TRUE if the glob contains an automatically
 *                generated image.
 */

int text_glob::is_auto_img (void)
{
  return is_img_auto;
}

/*
 *  is_br - returns TRUE if the glob is a tag containing a .br
 *          or an implied .br. Note that we do not include .nf or .fi
 *          as grohtml will place a .br after these commands if they
 *          should break the line.
 */

int text_glob::is_br (void)
{
  return is_a_tag() && ((strcmp ("devtag:.br", text_string) == 0) ||
			(strncmp("devtag:.sp", text_string,
				 strlen("devtag:.sp")) == 0));
}

int text_glob::get_arg (void)
{
  if (strncmp("devtag:", text_string, strlen("devtag:")) == 0) {
    const char *p = text_string;

    while ((*p != (char)0) && (!isspace(*p)))
      p++;
    while ((*p != (char)0) && (isspace(*p)))
      p++;
    if (*p == (char)0)
      return -1;
    return atoi(p);
  }
  return -1;
}

/*
 *  get_tab_args - returns the tab position and alignment of the tab tag
 */

int text_glob::get_tab_args (char *align)
{
  if (strncmp("devtag:", text_string, strlen("devtag:")) == 0) {
    const char *p = text_string;

    // firstly the alignment C|R|L
    while ((*p != (char)0) && (!isspace(*p)))
      p++;
    while ((*p != (char)0) && (isspace(*p)))
      p++;
    *align = *p;
    // now the int value
    while ((*p != (char)0) && (!isspace(*p)))
      p++;
    while ((*p != (char)0) && (isspace(*p)))
      p++;
    if (*p == (char)0)
      return -1;
    return atoi(p);
  }
  return -1;
}

/*
 *  remember_table - saves table, t, in the text_glob.
 */

void text_glob::remember_table (html_table *t)
{
  if (tab != 0)
    delete tab;
  tab = t;
}

/*
 *  get_table - returns the stored table description.
 */

html_table *text_glob::get_table (void)
{
  return tab;
}

/*
 *  the class and methods used to construct ordered double linked
 *  lists.  In a previous implementation we used templates via
 *  #include "ordered-list.h", but this does assume that all C++
 *  compilers can handle this feature. Pragmatically it is safer to
 *  assume this is not the case.
 */

struct element_list {
  element_list *right;
  element_list *left;
  text_glob    *datum;
  int           lineno;
  int           minv, minh, maxv, maxh;

  element_list  (text_glob *d,
		 int line_number,
		 int min_vertical, int min_horizontal,
		 int max_vertical, int max_horizontal);
  element_list  ();
  ~element_list ();
};

element_list::element_list ()
  : right(0), left(0), datum(0), lineno(0), minv(-1), minh(-1),
    maxv(-1), maxh(-1)
{
}

/*
 *  element_list - create a list element assigning the datum and region
 *                 parameters.
 */

element_list::element_list (text_glob *in,
			    int line_number,
			    int min_vertical, int min_horizontal,
			    int max_vertical, int max_horizontal)
  : right(0), left(0), datum(in), lineno(line_number),
    minv(min_vertical), minh(min_horizontal),
    maxv(max_vertical), maxh(max_horizontal)
{
}

element_list::~element_list ()
{
  if (datum != 0)
    delete datum;
}

class list {
public:
       list             ();
      ~list             ();
  int  is_less          (element_list *a, element_list *b);
  void add              (text_glob *in,
			 int line_number,
			 int min_vertical, int min_horizontal,
			 int max_vertical, int max_horizontal);
  void                  sub_move_right      (void);
  void                  move_right          (void);
  void                  move_left           (void);
  int                   is_empty            (void);
  int                   is_equal_to_tail    (void);
  int                   is_equal_to_head    (void);
  void                  start_from_head     (void);
  void                  start_from_tail     (void);
  void                  insert              (text_glob *in);
  void                  move_to             (text_glob *in);
  text_glob            *move_right_get_data (void);
  text_glob            *move_left_get_data  (void);
  text_glob            *get_data            (void);
private:
  element_list *head;
  element_list *tail;
  element_list *ptr;
};

/*
 *  list - construct an empty list.
 */

list::list ()
  : head(0), tail(0), ptr(0)
{
}

/*
 *  ~list - destroy a complete list.
 */

list::~list()
{
  element_list *temp=head;

  do {
    temp = head;
    if (temp != 0) {
      head = head->right;
      delete temp;
    }
  } while ((head != 0) && (head != tail));
}

/*
 *  is_less - returns TRUE if a is left of b if on the same line or
 *            if a is higher up the page than b.
 */

int list::is_less (element_list *a, element_list *b)
{
  // was:
  // if (is_intersection(a->minv+1, a->maxv-1, b->minv+1, b->maxv-1)) {
  if (a->lineno < b->lineno) {
    return  TRUE;
  } else if (a->lineno > b->lineno) {
    return  FALSE;
  } else if (is_intersection(a->minv, a->maxv, b->minv, b->maxv)) {
    return (a->minh < b->minh);
  } else {
    return (a->maxv < b->maxv);
  }
}

/*
 *  add - adds a datum to the list in the order specified by the
 *        region position.
 */

void list::add (text_glob *in, int line_number,
		int min_vertical, int min_horizontal,
		int max_vertical, int max_horizontal)
{
  // create a new list element with datum and position fields
  // initialized
  element_list *t    = new element_list(in, line_number,
					min_vertical, min_horizontal,
					max_vertical, max_horizontal);
  element_list *last;

#if 0
  fprintf(stderr, "[%s %d,%d,%d,%d] ",
	  in->text_string, min_vertical, min_horizontal,
	  max_vertical, max_horizontal);
  fflush(stderr);
#endif

  if (0 /* nullptr */ == head) {
    head     = t;
    tail     = t;
    ptr      = t;
    t->left  = t;
    t->right = t;
  } else {
    last = tail;

    while ((last != head) && (is_less(t, last)))
      last = last->left;

    if (is_less(t, last)) {
      t->right          = last;
      last->left->right = t;
      t->left           = last->left;
      last->left        = t;
      // now check for a new head
      if (last == head)
	head = t;
    } else {
      // add t beyond last
      t->right          = last->right;
      t->left           = last;
      last->right->left = t;
      last->right       = t;
      // now check for a new tail
      if (last == tail)
	tail = t;
    }
  }
}

/*
 *  sub_move_right - removes the element which is currently pointed to
 *                   by ptr from the list and moves ptr to the right.
 */

void list::sub_move_right (void)
{
  element_list *t=ptr->right;

  if (head == tail) {
    head = 0;
    if (tail != 0)
      delete tail;

    tail = 0;
    ptr  = 0;
  } else {
    if (head == ptr)
      head = head->right;
    if (tail == ptr)
      tail = tail->left;
    ptr->left->right = ptr->right;
    ptr->right->left = ptr->left;
    ptr = t;
  }
}

/*
 *  start_from_head - assigns ptr to the head.
 */

void list::start_from_head (void)
{
  ptr = head;
}

/*
 *  start_from_tail - assigns ptr to the tail.
 */

void list::start_from_tail (void)
{
  ptr = tail;
}

/*
 *  is_empty - returns TRUE if the list has no elements.
 */

int list::is_empty (void)
{
  return 0 /* nullptr */ == head;
}

/*
 *  is_equal_to_tail - returns TRUE if the ptr equals the tail.
 */

int list::is_equal_to_tail (void)
{
  return ptr == tail;
}

/*
 *  is_equal_to_head - returns TRUE if the ptr equals the head.
 */

int list::is_equal_to_head (void)
{
  return ptr == head;
}

/*
 *  move_left - moves the ptr left.
 */

void list::move_left (void)
{
  ptr = ptr->left;
}

/*
 *  move_right - moves the ptr right.
 */

void list::move_right (void)
{
  ptr = ptr->right;
}

/*
 *  get_datum - returns the datum referenced via ptr.
 */

text_glob* list::get_data (void)
{
  return ptr->datum;
}

/*
 *  move_right_get_data - returns the datum referenced via ptr and moves
 *                        ptr right.
 */

text_glob* list::move_right_get_data (void)
{
  ptr = ptr->right;
  if (ptr == head)
    return 0;
  else
    return ptr->datum;
}

/*
 *  move_left_get_data - returns the datum referenced via ptr and moves
 *                       ptr right.
 */

text_glob* list::move_left_get_data (void)
{
  ptr = ptr->left;
  if (ptr == tail)
    return 0;
  else
    return ptr->datum;
}

/*
 *  insert - inserts data after the current position.
 */

void list::insert (text_glob *in)
{
  if (is_empty())
    fatal("list must not be empty if we are inserting data");
  else {
    if (0 /* nullptr */ == ptr)
      ptr = head;

    element_list *t = new element_list(in, ptr->lineno,
				       ptr->minv, ptr->minh,
				       ptr->maxv, ptr->maxh);
    if (ptr == tail)
      tail = t;
    ptr->right->left = t;
    t->right = ptr->right;
    ptr->right = t;
    t->left = ptr;
  }
}

/*
 *  move_to - moves the current position to the point where data, in,
 *            exists.  This is an expensive method and should be used
 *            sparingly.
 */

void list::move_to (text_glob *in)
{
  ptr = head;
  while (ptr != tail && ptr->datum != in)
    ptr = ptr->right;
}

/*
 *  page class and methods
 */

class page {
public:
              page             (void);
  void        add             (style *s, const string &str,
			       int line_number,
			       int min_vertical, int min_horizontal,
			       int max_vertical, int max_horizontal);
  void        add_tag         (style *s, const string &str,
			       int line_number,
			       int min_vertical, int min_horizontal,
			       int max_vertical, int max_horizontal);
  void        add_and_encode  (style *s, const string &str,
			       int line_number,
			       int min_vertical, int min_horizontal,
			       int max_vertical, int max_horizontal,
			       int is_tag);
  void        add_line        (style *s,
			       int line_number,
			       int x1, int y1, int x2, int y2,
			       int thickness);
  void        insert_tag      (const string &str);
  void        dump_page       (void);   // debugging method

  // and the data

  list        glyphs;         // position of glyphs and specials on page
  char_buffer buffer;         // all characters for this page
};

page::page()
{
}

/*
 *  insert_tag - inserts a tag after the current position.
 */

void page::insert_tag (const string &str)
{
  if (str.length() > 0) {
    text_glob *g=new text_glob();
    text_glob *f=glyphs.get_data();
    g->text_glob_tag(&f->text_style, buffer.add_string(str),
		     str.length(), f->minv, f->minh, f->maxv, f->maxh);
    glyphs.insert(g);
  }
}

/*
 *  add - add html text to the list of glyphs.
 */

void page::add (style *s, const string &str,
		int line_number,
		int min_vertical, int min_horizontal,
		int max_vertical, int max_horizontal)
{
  if (str.length() > 0) {
    text_glob *g=new text_glob();
    g->text_glob_html(s, buffer.add_string(str), str.length(),
		      min_vertical, min_horizontal,
		      max_vertical, max_horizontal);
    glyphs.add(g, line_number, min_vertical, min_horizontal,
			       max_vertical, max_horizontal);
  }
}

/*
 *  add_tag - adds a troff tag, for example: .tl .sp .br
 */

void page::add_tag (style *s, const string &str,
		    int line_number,
		    int min_vertical, int min_horizontal,
		    int max_vertical, int max_horizontal)
{
  if (str.length() > 0) {
    text_glob *g;

    if (strncmp((str+'\0').contents(), "devtag:.auto-image",
		strlen("devtag:.auto-image")) == 0) {
      g = new text_glob();
      g->text_glob_auto_image(s, buffer.add_string(str), str.length(),
			      min_vertical, min_horizontal,
			      max_vertical, max_horizontal);
    } else {
      g = new text_glob();
      g->text_glob_tag(s, buffer.add_string(str), str.length(),
		       min_vertical, min_horizontal,
		       max_vertical, max_horizontal);
    }
    glyphs.add(g, line_number, min_vertical, min_horizontal,
			       max_vertical, max_horizontal);
  }
}

/*
 *  add_line - adds the <line> primitive providing that y1==y2
 */

void page::add_line (style *s,
		     int line_number,
		     int x_1, int y_1, int x_2, int y_2,
		     int thickness)
{
  if (y_1 == y_2) {
    text_glob *g = new text_glob();
    g->text_glob_line(s,
		      min(y_1, y_2), min(x_1, x_2),
		      max(y_1, y_2), max(x_1, x_2),
		      thickness);
    glyphs.add(g, line_number,
	       min(y_1, y_2), min(x_1, x_2),
	       max(y_1, y_2), max(x_1, x_2));
  }
}

/*
 *  to_unicode - returns a unicode translation of int, ch.
 */

static char *to_unicode (unsigned int ch)
{
  static char buf[30];

  sprintf(buf, "&#%u;", ch);
  return buf;
}

/*
 *  add_and_encode - adds a special string to the page, it translates
 *                   the string into html glyphs. The special string
 *                   will have come from x X html: and can contain troff
 *                   character encodings which appear as \[char]. A
 *                   sequence of \\ represents \.
 *                   So for example we can write:
 *                      "cost = \[Po]3.00 file = \\foo\\bar"
 *                   which is translated into:
 *                      "cost = &pound;3.00 file = \foo\bar"
 */

void page::add_and_encode (style *s, const string &str,
			   int line_number,
			   int min_vertical, int min_horizontal,
			   int max_vertical, int max_horizontal,
			   int is_tag)
{
  string html_string;
  const char *html_glyph;
  int i = 0;
  const int len = str.length();

  if (0 /* nullptr */ == s->f)
    return;
  while (i < len) {
    if ((i + 1 < len) && (str.substring(i, 2) == string("\\["))) {
      // start of escape
      i += 2; // move over \[
      int a = i;
      while ((i < len) && (str[i] != ']'))
	i++;
      if (i > 0) {
	string troff_charname = str.substring(a, i - a);
	html_glyph = get_html_translation(s->f, troff_charname);
	if (html_glyph)
	  html_string += html_glyph;
	else {
	  glyph *g = name_to_glyph((troff_charname + '\0').contents());
	  if (s->f->contains(g))
	    html_string += s->f->get_code(g);
	}
      }
    }
    else
      html_string += str[i];
    i++;
  }
  if (html_string.length() > 0) {
    text_glob *g=new text_glob();
    if (is_tag)
      g->text_glob_tag(s, buffer.add_string(html_string),
		       html_string.length(),
		       min_vertical, min_horizontal,
		       max_vertical, max_horizontal);
    else
      g->text_glob_special(s, buffer.add_string(html_string),
			   html_string.length(),
			   min_vertical, min_horizontal,
			   max_vertical, max_horizontal);
    glyphs.add(g, line_number, min_vertical,
	       min_horizontal, max_vertical, max_horizontal);
  }
}

/*
 *  dump_page - dump the page contents for debugging purposes.
 */

void page::dump_page(void)
{
#if defined(DEBUG_TABLES)
  text_glob *old_pos = glyphs.get_data();
  text_glob *g;

  printf("\n<!--\n");
  printf("\n\ndebugging start\n");
  glyphs.start_from_head();
  do {
    g = glyphs.get_data();
    if (g->is_tab_ts()) {
      printf("\n\n");
      if (g->get_table() != 0)
	g->get_table()->dump_table();
    }
    printf("%s ", g->text_string);
    if (g->is_tab_te())
      printf("\n\n");
    glyphs.move_right();
  } while (! glyphs.is_equal_to_head());
  glyphs.move_to(old_pos);
  printf("\ndebugging end\n\n");
  printf("\n-->\n");
  fflush(stdout);
#endif
}

/*
 *  font classes and methods
 */

class html_font : public font {
  html_font(const char *);
public:
  int encoding_index;
  char *encoding;
  char *reencoded_name;
  ~html_font();
  static html_font *load_html_font(const char *);
};

html_font *html_font::load_html_font(const char *s)
{
  html_font *f = new html_font(s);
  if (!f->load()) {
    delete f;
    return 0;
  }
  return f;
}

html_font::html_font(const char *nm)
: font(nm)
{
}

html_font::~html_font()
{
}

/*
 *  a simple class to contain the header to this document
 */

class title_desc {
public:
          title_desc ();
         ~title_desc ();

  int     has_been_written;
  int     has_been_found;
  int     with_h1;
  string  text;
};


title_desc::title_desc ()
  : has_been_written(FALSE), has_been_found(FALSE), with_h1(FALSE)
{
}

title_desc::~title_desc ()
{
}

class header_desc {
public:
              header_desc ();
              ~header_desc ();

  int         no_of_level_one_headings; // how many .SH or .NH 1 have we found?
  int         no_of_headings;           // how many headings have we found?
  char_buffer headings;                 // all the headings used in the document
  list        headers;                  // list of headers built from .NH and .SH
  list        header_filename;          // in which file is this header?
  int         header_level;             // current header level
  int         written_header;           // have we written the header yet?
  string      header_buffer;            // current header text

  void        write_headings (FILE *f, int force);
};

header_desc::header_desc ()
  :   no_of_level_one_headings(0), no_of_headings(0), header_level(2),
       written_header(0)
{
}

header_desc::~header_desc ()
{
}

/*
 *  write_headings - emits a list of links for the headings in this
 *                   document
 */

void header_desc::write_headings (FILE *f, int force)
{
  text_glob *g;

  if (auto_links || force) {
    if (! headers.is_empty()) {
      int h=1;

      headers.start_from_head();
      header_filename.start_from_head();
      if (dialect == xhtml)
	fputs("<p>", f);
      do {
	g = headers.get_data();
	fputs("<a href=\"", f);
	if (multiple_files && (! header_filename.is_empty())) {
	  text_glob *fn = header_filename.get_data();
	  fputs(fn->text_string, f);
	}
	fputs("#", f);
	if (simple_anchors) {
	  string buffer(ANCHOR_TEMPLATE);

	  buffer += as_string(h);
	  buffer += '\0';
	  fprintf(f, "%s", buffer.contents());
	} else
	  fputs(g->text_string, f);
	h++;
	fputs("\">", f);
	fputs(g->text_string, f);
        fputs("</a>", f);
	if (dialect == xhtml)
	  fputs("<br/>\n", f);
	else
	  fputs("<br>\n", f);
	headers.move_right();
	if (multiple_files && (! header_filename.is_empty()))
	  header_filename.move_right();
      } while (! headers.is_equal_to_head());
      fputs("\n", f);
      if (dialect == xhtml)
	fputs("</p>\n", f);
    }
  }
}

struct assert_pos {
  assert_pos *next;
  const char *val;
  const char *id;
};

class assert_state {
public:
        assert_state ();
        ~assert_state ();

  void  addx (const char *c, const char *i, const char *v,
	      const char *f, const char *l);
  void  addy (const char *c, const char *i, const char *v,
	      const char *f, const char *l);
  void  build(const char *c, const char *v,
	      const char *f, const char *l);
  void  check_br (int br);
  void  check_ce (int ce);
  void  check_fi (int fi);
  void  check_sp (int sp);
  void  reset    (void);

private:
  int check_br_flag;
  int check_ce_flag;
  int check_fi_flag;
  int check_sp_flag;
  const char *val_br;
  const char *val_ce;
  const char *val_fi;
  const char *val_sp;
  const char *file_br;
  const char *file_ce;
  const char *file_fi;
  const char *file_sp;
  const char *line_br;
  const char *line_ce;
  const char *line_fi;
  const char *line_sp;

  assert_pos *xhead;
  assert_pos *yhead;

  void add (assert_pos **h,
	    const char *c, const char *i, const char *v,
	    const char *f, const char *l);
  void compare(assert_pos *t,
	       const char *v, const char *f, const char *l);
  void close (const char *c);
  void set (const char *c, const char *v,
	    const char *f, const char *l);
  void check_value (const char *s, int v, const char *name,
		    const char *f, const char *l, int *flag);
  int check_value_error (int c, int v, const char *s,
			 const char *name,
			 const char *f, const char *l, int flag);
};

assert_state::assert_state ()
{
  reset();
  val_br   = 0;
  val_ce   = 0;
  val_fi   = 0;
  val_sp   = 0;
  file_br  = 0;
  file_ce  = 0;
  file_fi  = 0;
  file_sp  = 0;
  line_br  = 0;
  line_ce  = 0;
  line_fi  = 0;
  line_sp  = 0;
  xhead    = 0;
  yhead    = 0;
}

assert_state::~assert_state ()
{
  assert_pos *t;

  while (xhead != 0) {
    t = xhead;
    xhead = xhead->next;
    delete[] (char *)t->val;
    delete[] (char *)t->id;
    delete t;
  }
  while (yhead != 0) {
    t = yhead;
    yhead = yhead->next;
    delete[] (char *)t->val;
    delete[] (char *)t->id;
    delete t;
  }
}

void assert_state::reset (void)
{
  check_br_flag = 0;
  check_ce_flag = 0;
  check_fi_flag = 0;
  check_sp_flag = 0;
}

void assert_state::add (assert_pos **h,
			const char *c, const char *i, const char *v,
			const char *f, const char *l)
{
  assert_pos *t = *h;

  while (t != 0) {
    if (strcmp(t->id, i) == 0)
      break;
    t = t->next;
  }
  if (t != 0 && v != 0 && (v[0] != '='))
    compare(t, v, f, l);
  else {
    if (0 /* nullptr */ == t) {
      t = new assert_pos;
      t->next = *h;
      (*h) = t;
    }
    if (v == 0 || v[0] != '=') {
      if (0 /* nullptr */ == f)
	f = strsave("stdin");
      if (0 /* nullptr */ == l)
	l = strsave("<none>");
      if (0 /* nullptr */ == v)
	v = "no value at all";
      fprintf(stderr, "%s:%s:%s: error in assertion format of id=%s;"
	      " expected value prefixed with an '=', got %s\n",
	      program_name, f, l, i, v);
    }
    t->id = i;
    t->val = v;
    delete[] (char *)c;
    delete[] (char *)f;
    delete[] (char *)l;
  }
}

void assert_state::addx (const char *c, const char *i, const char *v,
			 const char *f, const char *l)
{
  add(&xhead, c, i, v, f, l);
}

void assert_state::addy (const char *c, const char *i, const char *v,
			 const char *f, const char *l)
{
  add(&yhead, c, i, v, f, l);
}

void assert_state::compare(assert_pos *t,
			   const char *v, const char *f, const char *l)
{
  const char *s=t->val;

  while ((*v) == '=')
    v++;
  while ((*s) == '=')
    s++;

  if (strcmp(v, s) != 0) {
    if (0 /* nullptr */ == f)
      f = "stdin";
    if (0 /* nullptr */ == l)
      l = "<none>";
    fprintf(stderr, "%s:%s: grohtml assertion failed at id%s: "
		    "expected %s, got %s\n", f, l, t->id, s, v);
  }
}

void assert_state::close (const char *c)
{
  if (strcmp(c, "sp") == 0)
    check_sp_flag = 0;
  else if (strcmp(c, "br") == 0)
    check_br_flag = 0;
  else if (strcmp(c, "fi") == 0)
    check_fi_flag = 0;
  else if (strcmp(c, "nf") == 0)
    check_fi_flag = 0;
  else if (strcmp(c, "ce") == 0)
    check_ce_flag = 0;
  else
    fprintf(stderr, "internal error: unrecognised tag in grohtml "
		    "(%s)\n", c);
}

const char *replace_negate_str (const char *before, char *after)
{
  if (before != 0)
    delete[] (char *)before;

  if (strlen(after) > 0) {
    int d = atoi(after);

    if (d < 0 || d > 1) {
      fprintf(stderr, "expected nf/fi value of 0 or 1, got %d\n", d);
      d = 0;
    }
    if (d == 0)
      after[0] = '1';
    else
      after[0] = '0';
    after[1] = (char)0;
  }
  return after;
}

const char *replace_str (const char *before, const char *after)
{
  if (before != 0)
    delete[] (char *)before;
  return after;
}

void assert_state::set (const char *c, const char *v,
			const char *f, const char *l)
{
  if (0 /* nullptr */ == l)
    l = "<none>";
  if (0 /* nullptr */ == f)
    f = "stdin";

  // fprintf(stderr, "%s:%s:setting %s to %s\n", f, l, c, v);
  if (strcmp(c, "sp") == 0) {
    check_sp_flag = 1;
    val_sp = replace_str(val_sp, strsave(v));
    file_sp = replace_str(file_sp, strsave(f));
    line_sp = replace_str(line_sp, strsave(l));
  } else if (strcmp(c, "br") == 0) {
    check_br_flag = 1;
    val_br = replace_str(val_br, strsave(v));
    file_br = replace_str(file_br, strsave(f));
    line_br = replace_str(line_br, strsave(l));
  } else if (strcmp(c, "fi") == 0) {
    check_fi_flag = 1;
    val_fi = replace_str(val_fi, strsave(v));
    file_fi = replace_str(file_fi, strsave(f));
    line_fi = replace_str(line_fi, strsave(l));
  } else if (strcmp(c, "nf") == 0) {
    check_fi_flag = 1;
    val_fi = replace_negate_str(val_fi, strsave(v));
    file_fi = replace_str(file_fi, strsave(f));
    line_fi = replace_str(line_fi, strsave(l));
  } else if (strcmp(c, "ce") == 0) {
    check_ce_flag = 1;
    val_ce = replace_str(val_ce, strsave(v));
    file_ce = replace_str(file_ce, strsave(f));
    line_ce = replace_str(line_ce, strsave(l));
  }
}

/*
 *  build - builds the troff state assertion.
 *          see tmac/www.tmac for cmd examples.
 */

void assert_state::build (const char *c, const char *v,
			  const char *f, const char *l)
{
  if (c[0] == '{')
    set(&c[1], v, f, l);
  if (c[0] == '}')
    close(&c[1]);
}

int assert_state::check_value_error (int c, int v, const char *s,
				     const char *name, const char *f,
				     const char *l, int flag)
{
  if (! c) {
    if (0 /* nullptr */ == f)
      f = "stdin";
    if (0 /* nullptr */ == l)
      l = "<none>";
    fprintf(stderr, "%s:%s:grohtml (troff state) assertion failed; "
	    "expected %s to be %s, got %d\n", f, l, name, s, v);
    return 0;
  }
  return flag;
}

void assert_state::check_value (const char *s, int v, const char *name,
				const char *f, const char *l, int *flag)
{
  if (strncmp(s, "<=", 2) == 0)
    *flag = check_value_error(v <= atoi(&s[2]), v, s, name, f, l, *flag);
  else if (strncmp(s, ">=", 2) == 0)
    *flag = check_value_error(v >= atoi(&s[2]), v, s, name, f, l, *flag);
  else if (strncmp(s, "==", 2) == 0)
    *flag = check_value_error(v == atoi(&s[2]), v, s, name, f, l, *flag);
  else if (strncmp(s, "!=", 2) == 0)
    *flag = check_value_error(v != atoi(&s[2]), v, s, name, f, l, *flag);
  else if (strncmp(s, "<", 1) == 0)
    *flag = check_value_error(v < atoi(&s[2]), v, s, name, f, l, *flag);
  else if (strncmp(s, ">", 1) == 0)
    *flag = check_value_error(v > atoi(&s[2]), v, s, name, f, l, *flag);
  else if (strncmp(s, "=", 1) == 0)
    *flag = check_value_error(v == atoi(&s[1]), v, s, name, f, l, *flag);
  else
    *flag = check_value_error(v == atoi(s), v, s, name, f, l, *flag);
}

void assert_state::check_sp (int sp)
{
  if (check_sp_flag)
    check_value(val_sp, sp, "sp", file_sp, line_sp, &check_sp_flag);
}

void assert_state::check_fi (int fi)
{
  if (check_fi_flag)
    check_value(val_fi, fi, "fi", file_fi, line_fi, &check_fi_flag);
}

void assert_state::check_br (int br)
{
  if (check_br_flag)
    check_value(val_br, br, "br", file_br, line_br, &check_br_flag);
}

void assert_state::check_ce (int ce)
{
  if (check_ce_flag)
    check_value(val_ce, ce, "ce", file_ce, line_ce, &check_ce_flag);
}

class html_printer : public printer {
  files                file_list;
  simple_output        html;
  int                  res;
  glyph               *space_glyph;
  int                  space_width;
  int                  no_of_printed_pages;
  int                  paper_length;
  string               sbuf;
  int                  sbuf_start_hpos;
  int                  sbuf_vpos;
  int                  sbuf_end_hpos;
  int                  sbuf_prev_hpos;
  int                  sbuf_kern;
  style                sbuf_style;
  int                  last_sbuf_length;
  int                  overstrike_detected;
  style                output_style;
  int                  output_hpos;
  int                  output_vpos;
  int                  output_vpos_max;
  int                  output_draw_point_size;
  int                  line_thickness;
  int                  output_line_thickness;
  unsigned char        output_space_code;
  char                *inside_font_style;
  int                  page_number;
  title_desc           title;
  header_desc          header;
  int                  header_indent;
  int                  suppress_sub_sup;
  int                  cutoff_heading;
  page                *page_contents;
  html_text           *current_paragraph;
  html_indent         *indent;
  html_table          *table;
  int                  end_center;
  int                  end_tempindent;
  TAG_ALIGNMENT        next_tag;
  int                  fill_on;
  int                  max_linelength;
  int                  linelength;
  int                  pageoffset;
  int                  troff_indent;
  int                  device_indent;
  int                  temp_indent;
  int                  pointsize;
  int                  vertical_spacing;
  int                  line_number;
  color               *background;
  int                  seen_indent;
  int                  next_indent;
  int                  seen_pageoffset;
  int                  next_pageoffset;
  int                  seen_linelength;
  int                  next_linelength;
  int                  seen_center;
  int                  next_center;
  int                  seen_space;
  int                  seen_break;
  int                  current_column;
  int                  row_space;
  assert_state         as;

  void  flush_sbuf                    ();
  void  set_style                     (const style &);
  void  set_space_code                (unsigned char c);
  void  do_exec                       (char *, const environment *);
  void  do_import                     (char *, const environment *);
  void  do_def                        (char *, const environment *);
  void  do_mdef                       (char *, const environment *);
  void  do_file                       (char *, const environment *);
  void  set_line_thickness            (const environment *);
  void  terminate_current_font        (void);
  void  flush_font                    (void);
  void  add_to_sbuf                   (glyph *g, const string &s);
  void  write_title                   (int in_head);
  int   sbuf_continuation             (glyph *g, const char *name,
				       const environment *env, int w);
  void  flush_page                    (void);
  void  troff_tag                     (text_glob *g);
  void  flush_globs                   (void);
  void  emit_line                     (text_glob *g);
  void  emit_raw                      (text_glob *g);
  void  emit_html                     (text_glob *g);
  void  determine_space               (text_glob *g);
  void  start_font                    (const char *name);
  void  end_font                      (const char *name);
  int   is_font_courier               (font *f);
  int   is_line_start                 (int nf);
  int   is_courier_until_eol          (void);
  void  start_size                    (int from, int to);
  void  do_font                       (text_glob *g);
  void  do_center                     (char *arg);
  void  do_check_center               (void);
  void  do_break                      (void);
  void  do_space                      (char *arg);
  void  do_eol                        (void);
  void  do_eol_ce                     (void);
  void  do_title                      (void);
  void  do_fill                       (char *arg);
  void  do_heading                    (char *arg);
  void  write_header                  (void);
  void  determine_header_level        (int level);
  void  do_linelength                 (char *arg);
  void  do_pageoffset                 (char *arg);
  void  do_indentation                (char *arg);
  void  do_tempindent                 (char *arg);
  void  do_indentedparagraph          (void);
  void  do_verticalspacing            (char *arg);
  void  do_pointsize                  (char *arg);
  void  do_centered_image             (void);
  void  do_left_image                 (void);
  void  do_right_image                (void);
  void  do_auto_image                 (text_glob *g,
				       const char *filename);
  void  do_links                      (void);
  void  do_flush                      (void);
  void  do_job_name                   (char *name);
  void  do_head                       (char *name);
  void  insert_split_file             (void);
  int   is_in_middle                  (int left, int right);
  void  do_sup_or_sub                 (text_glob *g);
  int   start_subscript               (text_glob *g);
  int   end_subscript                 (text_glob *g);
  int   start_superscript             (text_glob *g);
  int   end_superscript               (text_glob *g);
  void  outstanding_eol               (int n);
  int   is_bold                       (font *f);
  font *make_bold                     (font *f);
  int   overstrike                    (glyph *g, const char *name,
				       const environment *env, int w);
  void  do_body                       (void);
  int   next_horiz_pos                (text_glob *g, int nf);
  void  lookahead_for_tables          (void);
  void  insert_tab_te                 (void);
  text_glob *insert_tab_ts            (text_glob *where);
  void insert_tab0_foreach_tab        (void);
  void insert_tab_0                   (text_glob *where);
  void do_indent                      (int in, int pageoff,
				       int linelen);
  void shutdown_table                 (void);
  void do_tab_ts                      (text_glob *g);
  void do_tab_te                      (void);
  void do_col                         (char *s);
  void do_tab                         (char *s);
  void do_tab0                        (void);
  int  calc_nf                        (text_glob *g, int nf);
  void calc_po_in                     (text_glob *g, int nf);
  void remove_tabs                    (void);
  void remove_courier_tabs            (void);
  void update_min_max                 (colType type_of_col,
				       int *minimum, int *maximum,
				       text_glob *g);
  void add_table_end                  (const char *);
  void do_file_components             (void);
  void write_navigation               (const string &top,
				       const string &prev,
				       const string &next,
				       const string &current);
  void emit_link                      (const string &to,
				       const char *name);
  int  get_troff_indent               (void);
  void restore_troff_indent           (void);
  void handle_assertion               (int minv, int minh,
				       int maxv, int maxh,
				       const char *s);
  void handle_state_assertion         (text_glob *g);
  void do_end_para                    (text_glob *g);
  int  round_width                    (int x);
  void handle_tag_within_title        (text_glob *g);
  void writeHeadMetaStyle             (void);
  void handle_valid_flag              (int needs_para);
  void do_math                        (text_glob *g);
  void write_html_anchor              (text_glob *h);
  void write_xhtml_anchor             (text_glob *h);
  // ADD HERE

public:
  html_printer          ();
  ~html_printer         ();
  void set_char         (glyph *g, font *f, const environment *env,
			 int w, const char *name);
  void set_numbered_char(int num, const environment *env, int *widthp);
  glyph *set_char_and_width(const char *nm, const environment *env,
			 int *widthp, font **f);
  void draw             (int code, int *p, int np,
			 const environment *env);
  void begin_page       (int);
  void end_page         (int);
  void special          (char *arg, const environment *env, char type);
  void devtag           (char *arg, const environment *env, char type);
  font *make_font       (const char *);
  void end_of_line      ();
};

printer *make_printer()
{
  return new html_printer;
}

static void usage(FILE *stream);

void html_printer::set_style(const style &sty)
{
  const char *fontname = sty.f->get_name();
  if (0 /* nullptr */ == fontname)
    fatal("no internalname specified for font");

#if 0
  change_font(fontname, (font::res / (72 * font::sizescale))
			* sty.point_size);
#endif
}

/*
 *  is_bold - returns TRUE if font, f, is bold.
 */

int html_printer::is_bold (font *f)
{
  const char *fontname = f->get_name();
  return (strcmp(fontname, "B") == 0) || (strcmp(fontname, "BI") == 0);
}

/*
 *  make_bold - if a bold style for f exists, return it.
 */

font *html_printer::make_bold (font *f)
{
  const char *fontname = f->get_name();

  if (strcmp(fontname, "B") == 0)
    return f;
  if (strcmp(fontname, "I") == 0)
    return font::load_font("BI");
  if (strcmp(fontname, "BI") == 0)
    return f;
  return 0;
}

void html_printer::end_of_line()
{
  flush_sbuf();
  line_number++;
}

/*
 *  emit_line - writes out a horizontal rule.
 */

void html_printer::emit_line (text_glob *)
{
  // --fixme-- needs to know the length in percentage
  if (dialect == xhtml)
    html.put_string("<hr/>");
  else
    html.put_string("<hr>");
}

/*
 *  restore_troff_indent - is called when we have temporarily shutdown
 *                         indentation (typically done when we have
 *                         centered an image).
 */

void html_printer::restore_troff_indent (void)
{
  troff_indent = next_indent;
  if (troff_indent > 0) {
    /*
     *  force device indentation
     */
    device_indent = 0;
    do_indent(get_troff_indent(), pageoffset, linelength);
  }
}

/*
 *  emit_raw - writes the raw html information directly to the device.
 */

void html_printer::emit_raw (text_glob *g)
{
  do_font(g);
  if (next_tag == INLINE) {
    determine_space(g);
    current_paragraph->do_emittext(g->text_string, g->text_length);
  } else {
    int space = current_paragraph->retrieve_para_space() || seen_space;

    current_paragraph->done_para();
    shutdown_table();
    switch (next_tag) {

    case CENTERED:
      if (dialect == html4)
	current_paragraph->do_para("align=\"center\"", space);
      else
	current_paragraph->do_para("class=\"center\"", space);
      break;
    case LEFT:
      if (dialect == html4)
	current_paragraph->do_para(&html, "align=\"left\"",
				   get_troff_indent(), pageoffset,
				   linelength, space);
      else
	current_paragraph->do_para(&html, "class=\"left\"",
				   get_troff_indent(), pageoffset,
				   linelength, space);
      break;
    case RIGHT:
      if (dialect == html4)
	current_paragraph->do_para(&html, "align=\"right\"",
				   get_troff_indent(), pageoffset,
				   linelength, space);
      else
	current_paragraph->do_para(&html, "class=\"right\"",
				   get_troff_indent(), pageoffset,
				   linelength, space);
      break;
    default:
      fatal("unknown enumeration");
    }
    current_paragraph->do_emittext(g->text_string, g->text_length);
    current_paragraph->done_para();
    next_tag        = INLINE;
    suppress_sub_sup = TRUE;
    seen_space      = FALSE;
    restore_troff_indent();
  }
}

/*
 *  handle_tag_within_title - handle a limited number of tags within
 *                            the context of a table. Those tags which
 *                            set values rather than generate spaces
 *                            and paragraphs.
 */

void html_printer::handle_tag_within_title (text_glob *g)
{
  if (g->is_in() || g->is_ti() || g->is_po() || g->is_ce() || g->is_ll()
      || g->is_fi() || g->is_nf())
    troff_tag(g);
}

/*
 *  do_center - handle the .ce commands from troff.
 */

void html_printer::do_center (char *arg)
{
  next_center = atoi(arg);
  seen_center = TRUE;
}

/*
 *  do_centered_image - set a flag such that the next devtag is
 *                      placed inside a centered paragraph.
 */

void html_printer::do_centered_image (void)
{
  next_tag = CENTERED;
}

/*
 *  do_right_image - set a flag such that the next devtag is
 *                   placed inside a right aligned paragraph.
 */

void html_printer::do_right_image (void)
{
  next_tag = RIGHT;
}

/*
 *  do_left_image - set a flag such that the next devtag is
 *                  placed inside a left aligned paragraph.
 */

void html_printer::do_left_image (void)
{
  next_tag = LEFT;
}

/*
 *  exists - returns TRUE if filename exists.
 */

static int exists (const char *filename)
{
  FILE *fp = fopen(filename, "r");

  if (fp == 0) {
    return FALSE;
  } else {
    fclose(fp);
    return TRUE;
  }
}

/*
 *  generate_img_src - returns a html image tag for the filename
 *                     providing that the image exists.
 */

static string &generate_img_src (const char *filename)
{
  string *s = new string("");

  while (filename && (filename[0] == ' ')) {
    filename++;
  }
  if (exists(filename)) {
    *s += string("<img src=\"") + filename + "\" "
	  + "alt=\"Image " + filename + "\">";
    if (dialect == xhtml)
      *s += "</img>";
  }
  return *s;
}

/*
 *  do_auto_image - tests whether the image, indicated by filename,
 *                  is present, if so then it emits an html image tag.
 *                  An image tag may be passed through from pic, eqn
 *                  but the corresponding image might not be created.
 *                  Consider .EQ delim $$ .EN  or an empty .PS .PE.
 */

void html_printer::do_auto_image (text_glob *g, const char *filename)
{
  string buffer = generate_img_src(filename);

  if (! buffer.empty()) {
    /*
     *  utilize emit_raw by creating a new text_glob.
     */
    text_glob h = *g;

    h.text_string = buffer.contents();
    h.text_length = buffer.length();
    emit_raw(&h);
  } else
    next_tag = INLINE;
}

/*
 *  outstanding_eol - call do_eol, n, times.
 */

void html_printer::outstanding_eol (int n)
{
  while (n > 0) {
    do_eol();
    n--;
  }
}

/*
 *  do_title - handle the .tl commands from troff.
 */

void html_printer::do_title (void)
{
  text_glob    *t;
  int           removed_from_head;

  if (page_number == 1) {
    int found_title_start  = FALSE;
    if (! page_contents->glyphs.is_empty()) {
      page_contents->glyphs.sub_move_right(); // move onto next word
      do {
	t = page_contents->glyphs.get_data();
	removed_from_head = FALSE;
	if (t->is_auto_img()) {
	  string img = generate_img_src((char *)(t->text_string + 20));

	  if (! img.empty()) {
	    if (found_title_start)
	      title.text += " ";
	    found_title_start = TRUE;
	    title.has_been_found = TRUE;
	    title.text += img;
	  }
	  page_contents->glyphs.sub_move_right(); // move onto next word
	  removed_from_head = ((!page_contents->glyphs.is_empty()) &&
			       (page_contents->glyphs
				  .is_equal_to_head()));
	} else if (t->is_eo_tl()) {
	  // end of title found
	  title.has_been_found = TRUE;
	  return;
	} else if (t->is_a_tag()) {
	  handle_tag_within_title(t);
	  page_contents->glyphs.sub_move_right(); // move onto next word
	  removed_from_head = ((!page_contents->glyphs.is_empty()) &&
			       (page_contents->glyphs
				  .is_equal_to_head()));
	} else if (found_title_start) {
	  title.text += " " + string(t->text_string, t->text_length);
	  page_contents->glyphs.sub_move_right(); // move onto next word
	  removed_from_head = ((!page_contents->glyphs.is_empty()) &&
			       (page_contents->glyphs
				  .is_equal_to_head()));
	} else {
	  title.text += string(t->text_string, t->text_length);
	  found_title_start    = TRUE;
	  title.has_been_found = TRUE;
	  page_contents->glyphs.sub_move_right(); // move onto next word
	  removed_from_head = ((!page_contents->glyphs.is_empty()) &&
			       (page_contents->glyphs
				  .is_equal_to_head()));
	}
      } while ((! page_contents->glyphs.is_equal_to_head()) ||
	       (removed_from_head));
    }
  }
}

/*
 *  write_html_anchor - writes out an anchor.  The style of the anchor
 *                      dependent upon simple_anchor.
 */

void html_printer::write_html_anchor (text_glob *h)
{
  if (dialect == html4) {
    if (h != 0) {
      html.put_string("<a name=\"");
      if (simple_anchors) {
	string buffer(ANCHOR_TEMPLATE);

	buffer += as_string(header.no_of_headings);
	buffer += '\0';
	html.put_string(buffer.contents());
      } else
	html.put_string(header.header_buffer);
      html.put_string("\"></a>").nl();
    }
  }
}

/*
 *  write_xhtml_anchor - writes out an anchor.  The style of the anchor
 *                       dependent upon simple_anchor.
 */

void html_printer::write_xhtml_anchor (text_glob *h)
{
  if (dialect == xhtml) {
    if (h != 0) {
      html.put_string(" id=\"");
      if (simple_anchors) {
	string buffer(ANCHOR_TEMPLATE);

	buffer += as_string(header.no_of_headings);
	buffer += '\0';
	html.put_string(buffer.contents());
      } else
	html.put_string(header.header_buffer);
      html.put_string("\"");
    }
  }
}

void html_printer::write_header (void)
{
  if (! header.header_buffer.empty()) {
    text_glob *a = 0;
    int space = current_paragraph->retrieve_para_space() || seen_space;

    if (header.header_level > 7)
      header.header_level = 7;

    // firstly we must terminate any font and type faces
    current_paragraph->done_para();
    suppress_sub_sup = TRUE;

    if (cutoff_heading+2 > header.header_level) {
      // now we save the header so we can issue a list of links
      header.no_of_headings++;
      style st;

      a = new text_glob();
      a->text_glob_html(&st,
			header.headings
			  .add_string(header.header_buffer),
			header.header_buffer.length(),
			header.no_of_headings, header.header_level,
			header.no_of_headings, header.header_level);

      // and add this header to the header list
      header.headers.add(a,
			 header.no_of_headings,
			 header.no_of_headings, header.no_of_headings,
			 header.no_of_headings, header.no_of_headings);
    }

    html.nl().nl();

    if (manufacture_headings) {
      // line break before a header
      if (!current_paragraph->emitted_text())
	current_paragraph->do_space();
      // user wants manufactured headings which look better than
      // <Hn></Hn>
      if (header.header_level<4) {
	html.put_string("<b><font size=\"+1\">");
	html.put_string(header.header_buffer);
	html.put_string("</font>").nl();
	write_html_anchor(a);
        html.put_string("</b>").nl();
      }
      else {
	html.put_string("<b>");
	html.put_string(header.header_buffer).nl();
	write_html_anchor(a);
	html.put_string("</b>").nl();
      }
    }
    else {
      // and now we issue the real header
      html.put_string("<h");
      html.put_number(header.header_level);
      write_xhtml_anchor(a);
      html.put_string(">");
      html.put_string(header.header_buffer).nl();
      write_html_anchor(a);
      html.put_string("</h");
      html.put_number(header.header_level);
      html.put_string(">").nl();
    }

    /* and now we save the file name in which this header will occur */

    style st;   // fake style to enable us to use the list data structure

    text_glob *h=new text_glob();
    h->text_glob_html(&st,
		      header.headings.add_string(file_list.file_name()),
		      file_list.file_name().length(),
		      header.no_of_headings, header.header_level,
		      header.no_of_headings, header.header_level);

    header.header_filename.add(h,
			       header.no_of_headings,
			       header.no_of_headings,
			       header.no_of_headings,
			       header.no_of_headings,
			       header.no_of_headings);

    current_paragraph->do_para(&html, "", get_troff_indent(),
			       pageoffset, linelength, space);
  }
}

void html_printer::determine_header_level (int level)
{
  if (level == 0) {
    int i;

    for (i = 0; ((i<header.header_buffer.length())
		 && ((header.header_buffer[i] == '.')
		   || is_digit(header.header_buffer[i]))) ; i++) {
      if (header.header_buffer[i] == '.') {
	level++;
      }
    }
  }
  header.header_level = level+1;
  if (header.header_level >= 2 && header.header_level <= split_level) {
    header.no_of_level_one_headings++;
    insert_split_file();
  }
}

/*
 *  do_heading - handle the .SH and .NH and equivalent commands from
 *               troff.
 */

void html_printer::do_heading (char *arg)
{
  text_glob *g;
  int  level=atoi(arg);
  int  horiz;

  header.header_buffer.clear();
  page_contents->glyphs.move_right();
  if (! page_contents->glyphs.is_equal_to_head()) {
    g = page_contents->glyphs.get_data();
    horiz = g->minh;
    do {
      if (g->is_auto_img()) {
	string img=generate_img_src((char *)(g->text_string + 20));

	if (! img.empty()) {
	  // we cannot use full heading anchors with images
	  simple_anchors = TRUE;
	  if (horiz < g->minh)
	    header.header_buffer += " ";

	  header.header_buffer += img;
	}
      }
      else if (g->is_in() || g->is_ti() || g->is_po() || g->is_ce()
	       || g->is_ll())
	troff_tag(g);
      else if (g->is_fi())
	fill_on = 1;
      else if (g->is_nf())
	fill_on = 0;
      else if (! (g->is_a_line() || g->is_a_tag())) {
	/*
	 *  we ignore the other tag commands when constructing a heading
	 */
	if (horiz < g->minh)
	  header.header_buffer += " ";

	horiz = g->maxh;
	header.header_buffer += string(g->text_string, g->text_length);
      }
      page_contents->glyphs.move_right();
      g = page_contents->glyphs.get_data();
    } while ((! page_contents->glyphs.is_equal_to_head()) &&
	     (! g->is_eo_h()));
  }

  determine_header_level(level);
  write_header();

  /*
   *  finally set the output font to uninitialized, thus forcing
   *  the new paragraph to start a new font block.
   */

  output_style.f = 0;
  g = page_contents->glyphs.get_data();
  page_contents->glyphs.move_left(); // so that next time we use old g
}

/*
 *  is_courier_until_eol - returns TRUE if we can see a whole line which
 *                         is courier
 */

int html_printer::is_courier_until_eol (void)
{
  text_glob *orig = page_contents->glyphs.get_data();
  int result      = TRUE;
  text_glob *g;

  if (! page_contents->glyphs.is_equal_to_tail()) {
    page_contents->glyphs.move_right();
    do {
      g = page_contents->glyphs.get_data();
      if (! g->is_a_tag() && (! is_font_courier(g->text_style.f)))
	result = FALSE;
      page_contents->glyphs.move_right();
    } while (result &&
	     (! page_contents->glyphs.is_equal_to_head()) &&
	     (! g->is_fi()) && (! g->is_eol()));

    /*
     *  now restore our previous position.
     */
    while (page_contents->glyphs.get_data() != orig)
      page_contents->glyphs.move_left();
  }
  return result;
}

/*
 *  do_linelength - handle the .ll command from troff.
 */

void html_printer::do_linelength (char *arg)
{
  if (max_linelength == -1)
    max_linelength = atoi(arg);

  next_linelength = atoi(arg);
  seen_linelength = TRUE;
}

/*
 *  do_pageoffset - handle the .po command from troff.
 */

void html_printer::do_pageoffset (char *arg)
{
  next_pageoffset = atoi(arg);
  seen_pageoffset = TRUE;
}

/*
 *  get_troff_indent - returns the indent value.
 */

int html_printer::get_troff_indent (void)
{
  if (end_tempindent > 0)
    return temp_indent;
  else
    return troff_indent;
}

/*
 *  do_indentation - handle the .in command from troff.
 */

void html_printer::do_indentation (char *arg)
{
  next_indent = atoi(arg);
  seen_indent = TRUE;
}

/*
 *  do_tempindent - handle the .ti command from troff.
 */

void html_printer::do_tempindent (char *arg)
{
  if (fill_on) {
    /*
     *  we set the end_tempindent to 2 as the first .br
     *  activates the .ti and the second terminates it.
     */
    end_tempindent = 2;
    temp_indent = atoi(arg);
  }
}

/*
 *  shutdown_table - shuts down the current table.
 */

void html_printer::shutdown_table (void)
{
  if (table != 0) {
    current_paragraph->done_para();
    table->emit_finish_table();
    // don't delete this table as it will be deleted when we destroy the
    // text_glob
    table = 0;
  }
}

/*
 *  do_indent - remember the indent parameters and if
 *              indent is > pageoff and indent has changed
 *              then we start a html table to implement the indentation.
 */

void html_printer::do_indent (int in, int pageoff, int linelen)
{
  if ((device_indent != -1) &&
      (pageoffset+device_indent != in+pageoff)) {

    int space = current_paragraph->retrieve_para_space() || seen_space;
    current_paragraph->done_para();

    device_indent = in;
    pageoffset  = pageoff;
    if (linelen <= max_linelength)
      linelength  = linelen;

    current_paragraph->do_para(&html, "", device_indent,
			       pageoffset, max_linelength, space);
  }
}

/*
 *  do_verticalspacing - handle the .vs command from troff.
 */

void html_printer::do_verticalspacing (char *arg)
{
  vertical_spacing = atoi(arg);
}

/*
 *  do_pointsize - handle the .ps command from troff.
 */

void html_printer::do_pointsize (char *arg)
{
  /*
   *  firstly check to see whether this point size is really associated
   *  with a .tl tag
   */

  if (! page_contents->glyphs.is_empty()) {
    text_glob *g = page_contents->glyphs.get_data();
    text_glob *t = page_contents->glyphs.get_data();

    while (t->is_a_tag() && (!page_contents->glyphs.is_equal_to_head()))
    {
      if (t->is_tl()) {
	/*
	 *  found title therefore ignore this .ps tag
	 */
	while (t != g) {
	  page_contents->glyphs.move_left();
	  t = page_contents->glyphs.get_data();
	}
	return;
      }
      page_contents->glyphs.move_right();
      t = page_contents->glyphs.get_data();
    }
    /*
     *  move back to original position
     */
    while (t != g) {
      page_contents->glyphs.move_left();
      t = page_contents->glyphs.get_data();
    }
    /*
     *  collect valid pointsize
     */
    pointsize = atoi(arg);
  }
}

/*
 *  do_fill - records whether troff has requested that text be filled.
 */

void html_printer::do_fill (char *arg)
{
  int on = atoi(arg);

  output_hpos = get_troff_indent()+pageoffset;
  suppress_sub_sup = TRUE;

  if (fill_on != on) {
    if (on)
      current_paragraph->do_para("", seen_space);
    fill_on = on;
  }
}

/*
 *  do_eol - handle the end of line
 */

void html_printer::do_eol (void)
{
  if (! fill_on) {
    if (current_paragraph->ever_emitted_text()) {
      current_paragraph->do_newline();
      current_paragraph->do_break();
    }
  }
  output_hpos = get_troff_indent()+pageoffset;
}

/*
 *  do_check_center - checks to see whether we have seen a '.ce' tag
 *                    during the previous line.
 */

void html_printer::do_check_center(void)
{
  if (seen_center) {
    seen_center = FALSE;
    if (next_center > 0) {
      if (end_center == 0) {
	int space = current_paragraph->retrieve_para_space()
		    || seen_space;
	current_paragraph->done_para();
	suppress_sub_sup = TRUE;
	if (dialect == html4)
	  current_paragraph->do_para("align=\"center\"", space);
	else
	  current_paragraph->do_para("class=\"center\"", space);
      } else
	if ((strcmp("align=\"center\"",
		    current_paragraph->get_alignment()) != 0) &&
	    (strcmp("class=\"center\"",
		    current_paragraph->get_alignment()) != 0)) {
	  /*
	   *  different alignment, so shutdown paragraph and open
	   *  a new one.
	   */
	  int space = current_paragraph->retrieve_para_space()
		      || seen_space;
	  current_paragraph->done_para();
	  suppress_sub_sup = TRUE;
	  if (dialect == html4)
	    current_paragraph->do_para("align=\"center\"", space);
	  else
	    current_paragraph->do_para("class=\"center\"", space);
	} else
	   // same alignment; if we have emitted text, issue a break.
	  if (current_paragraph->emitted_text())
	    current_paragraph->do_break();
    } else
      /*
       *  next_center == 0
       */
      if (end_center > 0) {
	seen_space = seen_space
		     || current_paragraph->retrieve_para_space();
	current_paragraph->done_para();
	suppress_sub_sup = TRUE;
	current_paragraph->do_para("", seen_space);
      }
    end_center = next_center;
  }
}

/*
 *  do_eol_ce - handle end of line specifically for a .ce
 */

void html_printer::do_eol_ce (void)
{
  if (end_center > 0) {
    if (end_center > 1)
      if (current_paragraph->emitted_text())
	current_paragraph->do_break();

    end_center--;
    if (end_center == 0) {
      current_paragraph->done_para();
      suppress_sub_sup = TRUE;
    }
  }
}

/*
 *  do_flush - flushes all output and tags.
 */

void html_printer::do_flush (void)
{
  current_paragraph->done_para();
}

/*
 *  do_links - moves onto a new temporary file and sets auto_links to
 *             false.
 */

void html_printer::do_links (void)
{
  html.end_line();  // flush line
  auto_links = FALSE;  // from now on only emit under user request
  file_list.add_new_file(xtmpfile());
  file_list.set_links_required();
  html.set_file(file_list.get_file());
}

/*
 *  insert_split_file -
 */

void html_printer::insert_split_file (void)
{
  if (multiple_files) {
    current_paragraph->done_para();       // flush paragraph
    html.end_line();                      // flush line
    html.set_file(file_list.get_file());  // flush current file
    file_list.add_new_file(xtmpfile());
    string split_file = job_name;

    split_file += string("-");
    split_file += as_string(header.no_of_level_one_headings);
    if (dialect == xhtml)
      split_file += string(".xhtml");
    else
      split_file += string(".html");
    split_file += '\0';

    file_list.set_file_name(split_file);
    html.set_file(file_list.get_file());
  }
}

/*
 *  do_job_name - assigns the job_name to name.
 */

void html_printer::do_job_name (char *name)
{
  if (! multiple_files) {
    multiple_files = TRUE;
    while (name != 0 && (*name != (char)0) && (*name == ' '))
      name++;
    job_name = name;
  }
}

/*
 *  do_head - adds a string to head_info which is to be included into
 *            the <head> </head> section of the html document.
 */

void html_printer::do_head (char *name)
{
  head_info += string(name);
  head_info += '\n';
}

/*
 *  do_break - handles the ".br" request and also undoes an outstanding
 *             ".ti" command and calls indent if the indentation related
 *             registers have changed.
 */

void html_printer::do_break (void)
{
  int seen_temp_indent = FALSE;

  current_paragraph->do_break();
  if (end_tempindent > 0) {
    end_tempindent--;
    if (end_tempindent > 0)
      seen_temp_indent = TRUE;
  }
  if (seen_indent || seen_pageoffset || seen_linelength
      || seen_temp_indent) {
    if (seen_indent && (! seen_temp_indent))
      troff_indent = next_indent;
    if (! seen_pageoffset)
      next_pageoffset = pageoffset;
    if (! seen_linelength)
      next_linelength = linelength;
    do_indent(get_troff_indent(), next_pageoffset, next_linelength);
  }
  seen_indent     = seen_temp_indent;
  seen_linelength = FALSE;
  seen_pageoffset = FALSE;
  do_check_center();
  output_hpos     = get_troff_indent()+pageoffset;
  suppress_sub_sup = TRUE;
}

void html_printer::do_space (char *arg)
{
  int n = atoi(arg);

  seen_space = atoi(arg);
  as.check_sp(seen_space);
#if 0
  if (n>0 && table)
    table->set_space(TRUE);
#endif

  while (n>0) {
    current_paragraph->do_space();
    n--;
  }
  suppress_sub_sup = TRUE;
}

/*
 *  do_tab_ts - start a table, which will have already been defined.
 */

void html_printer::do_tab_ts (text_glob *g)
{
  html_table *t = g->get_table();

  if (t != 0) {
    current_column = 0;
    current_paragraph->done_pre();
    current_paragraph->done_para();
    current_paragraph->remove_para_space();

#if defined(DEBUG_TABLES)
    html.simple_comment("TABS");
#endif

    t->set_linelength(max_linelength);
    t->add_indent(pageoffset);
#if 0
    t->emit_table_header(seen_space);
#else
    t->emit_table_header(FALSE);
    row_space = current_paragraph->retrieve_para_space() || seen_space;
    seen_space = FALSE;
#endif
  }

  table = t;
}

/*
 *  do_tab_te - finish a table.
 */

void html_printer::do_tab_te (void)
{
  if (table) {
    current_paragraph->done_para();
    current_paragraph->remove_para_space();
    table->emit_finish_table();
  }

  table = 0;
  restore_troff_indent();
}

/*
 *  do_tab - handle the "devtag:tab" tag
 */

void html_printer::do_tab (char *s)
{
  if (table) {
    while (isspace(*s))
      s++;
    s++;
    int col = table->find_column(atoi(s) + pageoffset
	      + get_troff_indent());
    if (col > 0) {
      current_paragraph->done_para();
      table->emit_col(col);
    }
  }
}

/*
 *  do_tab0 - handle the "devtag:tab0" tag
 */

void html_printer::do_tab0 (void)
{
  if (table) {
    int col = table->find_column(pageoffset+get_troff_indent());
    if (col > 0) {
      current_paragraph->done_para();
      table->emit_col(col);
    }
  }
}

/*
 *  do_col - start column, s.
 */

void html_printer::do_col (char *s)
{
  if (table) {
    if (atoi(s) < current_column)
      row_space = seen_space;

    current_column = atoi(s);
    current_paragraph->done_para();
    table->emit_col(current_column);
    current_paragraph->do_para("", row_space);
  }
}

/*
 *  troff_tag - processes the troff tag and manipulates the troff
 *              state machine.
 */

void html_printer::troff_tag (text_glob *g)
{
  /*
   *  firstly skip over devtag:
   */
  char *t=(char *)g->text_string+strlen("devtag:");
  if (strncmp(g->text_string, "html</p>:", strlen("html</p>:")) == 0) {
    do_end_para(g);
  } else if (strncmp(g->text_string, "html<?p>:", strlen("html<?p>:"))
	     == 0) {
    if (current_paragraph->emitted_text())
      html.put_string(g->text_string+9);
    else
      do_end_para(g);
  } else if (strncmp(g->text_string, "math<?p>:", strlen("math<?p>:"))
	      == 0) {
    do_math(g);
  } else if (g->is_eol()) {
    do_eol();
  } else if (g->is_eol_ce()) {
    do_eol_ce();
  } else if (strncmp(t, ".sp", 3) == 0) {
    char *a = (char *)t+3;
    do_space(a);
  } else if (strncmp(t, ".br", 3) == 0) {
    seen_break = 1;
    as.check_br(1);
    do_break();
  } else if (strcmp(t, ".centered-image") == 0) {
    do_centered_image();
  } else if (strcmp(t, ".right-image") == 0) {
    do_right_image();
  } else if (strcmp(t, ".left-image") == 0) {
    do_left_image();
  } else if (strncmp(t, ".auto-image", 11) == 0) {
    char *a = (char *)t+11;
    do_auto_image(g, a);
  } else if (strncmp(t, ".ce", 3) == 0) {
    char *a = (char *)t+3;
    suppress_sub_sup = TRUE;
    do_center(a);
  } else if (g->is_tl()) {
    suppress_sub_sup = TRUE;
    title.with_h1 = TRUE;
    do_title();
  } else if (strncmp(t, ".html-tl", 8) == 0) {
    suppress_sub_sup = TRUE;
    title.with_h1 = FALSE;
    do_title();
  } else if (strncmp(t, ".fi", 3) == 0) {
    char *a = (char *)t+3;
    do_fill(a);
  } else if ((strncmp(t, ".SH", 3) == 0)
	      || (strncmp(t, ".NH", 3) == 0)) {
    char *a = (char *)t+3;
    do_heading(a);
  } else if (strncmp(t, ".ll", 3) == 0) {
    char *a = (char *)t+3;
    do_linelength(a);
  } else if (strncmp(t, ".po", 3) == 0) {
    char *a = (char *)t+3;
    do_pageoffset(a);
  } else if (strncmp(t, ".in", 3) == 0) {
    char *a = (char *)t+3;
    do_indentation(a);
  } else if (strncmp(t, ".ti", 3) == 0) {
    char *a = (char *)t+3;
    do_tempindent(a);
  } else if (strncmp(t, ".vs", 3) == 0) {
    char *a = (char *)t+3;
    do_verticalspacing(a);
  } else if (strncmp(t, ".ps", 3) == 0) {
    char *a = (char *)t+3;
    do_pointsize(a);
  } else if (strcmp(t, ".links") == 0) {
    do_links();
  } else if (strncmp(t, ".job-name", 9) == 0) {
    char *a = (char *)t+9;
    do_job_name(a);
  } else if (strncmp(t, ".head", 5) == 0) {
    char *a = (char *)t+5;
    do_head(a);
  } else if (strcmp(t, ".no-auto-rule") == 0) {
    auto_rule = FALSE;
  } else if (strcmp(t, ".tab-ts") == 0) {
    do_tab_ts(g);
  } else if (strcmp(t, ".tab-te") == 0) {
    do_tab_te();
  } else if (strncmp(t, ".col ", 5) == 0) {
    char *a = (char *)t+4;
    do_col(a);
  } else if (strncmp(t, "tab ", 4) == 0) {
    char *a = (char *)t+3;
    do_tab(a);
  } else if (strncmp(t, "tab0", 4) == 0) {
    do_tab0();
  }
}

/*
 *  do_math - prints out the equation
 */

void html_printer::do_math (text_glob *g)
{
  do_font(g);
  if (current_paragraph->emitted_text())
    html.put_string(g->text_string+9);
  else
    do_end_para(g);
}

/*
 *  is_in_middle - returns TRUE if the positions left..right are in the
 *                 center of the page.
 */

int html_printer::is_in_middle (int left, int right)
{
  return( abs(abs(left-pageoffset) - abs(pageoffset+linelength-right))
	  <= CENTER_TOLERANCE );
}

/*
 *  flush_globs - runs through the text glob list and emits html.
 */

void html_printer::flush_globs (void)
{
  text_glob *g;

  if (! page_contents->glyphs.is_empty()) {
    page_contents->glyphs.start_from_head();
    do {
      g = page_contents->glyphs.get_data();
#if 0
      fprintf(stderr, "[%s:%d:%d:%d:%d]",
	      g->text_string, g->minv, g->minh, g->maxv, g->maxh) ;
      fflush(stderr);
#endif

      handle_state_assertion(g);

      if (strcmp(g->text_string, "XXXXXXX") == 0)
	stop();

      if (g->is_a_tag())
	troff_tag(g);
      else if (g->is_a_line())
	emit_line(g);
      else {
	as.check_sp(seen_space);
	as.check_br(seen_break);
	seen_break = 0;
	seen_space = 0;
	emit_html(g);
      }

      as.check_fi(fill_on);
      as.check_ce(end_center);
      /*
       *  after processing the title (and removing it) the glyph list
       *  might be empty
       */
      if (! page_contents->glyphs.is_empty()) {
	page_contents->glyphs.move_right();
      }
    } while (! page_contents->glyphs.is_equal_to_head());
  }
}

/*
 *  calc_nf - calculates the _no_ format flag, given the
 *            text glob, g.
 */

int html_printer::calc_nf (text_glob *g, int nf)
{
  if (g != 0) {
    if (g->is_fi()) {
      as.check_fi(TRUE);
      return FALSE;
    }
    if (g->is_nf()) {
      as.check_fi(FALSE);
      return TRUE;
    }
  }
  as.check_fi(! nf);
  return nf;
}

/*
 *  calc_po_in - calculates the, in, po, registers
 */

void html_printer::calc_po_in (text_glob *g, int nf)
{
  if (g->is_in())
    troff_indent = g->get_arg();
  else if (g->is_po())
    pageoffset = g->get_arg();
  else if (g->is_ti()) {
    temp_indent = g->get_arg();
    end_tempindent = 2;
  } else if (g->is_br() || (nf && g->is_eol())) {
    if (end_tempindent > 0)
      end_tempindent--;
  }
}

/*
 *  next_horiz_pos - returns the next horiz position.
 *                   -1 is returned if it doesn't exist.
 */

int html_printer::next_horiz_pos (text_glob *g, int nf)
{
  int next = -1;

  if ((g != 0) && (g->is_br() || (nf && g->is_eol())))
    if (! page_contents->glyphs.is_empty()) {
      page_contents->glyphs.move_right_get_data();
      if (0 /* nullptr */ == g) {
	page_contents->glyphs.start_from_head();
	as.reset();
      }
      else {
	next = g->minh;
	page_contents->glyphs.move_left();
      }
    }
  return next;
}

/*
 *  insert_tab_ts - inserts a tab-ts before, where.
 */

text_glob *html_printer::insert_tab_ts (text_glob *where)
{
  text_glob *start_of_table;
  text_glob *old_pos = page_contents->glyphs.get_data();
  page_contents->glyphs.move_to(where);
  page_contents->glyphs.move_left();
  // tab table start
  page_contents->insert_tag(string("devtag:.tab-ts"));
  page_contents->glyphs.move_right();
  start_of_table = page_contents->glyphs.get_data();
  page_contents->glyphs.move_to(old_pos);
  return start_of_table;
}

/*
 *  insert_tab_te - inserts a tab-te before the current position
 *                  (it skips backward over .sp/.br)
 */

void html_printer::insert_tab_te (void)
{
  text_glob *g = page_contents->glyphs.get_data();
  page_contents->dump_page();
  while (page_contents->glyphs.get_data()->is_a_tag())
    page_contents->glyphs.move_left();
  // tab table end
  page_contents->insert_tag(string("devtag:.tab-te"));
  while (g != page_contents->glyphs.get_data())
    page_contents->glyphs.move_right();
  page_contents->dump_page();
}

/*
 *  insert_tab_0 - inserts a tab0 before, where.
 */

void html_printer::insert_tab_0 (text_glob *where)
{
  text_glob *old_pos = page_contents->glyphs.get_data();

  page_contents->glyphs.move_to(where);
  page_contents->glyphs.move_left();
  // tab0 start of line
  page_contents->insert_tag(string("devtag:tab0"));
  page_contents->glyphs.move_right();
  page_contents->glyphs.move_to(old_pos);
}

/*
 *  remove_tabs - removes the tabs tags on this line.
 */

void html_printer::remove_tabs (void)
{
  text_glob *orig = page_contents->glyphs.get_data();
  text_glob *g;

  if (! page_contents->glyphs.is_equal_to_tail()) {
    do {
      g = page_contents->glyphs.get_data();
      if (g->is_tab()) {
	page_contents->glyphs.sub_move_right();
	if (g == orig)
	  orig = page_contents->glyphs.get_data();
      }	else
	page_contents->glyphs.move_right();
    } while ((! page_contents->glyphs.is_equal_to_head()) &&
	     (! g->is_eol()));

    /*
     *  now restore our previous position.
     */
    while (page_contents->glyphs.get_data() != orig)
      page_contents->glyphs.move_left();
  }
}

void html_printer::remove_courier_tabs (void)
{
  text_glob  *g;
  int line_start = TRUE;
  int nf         = FALSE;

  if (! page_contents->glyphs.is_empty()) {
    page_contents->glyphs.start_from_head();
    as.reset();
    line_start = TRUE;
    do {
      g = page_contents->glyphs.get_data();
      handle_state_assertion(g);
      nf = calc_nf(g, nf);

      if (line_start) {
	if (line_start && nf && is_courier_until_eol()) {
	  remove_tabs();
	  g = page_contents->glyphs.get_data();
	}
      }

      // line_start = g->is_br() || g->is_nf() || g->is_fi()
      //             || (nf && g->is_eol());
      line_start = g->is_br() || (nf && g->is_eol());
      page_contents->glyphs.move_right();
    } while (! page_contents->glyphs.is_equal_to_head());
  }
}

void html_printer::insert_tab0_foreach_tab (void)
{
  text_glob  *start_of_line  = 0;
  text_glob  *g              = 0;
  int seen_tab               = FALSE;
  int seen_col               = FALSE;
  int nf                     = FALSE;

  if (! page_contents->glyphs.is_empty()) {
    page_contents->glyphs.start_from_head();
    as.reset();
    start_of_line = page_contents->glyphs.get_data();
    do {
      g = page_contents->glyphs.get_data();
      handle_state_assertion(g);
      nf = calc_nf(g, nf);

      if (g->is_tab())
	seen_tab = TRUE;

      if (g->is_col())
	seen_col = TRUE;

      if (g->is_br() || (nf && g->is_eol())) {
	do {
	  page_contents->glyphs.move_right();
	  g = page_contents->glyphs.get_data();
	  handle_state_assertion(g);
	  nf = calc_nf(g, nf);
	  if (page_contents->glyphs.is_equal_to_head()) {
	    if (seen_tab && !seen_col)
	      insert_tab_0(start_of_line);
	    return;
	  }
	} while (g->is_br() || (nf && g->is_eol()) || g->is_ta());
	// printf("\nstart_of_line is: %s\n", g->text_string);
	if (seen_tab && !seen_col) {
	  insert_tab_0(start_of_line);
	  page_contents->glyphs.move_to(g);
	}

	seen_tab = FALSE;
	seen_col = FALSE;
	start_of_line = g;
      }
      page_contents->glyphs.move_right();
    } while (! page_contents->glyphs.is_equal_to_head());
    if (seen_tab && !seen_col)
      insert_tab_0(start_of_line);

  }
}

/*
 *  update_min_max - updates the extent of a column, given the left and
 *                   right extents of a glyph, g.
 */

void html_printer::update_min_max (colType type_of_col,
				   int *minimum, int *maximum,
				   text_glob *g)
{
  switch (type_of_col) {

  case tab_tag:
    break;
  case tab0_tag:
    *minimum = g->minh;
    break;
  case col_tag:
    *minimum = g->minh;
    *maximum = g->maxh;
    break;
  default:
    break;
  }
}

/*
 *  add_table_end - moves left one glyph, adds a table end tag and adds
 *                  a debugging string.
 */

void html_printer::add_table_end (const char *
#if defined(DEBUG_TABLES)
  debug_string
#endif
)
{
  page_contents->glyphs.move_left();
  insert_tab_te();
#if defined(DEBUG_TABLES)
  page_contents->insert_tag(string(debug_string));
#endif
}

/*
 *  lookahead_for_tables - checks for .col tags and inserts table
 *                         start/end tags
 */

void html_printer::lookahead_for_tables (void)
{
  text_glob  *g;
  text_glob  *start_of_line  = 0;
  text_glob  *start_of_table = 0;
  text_glob  *last           = 0;
  colType     type_of_col    = none;
  int         found_col      = FALSE;
  int         ncol           = 0;
  int         colmin         = 0;		// pacify compiler
  int         colmax         = 0;		// pacify compiler
  html_table *tbl            = new html_table(&html, -1);
  const char *tab_defs       = 0;
  char        align          = 'L';
  int         nf             = FALSE;
  int         old_pageoffset = pageoffset;

  remove_courier_tabs();
  page_contents->dump_page();
  insert_tab0_foreach_tab();
  page_contents->dump_page();
  if (! page_contents->glyphs.is_empty()) {
    page_contents->glyphs.start_from_head();
    as.reset();
    g = page_contents->glyphs.get_data();
    if (g->is_br()) {
      g = page_contents->glyphs.move_right_get_data();
      handle_state_assertion(g);
      if (page_contents->glyphs.is_equal_to_head()) {
	if (tbl != 0) {
	  delete tbl;
	  tbl = 0;
	}
	return;
      }

      start_of_line = g;
      ncol = 0;
      if (found_col)
	last = g;
      found_col = FALSE;
    }

    do {
#if defined(DEBUG_TABLES)
      fprintf(stderr, " [") ;
      fprintf(stderr, g->text_string) ;
      fprintf(stderr, "] ") ;
      fflush(stderr);
      if (strcmp(g->text_string, "XXXXXXX") == 0)
	stop();
#endif

      nf = calc_nf(g, nf);
      calc_po_in(g, nf);
      if (g->is_col()) {
	if (type_of_col == tab_tag && start_of_table != 0) {
	  page_contents->glyphs.move_left();
	  insert_tab_te();
	  start_of_table->remember_table(tbl);
	  tbl = new html_table(&html, -1);
	  page_contents->insert_tag(string("*** TAB -> COL ***"));
	  if (tab_defs != 0)
	    tbl->tab_stops->init(tab_defs);
	  start_of_table = 0;
	  last = 0;
	}
	type_of_col = col_tag;
	found_col = TRUE;
	ncol = g->get_arg();
	align = 'L';
	colmin = 0;
	colmax = 0;
      } else if (g->is_tab()) {
	type_of_col = tab_tag;
	colmin = g->get_tab_args(&align);
	align = 'L'; // for now as 'C' and 'R' are broken
	ncol = tbl->find_tab_column(colmin);
	colmin += pageoffset + get_troff_indent();
	colmax = tbl->get_tab_pos(ncol+1);
	if (colmax > 0)
	  colmax += pageoffset + get_troff_indent();
      } else if (g->is_tab0()) {
	if (type_of_col == col_tag && start_of_table != 0) {
	  page_contents->glyphs.move_left();
	  insert_tab_te();
	  start_of_table->remember_table(tbl);
	  tbl = new html_table(&html, -1);
	  page_contents->insert_tag(string("*** COL -> TAB ***"));
	  start_of_table = 0;
	  last = 0;
	}
	if (tab_defs != 0)
	  tbl->tab_stops->init(tab_defs);
	type_of_col = tab0_tag;
	ncol = 1;
	colmin = 0;
	colmax = tbl->get_tab_pos(2) + pageoffset + get_troff_indent();
      } else if (! g->is_a_tag())
	update_min_max(type_of_col, &colmin, &colmax, g);
      if ((g->is_col() || g->is_tab() || g->is_tab0())
	  && (start_of_line != 0)
	  && (0 /* nullptr */ == start_of_table)) {
	start_of_table = insert_tab_ts(start_of_line);
	start_of_line = 0;
      } else if (g->is_ce() && (start_of_table != 0)) {
	add_table_end("*** CE ***");
	start_of_table->remember_table(tbl);
	tbl = new html_table(&html, -1);
	start_of_table = 0;
	last = 0;
      } else if (g->is_ta()) {
	tab_defs = g->text_string;
	if (type_of_col == col_tag)
	  tbl->tab_stops->check_init(tab_defs);
	if (!tbl->tab_stops->compatible(tab_defs)) {
	  if (start_of_table != 0) {
	    add_table_end("*** TABS ***");
	    start_of_table->remember_table(tbl);
	    tbl = new html_table(&html, -1);
	    start_of_table = 0;
	    type_of_col = none;
	    last = 0;
	  }
	  tbl->tab_stops->init(tab_defs);
	}
      }
      if (((! g->is_a_tag()) || g->is_tab()) && (start_of_table != 0)) {
	// we are in a table and have a glyph
	if ((ncol == 0)
	    || (! tbl->add_column(ncol, colmin, colmax, align))) {
	  if (ncol == 0)
	    add_table_end("*** NCOL == 0 ***");
	  else
	    add_table_end("*** CROSSED COLS ***");

	  start_of_table->remember_table(tbl);
	  tbl = new html_table(&html, -1);
	  start_of_table = 0;
	  type_of_col = none;
	  last = 0;
	}
      }
      /*
       *  move onto next glob, check whether we are starting a new line
       */
      g = page_contents->glyphs.move_right_get_data();
      handle_state_assertion(g);
      if (0 /* nullptr */ == g) {
	if (found_col) {
	  page_contents->glyphs.start_from_head();
	  as.reset();
	  last = g;
	  found_col = FALSE;
	}
      } else if (g->is_br() || (nf && g->is_eol())) {
	do {
	  g = page_contents->glyphs.move_right_get_data();
	  handle_state_assertion(g);
	  nf = calc_nf(g, nf);
	} while ((g != 0) && (g->is_br() || (nf && g->is_eol())));
	start_of_line = g;
	ncol = 0;
	if (found_col)
	  last = g;
	found_col = FALSE;
      }
    } while ((g != 0) && (! page_contents->glyphs.is_equal_to_head()));

#if defined(DEBUG_TABLES)
    fprintf(stderr, "finished scanning for tables\n");
#endif

    page_contents->glyphs.start_from_head();
    if (start_of_table != 0) {
      if (last != 0)
	while (last != page_contents->glyphs.get_data())
	  page_contents->glyphs.move_left();

      insert_tab_te();
      start_of_table->remember_table(tbl);
      tbl = 0;
      page_contents->insert_tag(string("*** LAST ***"));
    }
  }
  if (tbl != 0) {
    delete tbl;
    tbl = 0;
  }

  // and reset the registers
  pageoffset = old_pageoffset;
  troff_indent = 0;
  temp_indent = 0;
  end_tempindent = 0;
}

void html_printer::flush_page (void)
{
  suppress_sub_sup = TRUE;
  flush_sbuf();
  page_contents->dump_page();
  lookahead_for_tables();
  page_contents->dump_page();
  flush_globs();
  current_paragraph->done_para();
  current_paragraph->flush_text();
  // move onto a new page
  delete page_contents;
#if defined(DEBUG_TABLES)
  fprintf(stderr, "\n\n*** flushed page ***\n\n");
  html.simple_comment("new page called");
#endif
  page_contents = new page;
}

/*
 *  determine_space - works out whether we need to write a space.
 *                    If last glyph is adjoining, then emit no space.
 */

void html_printer::determine_space (text_glob *g)
{
  if (current_paragraph->is_in_pre()) {
    /*
     *  .nf has been specified
     */
    while (output_hpos < g->minh) {
      output_hpos += space_width;
      current_paragraph->emit_space();
    }
  } else {
    if ((output_vpos != g->minv) || (output_hpos < g->minh)) {
      current_paragraph->emit_space();
    }
  }
}

/*
 *  is_line_start - returns TRUE if we are at the start of a line.
 */

int html_printer::is_line_start (int nf)
{
  int line_start  = FALSE;
  int result      = TRUE;
  text_glob *orig = page_contents->glyphs.get_data();
  text_glob *g;

  if (! page_contents->glyphs.is_equal_to_head()) {
    do {
      page_contents->glyphs.move_left();
      g = page_contents->glyphs.get_data();
      result = g->is_a_tag();
      if (g->is_fi())
	nf = FALSE;
      else if (g->is_nf())
	nf = TRUE;
      line_start = g->is_col() || g->is_br() || (nf && g->is_eol());
    } while ((!line_start) && (result));
    /*
     *  now restore our previous position.
     */
    while (page_contents->glyphs.get_data() != orig)
      page_contents->glyphs.move_right();
  }
  return result;
}

/*
 *  is_font_courier - returns TRUE if the font, f, is courier.
 */

int html_printer::is_font_courier (font *f)
{
  if (f != 0) {
    const char *fontname = f->get_name();

    return( (fontname != 0) && (fontname[0] == 'C') );
  }
  return FALSE;
}

/*
 *  end_font - shuts down the font corresponding to fontname.
 */

void html_printer::end_font (const char *fontname)
{
  if (strcmp(fontname, "B") == 0) {
    current_paragraph->done_bold();
  } else if (strcmp(fontname, "I") == 0) {
    current_paragraph->done_italic();
  } else if (strcmp(fontname, "BI") == 0) {
    current_paragraph->done_bold();
    current_paragraph->done_italic();
  } else if (strcmp(fontname, "CR") == 0) {
    current_paragraph->done_tt();
  } else if (strcmp(fontname, "CI") == 0) {
    current_paragraph->done_italic();
    current_paragraph->done_tt();
  } else if (strcmp(fontname, "CB") == 0) {
    current_paragraph->done_bold();
    current_paragraph->done_tt();
  } else if (strcmp(fontname, "CBI") == 0) {
    current_paragraph->done_bold();
    current_paragraph->done_italic();
    current_paragraph->done_tt();
  }
}

/*
 *  start_font - starts the font corresponding to name.
 */

void html_printer::start_font (const char *fontname)
{
  if (strcmp(fontname, "R") == 0) {
    current_paragraph->done_bold();
    current_paragraph->done_italic();
    current_paragraph->done_tt();
  } else if (strcmp(fontname, "B") == 0) {
    current_paragraph->do_bold();
  } else if (strcmp(fontname, "I") == 0) {
    current_paragraph->do_italic();
  } else if (strcmp(fontname, "BI") == 0) {
    current_paragraph->do_bold();
    current_paragraph->do_italic();
  } else if (strcmp(fontname, "CR") == 0) {
    if ((! fill_on) && (is_courier_until_eol()) &&
	is_line_start(! fill_on)) {
      current_paragraph->do_pre();
    }
    current_paragraph->do_tt();
  } else if (strcmp(fontname, "CI") == 0) {
    if ((! fill_on) && (is_courier_until_eol()) &&
	is_line_start(! fill_on)) {
      current_paragraph->do_pre();
    }
    current_paragraph->do_tt();
    current_paragraph->do_italic();
  } else if (strcmp(fontname, "CB") == 0) {
    if ((! fill_on) && (is_courier_until_eol()) &&
	is_line_start(! fill_on)) {
      current_paragraph->do_pre();
    }
    current_paragraph->do_tt();
    current_paragraph->do_bold();
  } else if (strcmp(fontname, "CBI") == 0) {
    if ((! fill_on) && (is_courier_until_eol()) &&
	is_line_start(! fill_on)) {
      current_paragraph->do_pre();
    }
    current_paragraph->do_tt();
    current_paragraph->do_italic();
    current_paragraph->do_bold();
  }
}

/*
 *  start_size - from is old font size, to is the new font size.
 *               The HTML elements <big> and <small> respectively
 *               increase and decrease the font size by 20%.  We try and
 *               map these onto glyph sizes.
 */

void html_printer::start_size (int from, int to)
{
  if (from < to) {
    while (from < to) {
      current_paragraph->do_big();
      from += SIZE_INCREMENT;
    }
  } else if (from > to) {
    while (from > to) {
      current_paragraph->do_small();
      from -= SIZE_INCREMENT;
    }
  }
}

/*
 *  do_font - checks to see whether we need to alter the html font.
 */

void html_printer::do_font (text_glob *g)
{
  /*
   *  check if the output_style.point_size has not been set yet
   *  this allow users to place .ps at the top of their troff files
   *  and grohtml can then treat the .ps value as the base font size (3)
   */
  if (output_style.point_size == -1) {
    output_style.point_size = pointsize;
  }

  if (g->text_style.f != output_style.f) {
    if (output_style.f != 0) {
      end_font(output_style.f->get_name());
    }
    output_style.f = g->text_style.f;
    if (output_style.f != 0) {
      start_font(output_style.f->get_name());
    }
  }
  if (output_style.point_size != g->text_style.point_size) {
    do_sup_or_sub(g);
    if ((output_style.point_size > 0) &&
	(g->text_style.point_size > 0)) {
      start_size(output_style.point_size, g->text_style.point_size);
    }
    if (g->text_style.point_size > 0) {
      output_style.point_size = g->text_style.point_size;
    }
  }
  if (output_style.col != g->text_style.col) {
    current_paragraph->done_color();
    output_style.col = g->text_style.col;
    current_paragraph->do_color(&output_style.col);
  }
}

/*
 *  start_subscript - returns TRUE if, g, looks like a subscript start.
 */

int html_printer::start_subscript (text_glob *g)
{
  int r        = font::res;
  int height   = output_style.point_size*r/72;

  return ((output_style.point_size != 0) &&
	  (output_vpos < g->minv) &&
	  (output_vpos-height > g->maxv) &&
	  (output_style.point_size > g->text_style.point_size));
}

/*
 *  start_superscript - returns TRUE if, g, looks like a superscript
 *                      start.
 */

int html_printer::start_superscript (text_glob *g)
{
  int r        = font::res;
  int height   = output_style.point_size*r/72;

  return ((output_style.point_size != 0) &&
	  (output_vpos > g->minv) &&
	  (output_vpos-height < g->maxv) &&
	  (output_style.point_size > g->text_style.point_size));
}

/*
 *  end_subscript - returns TRUE if, g, looks like the end of a
 *                  subscript.
 */

int html_printer::end_subscript (text_glob *g)
{
  int r        = font::res;
  int height   = output_style.point_size*r/72;

  return ((output_style.point_size != 0) &&
	  (g->minv < output_vpos) &&
	  (output_vpos-height > g->maxv) &&
	  (output_style.point_size < g->text_style.point_size));
}

/*
 *  end_superscript - returns TRUE if, g, looks like the end of a
 *                    superscript.
 */

int html_printer::end_superscript (text_glob *g)
{
  int r        = font::res;
  int height   = output_style.point_size*r/72;

  return ((output_style.point_size != 0) &&
	  (g->minv > output_vpos) &&
	  (output_vpos-height < g->maxv) &&
	  (output_style.point_size < g->text_style.point_size));
}

/*
 *  do_sup_or_sub - checks to see whether the next glyph is a
 *                  subscript/superscript start/end and it calls the
 *                  services of html-text to issue the appropriate tags.
 */

void html_printer::do_sup_or_sub (text_glob *g)
{
  if (! suppress_sub_sup) {
    if (start_subscript(g)) {
      current_paragraph->do_sub();
    } else if (start_superscript(g)) {
      current_paragraph->do_sup();
    } else if (end_subscript(g)) {
      current_paragraph->done_sub();
    } else if (end_superscript(g)) {
      current_paragraph->done_sup();
    }
  }
}

/*
 *  do_end_para - writes out the html text after shutting down the
 *                current paragraph.
 */

void html_printer::do_end_para (text_glob *g)
{
  do_font(g);
  current_paragraph->done_para();
  current_paragraph->remove_para_space();
  html.put_string(g->text_string+9);
  output_vpos     = g->minv;
  output_hpos     = g->maxh;
  output_vpos_max = g->maxv;
  suppress_sub_sup = FALSE;
}

/*
 *  emit_html - write out the html text
 */

void html_printer::emit_html (text_glob *g)
{
  do_font(g);
  determine_space(g);
  current_paragraph->do_emittext(g->text_string, g->text_length);
  output_vpos     = g->minv;
  output_hpos     = g->maxh;
  output_vpos_max = g->maxv;
  suppress_sub_sup = FALSE;
}

/*
 *  flush_sbuf - flushes the current sbuf into the list of glyphs.
 */

void html_printer::flush_sbuf()
{
  if (sbuf.length() > 0) {
    int r=font::res;   // resolution of the device
    set_style(sbuf_style);

    if (overstrike_detected && (! is_bold(sbuf_style.f))) {
      font *bold_font = make_bold(sbuf_style.f);
      if (bold_font != 0)
	sbuf_style.f = bold_font;
    }

    page_contents->add(&sbuf_style, sbuf, line_number,
		       (sbuf_vpos - (sbuf_style.point_size * r / 72)),
		       sbuf_start_hpos, sbuf_vpos, sbuf_end_hpos);
    output_hpos = sbuf_end_hpos;
    output_vpos = sbuf_vpos;
    last_sbuf_length = 0;
    sbuf_prev_hpos = sbuf_end_hpos;
    overstrike_detected = FALSE;
    sbuf.clear();
  }
}

void html_printer::set_line_thickness(const environment *env)
{
  line_thickness = env->size;
}

void html_printer::draw(int code, int *p, int np,
			const environment *env)
{
  switch (code) {

  case 'l':
# if 0
    if (np == 2) {
      page_contents->add_line(&sbuf_style,
			      line_number,
			      env->hpos, env->vpos,
			      (env->hpos + p[0]), (env->vpos + p[1]),
			      line_thickness);
    } else {
      error("2 arguments required for line");
    }
# endif
    break;
  case 't':
    {
      if (np == 0) {
	line_thickness = -1;
      } else {
	// troff gratuitously adds an extra 0
	if (np != 1 && np != 2) {
	  error("0 or 1 argument required for thickness");
	  break;
	}
	line_thickness = p[0];
      }
      break;
    }

  case 'P':
    break;
  case 'p':
    break;
  case 'E':
    break;
  case 'e':
    break;
  case 'C':
    break;
  case 'c':
    break;
  case 'a':
    break;
  case '~':
    break;
  case 'f':
    break;
  case 'F':
    // fill with color env->fill
    if (background != 0)
      delete background;
    background = new color;
    *background = *env->fill;
    break;

  default:
    error("unrecognised drawing command '%1'", char(code));
    break;
  }
}

html_printer::html_printer()
: html(0, MAX_LINE_LENGTH),
  no_of_printed_pages(0),
  last_sbuf_length(0),
  overstrike_detected(FALSE),
  output_hpos(-1),
  output_vpos(-1),
  output_vpos_max(-1),
  line_thickness(-1),
  inside_font_style(0),
  page_number(0),
  header_indent(-1),
  suppress_sub_sup(TRUE),
  cutoff_heading(100),
  indent(0),
  table(0),
  end_center(0),
  end_tempindent(0),
  next_tag(INLINE),
  fill_on(TRUE),
  max_linelength(-1),
  linelength(0),
  pageoffset(0),
  troff_indent(0),
  device_indent(0),
  temp_indent(0),
  pointsize(base_point_size),
  line_number(0),
  background(default_background),
  seen_indent(FALSE),
  next_indent(0),
  seen_pageoffset(FALSE),
  next_pageoffset(0),
  seen_linelength(FALSE),
  next_linelength(0),
  seen_center(FALSE),
  next_center(0),
  seen_space(0),
  seen_break(0),
  current_column(0),
  row_space(FALSE)
{
  file_list.add_new_file(xtmpfile());
  html.set_file(file_list.get_file());
  if (font::hor != 24)
    fatal("horizontal motion quantum must be 24");
  if (font::vert != 40)
    fatal("vertical motion quantum must be 40");
#if 0
  // should be sorted html..
  if (font::res % (font::sizescale*72) != 0)
    fatal("res must be a multiple of 72*sizescale");
#endif
  int r = font::res;
  int point = 0;
  while (r % 10 == 0) {
    r /= 10;
    point++;
  }
  res               = r;
  html.set_fixed_point(point);
  space_glyph       = name_to_glyph("space");
  space_width       = font::hor;
  paper_length      = font::paperlength;
  linelength        = font::res*13/2;
  if (paper_length == 0)
    paper_length    = 11*font::res;

  page_contents = new page();
}

/*
 *  add_to_sbuf - adds character code or name to the sbuf.
 */

void html_printer::add_to_sbuf (glyph *g, const string &s)
{
  if (0 /* nullptr */ == sbuf_style.f)
    return;

  const char *html_glyph = 0;
  unsigned int code = sbuf_style.f->get_code(g);

  if (s.empty()) {
    if (sbuf_style.f->contains(g))
      html_glyph = get_html_entity(sbuf_style.f->get_code(g));
    else
      html_glyph = 0;

    if ((0 /* nullptr */ == html_glyph) && (code >= UNICODE_DESC_START))
      html_glyph = to_unicode(code);
  } else
    html_glyph = get_html_translation(sbuf_style.f, s);

  last_sbuf_length = sbuf.length();
  if (0 /* nullptr */ == html_glyph)
    sbuf += ((char)code);
  else
    sbuf += html_glyph;
}

int html_printer::sbuf_continuation (glyph *g, const char *name,
				     const environment *env, int w)
{
  /*
   *  lets see whether the glyph is closer to the end of sbuf
   */
  if ((sbuf_end_hpos == env->hpos)
      || ((sbuf_prev_hpos < sbuf_end_hpos)
	  && (env->hpos < sbuf_end_hpos)
	  && ((sbuf_end_hpos-env->hpos < env->hpos-sbuf_prev_hpos)))) {
    add_to_sbuf(g, name);
    sbuf_prev_hpos = sbuf_end_hpos;
    sbuf_end_hpos += w + sbuf_kern;
    return TRUE;
  } else {
    if ((env->hpos >= sbuf_end_hpos)
	&& ((sbuf_kern == 0)
	    || (sbuf_end_hpos - sbuf_kern != env->hpos))) {
      /*
       *  lets see whether a space is needed or not
       */

      if (env->hpos-sbuf_end_hpos < space_width) {
	add_to_sbuf(g, name);
	sbuf_prev_hpos = sbuf_end_hpos;
	sbuf_end_hpos = env->hpos + w;
	return TRUE;
      }
    }
  }
  return FALSE;
}

/*
 *  get_html_translation - given the position of the character and its
 *                         name return the device encoding for such
 *                         character.
 */

const char *get_html_translation (font *f, const string &name)
{
  if ((0 /* nullptr */ == f) || name.empty())
    return 0;
  else {
    glyph *g = name_to_glyph((char *)(name + '\0').contents());
    if (f->contains(g))
      return get_html_entity(f->get_code(g));
    else
      return 0;
  }
}

/*
 * get_html_entity - given a Unicode character's code point, return an
 *                   HTML entity that represents the character, if the
 *                   character cannot represent itself in all contexts.
 *                   the return value, if not a null pointer, is
 *                   allocated in a static buffer and is only valid
 *                   until the next call of this function.
 */
static const char *get_html_entity (unsigned int code)
{
  if (code < UNICODE_DESC_START) {
    switch (code) {
      case 0x0022: return "&quot;";
      case 0x0026: return "&amp;";
      case 0x003C: return "&lt;";
      case 0x003E: return "&gt;";
      default: return 0;
    }
  } else {
    switch (code) {
      case 0x00A0: return "&nbsp;";
      case 0x00A1: return "&iexcl;";
      case 0x00A2: return "&cent;";
      case 0x00A3: return "&pound;";
      case 0x00A4: return "&curren;";
      case 0x00A5: return "&yen;";
      case 0x00A6: return "&brvbar;";
      case 0x00A7: return "&sect;";
      case 0x00A8: return "&uml;";
      case 0x00A9: return "&copy;";
      case 0x00AA: return "&ordf;";
      case 0x00AB: return "&laquo;";
      case 0x00AC: return "&not;";
      case 0x00AE: return "&reg;";
      case 0x00AF: return "&macr;";
      case 0x00B0: return "&deg;";
      case 0x00B1: return "&plusmn;";
      case 0x00B2: return "&sup2;";
      case 0x00B3: return "&sup3;";
      case 0x00B4: return "&acute;";
      case 0x00B5: return "&micro;";
      case 0x00B6: return "&para;";
      case 0x00B7: return "&middot;";
      case 0x00B8: return "&cedil;";
      case 0x00B9: return "&sup1;";
      case 0x00BA: return "&ordm;";
      case 0x00BB: return "&raquo;";
      case 0x00BC: return "&frac14;";
      case 0x00BD: return "&frac12;";
      case 0x00BE: return "&frac34;";
      case 0x00BF: return "&iquest;";
      case 0x00C0: return "&Agrave;";
      case 0x00C1: return "&Aacute;";
      case 0x00C2: return "&Acirc;";
      case 0x00C3: return "&Atilde;";
      case 0x00C4: return "&Auml;";
      case 0x00C5: return "&Aring;";
      case 0x00C6: return "&AElig;";
      case 0x00C7: return "&Ccedil;";
      case 0x00C8: return "&Egrave;";
      case 0x00C9: return "&Eacute;";
      case 0x00CA: return "&Ecirc;";
      case 0x00CB: return "&Euml;";
      case 0x00CC: return "&Igrave;";
      case 0x00CD: return "&Iacute;";
      case 0x00CE: return "&Icirc;";
      case 0x00CF: return "&Iuml;";
      case 0x00D0: return "&ETH;";
      case 0x00D1: return "&Ntilde;";
      case 0x00D2: return "&Ograve;";
      case 0x00D3: return "&Oacute;";
      case 0x00D4: return "&Ocirc;";
      case 0x00D5: return "&Otilde;";
      case 0x00D6: return "&Ouml;";
      case 0x00D7: return "&times;";
      case 0x00D8: return "&Oslash;";
      case 0x00D9: return "&Ugrave;";
      case 0x00DA: return "&Uacute;";
      case 0x00DB: return "&Ucirc;";
      case 0x00DC: return "&Uuml;";
      case 0x00DD: return "&Yacute;";
      case 0x00DE: return "&THORN;";
      case 0x00DF: return "&szlig;";
      case 0x00E0: return "&agrave;";
      case 0x00E1: return "&aacute;";
      case 0x00E2: return "&acirc;";
      case 0x00E3: return "&atilde;";
      case 0x00E4: return "&auml;";
      case 0x00E5: return "&aring;";
      case 0x00E6: return "&aelig;";
      case 0x00E7: return "&ccedil;";
      case 0x00E8: return "&egrave;";
      case 0x00E9: return "&eacute;";
      case 0x00EA: return "&ecirc;";
      case 0x00EB: return "&euml;";
      case 0x00EC: return "&igrave;";
      case 0x00ED: return "&iacute;";
      case 0x00EE: return "&icirc;";
      case 0x00EF: return "&iuml;";
      case 0x00F0: return "&eth;";
      case 0x00F1: return "&ntilde;";
      case 0x00F2: return "&ograve;";
      case 0x00F3: return "&oacute;";
      case 0x00F4: return "&ocirc;";
      case 0x00F5: return "&otilde;";
      case 0x00F6: return "&ouml;";
      case 0x00F7: return "&divide;";
      case 0x00F8: return "&oslash;";
      case 0x00F9: return "&ugrave;";
      case 0x00FA: return "&uacute;";
      case 0x00FB: return "&ucirc;";
      case 0x00FC: return "&uuml;";
      case 0x00FD: return "&yacute;";
      case 0x00FE: return "&thorn;";
      case 0x00FF: return "&yuml;";
      case 0x0152: return "&OElig;";
      case 0x0153: return "&oelig;";
      case 0x0160: return "&Scaron;";
      case 0x0161: return "&scaron;";
      case 0x0178: return "&Yuml;";
      case 0x0192: return "&fnof;";
      case 0x0391: return "&Alpha;";
      case 0x0392: return "&Beta;";
      case 0x0393: return "&Gamma;";
      case 0x0394: return "&Delta;";
      case 0x0395: return "&Epsilon;";
      case 0x0396: return "&Zeta;";
      case 0x0397: return "&Eta;";
      case 0x0398: return "&Theta;";
      case 0x0399: return "&Iota;";
      case 0x039A: return "&Kappa;";
      case 0x039B: return "&Lambda;";
      case 0x039C: return "&Mu;";
      case 0x039D: return "&Nu;";
      case 0x039E: return "&Xi;";
      case 0x039F: return "&Omicron;";
      case 0x03A0: return "&Pi;";
      case 0x03A1: return "&Rho;";
      case 0x03A3: return "&Sigma;";
      case 0x03A4: return "&Tau;";
      case 0x03A5: return "&Upsilon;";
      case 0x03A6: return "&Phi;";
      case 0x03A7: return "&Chi;";
      case 0x03A8: return "&Psi;";
      case 0x03A9: return "&Omega;";
      case 0x03B1: return "&alpha;";
      case 0x03B2: return "&beta;";
      case 0x03B3: return "&gamma;";
      case 0x03B4: return "&delta;";
      case 0x03B5: return "&epsilon;";
      case 0x03B6: return "&zeta;";
      case 0x03B7: return "&eta;";
      case 0x03B8: return "&theta;";
      case 0x03B9: return "&iota;";
      case 0x03BA: return "&kappa;";
      case 0x03BB: return "&lambda;";
      case 0x03BC: return "&mu;";
      case 0x03BD: return "&nu;";
      case 0x03BE: return "&xi;";
      case 0x03BF: return "&omicron;";
      case 0x03C0: return "&pi;";
      case 0x03C1: return "&rho;";
      case 0x03C2: return "&sigmaf;";
      case 0x03C3: return "&sigma;";
      case 0x03C4: return "&tau;";
      case 0x03C5: return "&upsilon;";
      case 0x03C6: return "&phi;";
      case 0x03C7: return "&chi;";
      case 0x03C8: return "&psi;";
      case 0x03C9: return "&omega;";
      case 0x03D1: return "&thetasym;";
      case 0x03D6: return "&piv;";
      case 0x2013: return "&ndash;";
      case 0x2014: return "&mdash;";
      case 0x2018: return "&lsquo;";
      case 0x2019: return "&rsquo;";
      case 0x201A: return "&sbquo;";
      case 0x201C: return "&ldquo;";
      case 0x201D: return "&rdquo;";
      case 0x201E: return "&bdquo;";
      case 0x2020: return "&dagger;";
      case 0x2021: return "&Dagger;";
      case 0x2022: return "&bull;";
      case 0x2030: return "&permil;";
      case 0x2032: return "&prime;";
      case 0x2033: return "&Prime;";
      case 0x2039: return "&lsaquo;";
      case 0x203A: return "&rsaquo;";
      case 0x203E: return "&oline;";
      case 0x2044: return "&frasl;";
      case 0x20AC: return "&euro;";
      case 0x2111: return "&image;";
      case 0x2118: return "&weierp;";
      case 0x211C: return "&real;";
      case 0x2122: return "&trade;";
      case 0x2135: return "&alefsym;";
      case 0x2190: return "&larr;";
      case 0x2191: return "&uarr;";
      case 0x2192: return "&rarr;";
      case 0x2193: return "&darr;";
      case 0x2194: return "&harr;";
      case 0x21D0: return "&lArr;";
      case 0x21D1: return "&uArr;";
      case 0x21D2: return "&rArr;";
      case 0x21D3: return "&dArr;";
      case 0x21D4: return "&hArr;";
      case 0x2200: return "&forall;";
      case 0x2202: return "&part;";
      case 0x2203: return "&exist;";
      case 0x2205: return "&empty;";
      case 0x2207: return "&nabla;";
      case 0x2208: return "&isin;";
      case 0x2209: return "&notin;";
      case 0x220B: return "&ni;";
      case 0x220F: return "&prod;";
      case 0x2211: return "&sum;";
      case 0x2212: return "&minus;";
      case 0x2217: return "&lowast;";
      case 0x221A: return "&radic;";
      case 0x221D: return "&prop;";
      case 0x221E: return "&infin;";
      case 0x2220: return "&ang;";
      case 0x2227: return "&and;";
      case 0x2228: return "&or;";
      case 0x2229: return "&cap;";
      case 0x222A: return "&cup;";
      case 0x222B: return "&int;";
      case 0x2234: return "&there4;";
      case 0x223C: return "&sim;";
      case 0x2245: return "&cong;";
      case 0x2248: return "&asymp;";
      case 0x2260: return "&ne;";
      case 0x2261: return "&equiv;";
      case 0x2264: return "&le;";
      case 0x2265: return "&ge;";
      case 0x2282: return "&sub;";
      case 0x2283: return "&sup;";
      case 0x2284: return "&nsub;";
      case 0x2286: return "&sube;";
      case 0x2287: return "&supe;";
      case 0x2295: return "&oplus;";
      case 0x2297: return "&otimes;";
      case 0x22A5: return "&perp;";
      case 0x22C5: return "&sdot;";
      case 0x2308: return "&lceil;";
      case 0x2309: return "&rceil;";
      case 0x230A: return "&lfloor;";
      case 0x230B: return "&rfloor;";
      case 0x2329: return "&lang;";
      case 0x232A: return "&rang;";
      case 0x25CA: return "&loz;";
      case 0x2660: return "&spades;";
      case 0x2663: return "&clubs;";
      case 0x2665: return "&hearts;";
      case 0x2666: return "&diams;";
      case 0x27E8: return "&lang;";
      case 0x27E9: return "&rang;";
      default: return to_unicode(code);
    }
  }
}

/*
 *  overstrike - returns TRUE if the glyph (i, name) is going to
 *               overstrike a previous glyph in sbuf.  If TRUE the font
 *               is changed to bold and the previous sbuf is flushed.
 */

int html_printer::overstrike(glyph *g, const char *name,
			     const environment *env, int w)
{
  if ((env->hpos < sbuf_end_hpos)
      || ((sbuf_kern != 0) && (sbuf_end_hpos - sbuf_kern < env->hpos)))
  {
    /*
     *  at this point we have detected an overlap
     */
    if (overstrike_detected) {
      /* already detected, remove previous glyph and use this glyph */
      sbuf.set_length(last_sbuf_length);
      add_to_sbuf(g, name);
      sbuf_end_hpos = env->hpos + w;
      return TRUE;
    } else {
      /* first time we have detected an overstrike in the sbuf */
      sbuf.set_length(last_sbuf_length); /* remove previous glyph */
      if (! is_bold(sbuf_style.f))
	flush_sbuf();
      overstrike_detected = TRUE;
      add_to_sbuf(g, name);
      sbuf_end_hpos = env->hpos + w;
      return TRUE;
    }
  }
  return FALSE;
}

/*
 *  set_char - adds a character into the sbuf if it is a continuation
 *             with the previous word otherwise flush the current sbuf
 *             and add character anew.
 */

void html_printer::set_char(glyph *g, font *f, const environment *env,
			    int w, const char *name)
{
  style sty(f, env->size, env->height, env->slant, env->fontno,
	    *env->col);
  if (sty.slant != 0) {
    if (sty.slant > 80 || sty.slant < -80) {
      error("slant of %1 degrees out of range", sty.slant);
      sty.slant = 0;
    }
  }
  if (((!sbuf.empty())
       && (sty == sbuf_style)
       && (sbuf_vpos == env->vpos))
      && (sbuf_continuation(g, name, env, w)
	  || overstrike(g, name, env, w)))
    return;

  flush_sbuf();
  if (0 /* nullptr */ == sbuf_style.f)
    sbuf_style = sty;
  add_to_sbuf(g, name);
  sbuf_end_hpos = env->hpos + w;
  sbuf_start_hpos = env->hpos;
  sbuf_prev_hpos = env->hpos;
  sbuf_vpos = env->vpos;
  sbuf_style = sty;
  sbuf_kern = 0;
}

/*
 *  set_numbered_char - handle numbered characters.  Negative values are
 *                      interpreted as unbreakable spaces; the value
 *                      (taken positive) gives the width.
 */

void html_printer::set_numbered_char(int num, const environment *env,
				     int *widthp)
{
  int nbsp_width = 0;
  if (num < 0) {
    nbsp_width = -num;
    num = 160;		// &nbsp;
  }
  glyph *g = number_to_glyph(num);
  int fn = env->fontno;
  if (fn < 0 || fn >= nfonts) {
    error("invalid font position '%1'", fn);
    return;
  }
  font *f = font_table[fn];
  if (f == 0) {
    error("no font mounted at position %1", fn);
    return;
  }
  if (!f->contains(g)) {
    error("font '%1' has no glyph at index %2", f->get_name(), num);
    return;
  }
  int w;
  if (nbsp_width)
    w = nbsp_width;
  else
    w = f->get_width(g, env->size);
  w = round_width(w);
  if (widthp)
    *widthp = w;
  set_char(g, f, env, w, 0);
}

glyph *html_printer::set_char_and_width(const char *nm,
					const environment *env,
					int *widthp, font **f)
{
  glyph *g = name_to_glyph(nm);
  int fn = env->fontno;
  if (fn < 0 || fn >= nfonts) {
    error("invalid font position '%1'", fn);
    return UNDEFINED_GLYPH;
  }
  *f = font_table[fn];
  if (*f == 0) {
    error("no font mounted at position %1", fn);
    return UNDEFINED_GLYPH;
  }
  if (!(*f)->contains(g)) {
    if (nm[0] != '\0' && nm[1] == '\0')
      error("font '%1' does not contain ordinary character '%2'",
	    (*f)->get_name(), nm[0]);
    else
      error("font '%1' does not contain special character '%2'",
	    (*f)->get_name(), nm);
    return UNDEFINED_GLYPH;
  }
  int w = (*f)->get_width(g, env->size);
  w = round_width(w);
  if (widthp)
    *widthp = w;
  return g;
}

/*
 *  write_title - writes the title to this document
 */

void html_printer::write_title (int in_head)
{
  if (title.has_been_found) {
    if (in_head) {
      html.put_string("<title>");
      html.put_string(title.text);
      html.put_string("</title>").nl().nl();
    } else {
      title.has_been_written = TRUE;
      if (title.with_h1) {
	if (dialect == xhtml)
	  html.put_string("<h1>");
	else
	  html.put_string("<h1 align=\"center\">");
	html.put_string(title.text);
	html.put_string("</h1>").nl().nl();
      }
    }
  } else if (in_head) {
    // place empty title tags to help conform to 'tidy'
    html.put_string("<title></title>").nl();
  }
}

/*
 *  write_rule - emits HTML rule element if the auto_rule is TRUE.
 */

static void write_rule (void)
{
  if (auto_rule) {
    if (dialect == xhtml)
      fputs("<hr/>\n", stdout);
    else
      fputs("<hr>\n", stdout);
  }
}

void html_printer::begin_page(int n)
{
  page_number            =  n;
#if defined(DEBUGGING)
  html.begin_comment("Page: ")
    .put_string(i_to_a(page_number)).end_comment();;
#endif
  no_of_printed_pages++;

  output_style.f         =  0;
  output_style.point_size= -1;
  output_space_code      = 32;
  output_draw_point_size = -1;
  output_line_thickness  = -1;
  output_hpos            = -1;
  output_vpos            = -1;
  output_vpos_max        = -1;
  current_paragraph      = new html_text(&html, dialect);
  do_indent(get_troff_indent(), pageoffset, linelength);
  current_paragraph->do_para("", FALSE);
}

void html_printer::end_page(int)
{
  flush_sbuf();
  flush_page();
}

font *html_printer::make_font(const char *nm)
{
  return html_font::load_html_font(nm);
}

void html_printer::do_body (void)
{
  if (0 /* nullptr */ == background)
    fputs("<body>\n\n", stdout);
  else {
    char buf[(INT_HEXDIGITS * 3) + 1];
    unsigned int r, g, b;

    background->get_rgb(&r, &g, &b);
    // we have to scale 0..0xFFFF to 0..0xFF
    sprintf(buf, "%.2X%.2X%.2X", r/0x101, g/0x101, b/0x101);

    fputs("<body bgcolor=\"#", stdout);
    fputs(buf, stdout);
    fputs("\">\n\n", stdout);
  }
}

/*
 *  emit_link - generates: <a href="to">name</a>
 */

void html_printer::emit_link (const string &to, const char *name)
{
  fputs("<a href=\"", stdout);
  fputs(to.contents(), stdout);
  fputs("\">", stdout);
  fputs(name, stdout);
  fputs("</a>", stdout);
}

/*
 *  write_navigation - writes out the links which navigate between
 *                     file fragments.
 */

void html_printer::write_navigation (const string &top,
				     const string &prev,
				     const string &next,
				     const string &current)
{
  int need_bar = FALSE;

  if (multiple_files) {
    current_paragraph->done_para();
    write_rule();
    if (groff_sig)
      fputs("\n\n<table width=\"100%\" border=\"0\" rules=\"none\"\n"
	    "frame=\"void\" cellspacing=\"1\" cellpadding=\"0\">\n"
	    "<colgroup><col class=\"left\"></col>"
	    "<col class=\"right\"></col></colgroup>\n"
	    "<tr><td class=\"left\">", stdout);
    handle_valid_flag(FALSE);
    fputs("[ ", stdout);
    if ((strcmp(prev.contents(), "") != 0)
	&& prev != top
	&& prev != current) {
      emit_link(prev, "prev");
      need_bar = TRUE;
    }
    if ((strcmp(next.contents(), "") != 0)
	&& next != top
	&& next != current) {
      if (need_bar)
	fputs(" | ", stdout);
      emit_link(next, "next");
      need_bar = TRUE;
    }
    if (top != "<standard input>"
	&& (strcmp(top.contents(), "") != 0)
	&& top != current) {
      if (need_bar)
	fputs(" | ", stdout);
      emit_link(top, "top");
    }
    fputs(" ]\n", stdout);
    if (groff_sig) {
      fputs("</td><td class=\"right\"><i><small>"
	    "This document was produced using "
	    "<a href=\"http://www.gnu.org/software/groff/\">"
	    "groff-", stdout);
      fputs(Version_string, stdout);
      fputs("</a>.</small></i></td></tr></table>\n", stdout);
    }
    write_rule();
  }
}

/*
 *  do_file_components - scan the file list copying each temporary file
 *                       in turn.  This has twofold use: firstly to emit
 *                       section heading links, between file fragments
 *                       if required and secondly to generate jobname
 *                       file fragments if required.
 */

void html_printer::do_file_components (void)
{
  int fragment_no = 1;
  string top;
  string prev;
  string next;
  string current;

  file_list.start_of_list();
  top = string(job_name);
  if (dialect == xhtml)
    top += string(".xhtml");
  else
    top += string(".html");
  top += '\0';
  next = file_list.next_file_name();
  next += '\0';
  current = next;
  while (file_list.get_file() != 0) {
    if (fseek(file_list.get_file(), 0L, 0) < 0)
      fatal("fseek on temporary file failed");
    html.copy_file(file_list.get_file());
    fclose(file_list.get_file());
    file_list.move_next();
    if (file_list.is_new_output_file()) {
      struct tm *t;

      if (fragment_no > 1)
	write_navigation(top, prev, next, current);
      prev = current;
      current = next;
      next = file_list.next_file_name();
      next += '\0';
      string split_file = file_list.file_name();
      split_file += '\0';
      fflush(stdout);
      if (!freopen(split_file.contents(), "w", stdout)) {
	fatal("unable to reopen standard output stream: %1",
	      strerror(errno));
      }
      fragment_no++;
      if (dialect == xhtml)
	writeHeadMetaStyle();

      if (do_write_creator_comment) {
	html.begin_comment("Creator     : ")
	  .put_string("groff ")
	  .put_string("version ")
	  .put_string(Version_string)
	  .end_comment();
      }

      if (do_write_date_comment) {
	t = current_time();
	html.begin_comment("CreationDate: ")
	  .put_string(asctime(t), strlen(asctime(t))-1)
	  .end_comment();
      }

      if (dialect == html4)
	writeHeadMetaStyle();

      html.put_string("<title>");
      html.put_string(split_file.contents());
      html.put_string("</title>").nl().nl();

      fputs(head_info.contents(), stdout);
      fputs("</head>\n", stdout);
      write_navigation(top, prev, next, current);
    }
    if (file_list.are_links_required())
      header.write_headings(stdout, TRUE);
  }
  if (fragment_no > 1)
    write_navigation(top, prev, next, current);
  else {
    assert(current_paragraph != 0);
    current_paragraph->done_para();
    write_rule();
    if (valid_flag) {
      if (groff_sig)
	fputs("\n\n<table width=\"100%\" border=\"0\" rules=\"none\"\n"
	      "frame=\"void\" cellspacing=\"1\" cellpadding=\"0\">\n"
	      "<colgroup><col class=\"left\"></col>"
	      "<col class=\"right\"></col></colgroup>\n"
	      "<tr><td class=\"left\">", stdout);
      handle_valid_flag(TRUE);
      if (groff_sig) {
	fputs("</td><td class=\"right\"><i><small>"
	      "This document was produced using "
	      "<a href=\"http://www.gnu.org/software/groff/\">"
	      "groff-", stdout);
	fputs(Version_string, stdout);
	fputs("</a>.</small></i></td></tr></table>\n", stdout);
      }
      write_rule();
    }
  }
}

/*
 *  writeHeadMetaStyle - emits the <head> <meta> and <style> tags and
 *                       related information.
 */

void html_printer::writeHeadMetaStyle (void)
{
  if (dialect == html4) {
    fputs("<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 4.01 Transitional"
	  "//EN\"\n", stdout);
    fputs("\"http://www.w3.org/TR/html4/loose.dtd\">\n", stdout);
    fputs("<html>\n", stdout);
    fputs("<head>\n", stdout);
    fputs("<meta name=\"generator\" "
	  "content=\"groff -Thtml, see www.gnu.org\">\n", stdout);
    fputs("<meta http-equiv=\"Content-Type\" "
	  "content=\"text/html; charset=US-ASCII\">\n", stdout);
    fputs("<meta name=\"Content-Style\" content=\"text/css\">\n",
	  stdout);
    fputs("<style type=\"text/css\">\n", stdout);
  }
  else {
    fputs("<?xml version=\"1.0\" encoding=\"us-ascii\"?>\n", stdout);
    fputs("<!DOCTYPE html PUBLIC \"-//W3C//"
	  "DTD XHTML 1.1 plus MathML 2.0//EN\"\n", stdout);
    fputs(" \"http://www.w3.org/TR/MathML2/dtd/xhtml-math11-f.dtd\"\n",
	  stdout);
    fputs(" [<!ENTITY mathml \"http://www.w3.org/1998/Math/"
	  "MathML\">]>\n", stdout);

    fputs("<html xmlns=\"http://www.w3.org/1999/xhtml\" "
	  "xml:lang=\"en\">\n", stdout);
    fputs("<head>\n", stdout);
    fputs("<meta name=\"generator\" "
	  "content=\"groff -Txhtml, see www.gnu.org\"/>\n", stdout);
    fputs("<meta http-equiv=\"Content-Type\" "
	  "content=\"text/html; charset=US-ASCII\"/>\n", stdout);
    fputs("<meta name=\"Content-Style\" content=\"text/css\"/>\n",
	  stdout);
    fputs("<style type=\"text/css\">\n", stdout);
    fputs("       .center { text-align: center }\n", stdout);
    fputs("       .right  { text-align: right }\n", stdout);
  }
  fputs("       p       { margin-top: 0; margin-bottom: 0; "
	"vertical-align: top }\n", stdout);
  fputs("       pre     { margin-top: 0; margin-bottom: 0; "
	"vertical-align: top }\n", stdout);
  fputs("       table   { margin-top: 0; margin-bottom: 0; "
	"vertical-align: top }\n", stdout);
  fputs("       h1      { text-align: center }\n", stdout);
  fputs("</style>\n", stdout);
}

html_printer::~html_printer()
{
  struct tm *t;

  current_lineno = 0; // At this point, we've read all the input.

  if (current_paragraph)
    current_paragraph->flush_text();
  html.end_line();
  html.set_file(stdout);

  if (dialect == xhtml)
    writeHeadMetaStyle();

  if (do_write_creator_comment) {
    html.begin_comment("Creator     : ")
      .put_string("groff ")
      .put_string("version ")
      .put_string(Version_string)
      .end_comment();
  }

  if (do_write_date_comment) {
    t = current_time();
    html.begin_comment("CreationDate: ")
      .put_string(asctime(t), strlen(asctime(t))-1)
      .end_comment();
  }

  if (dialect == html4)
    writeHeadMetaStyle();

  write_title(TRUE);
  head_info += '\0';
  fputs(head_info.contents(), stdout);
  fputs("</head>\n", stdout);
  do_body();

  write_title(FALSE);
  header.write_headings(stdout, FALSE);
  write_rule();
#if defined(DEBUGGING)
  html.begin_comment("Total number of pages: ")
    .put_string(i_to_a(no_of_printed_pages)).end_comment();
#endif
  html.end_line();
  html.end_line();

  if (multiple_files) {
    fputs("</body>\n", stdout);
    fputs("</html>\n", stdout);
    do_file_components();
  } else {
    do_file_components();
    fputs("</body>\n", stdout);
    fputs("</html>\n", stdout);
  }
}

/*
 *  get_str - returns a duplicate of string, s. The duplicate
 *            string is terminated at the next ',' or ']'.
 */

static char *get_str (const char *s, char **n)
{
  int i = 0;
  char *v;

  while ((s[i] != (char)0) && (s[i] != ',') && (s[i] != ']'))
    i++;
  if (i>0) {
    v = new char[i+1];
    memcpy(v, s, i+1);
    v[i] = (char)0;
    if (s[i] == ',')
      (*n) = (char *)&s[i+1];
    else
      (*n) = (char *)&s[i];
    return v;
  }
  if (s[i] == ',')
    (*n) = (char *)&s[1];
  else
    (*n) = (char *)s;
  return 0;
}

/*
 *  make_val - creates a string from if s is a null pointer.
 */

char *make_val (char *s, int v, char *id, char *f, char *l)
{
  if (0 /* nullptr */ == s) {
    char buf[30];

    sprintf(buf, "%d", v);
    return strsave(buf);
  }
  else {
    /*
     *  check that value, s, is the same as, v.
     */
    char *t = s;

    while (*t == '=')
      t++;
    if (atoi(t) != v) {
      if (0 /* nullptr */ == f)
	f = (char *)"stdin";
      if (0 /* nullptr */ == l)
	l = (char *)"<none>";
      fprintf(stderr, "%s:%s: grohtml assertion failed at id%s; "
	      "expected %d, got %s\n", f, l, id, v, s);
    }
    return s;
  }
}

/*
 *  handle_assertion - handles the assertions created via .www:ASSERT
 *                     in www.tmac.  See www.tmac for examples.  This
 *                     method should be called as we are parsing the
 *                     ditroff input.  It checks the x, y position
 *                     assertions.  It does _not_ check the troff state
 *                     assertions as these are unknown at this point.
 */

void html_printer::handle_assertion (int minv, int minh,
				     int maxv, int maxh, const char *s)
{
  char *n;
  char *cmd = get_str(s, &n);
  char *id  = get_str(n, &n);
  char *val = get_str(n, &n);
  char *file= get_str(n, &n);
  char *line= get_str(n, &n);

  if (strcmp(cmd, "assertion:[x") == 0)
    as.addx(cmd, id, make_val(val, minh, id, file, line), file, line);
  else if (strcmp(cmd, "assertion:[y") == 0)
    as.addy(cmd, id, make_val(val, minv, id, file, line), file, line);
  else
    if (strncmp(cmd, "assertion:[", strlen("assertion:[")) == 0)
      page_contents->add_tag(&sbuf_style, string(s),
			     line_number, minv, minh, maxv, maxh);
}

/*
 *  build_state_assertion - builds the troff state assertions.
 */

void html_printer::handle_state_assertion (text_glob *g)
{
  if (g != 0 && g->is_a_tag()
      && (strncmp(g->text_string, "assertion:[", 11) == 0)) {
    char *n   = (char *)&g->text_string[11];
    char *cmd = get_str(n, &n);
    char *val = get_str(n, &n);
    (void)get_str(n, &n);	// unused
    char *file= get_str(n, &n);
    char *line= get_str(n, &n);

    as.build(cmd, val, file, line);
  }
}

/*
 *  special - handle all x X requests from troff.  For post-html they
 *            allow users to pass raw HTML commands, turn auto linked
 *            headings off/on, and so forth.
 */

void html_printer::special(char *s, const environment *env, char type)
{
  if (type != 'p')
    return;
  if (s != 0) {
    flush_sbuf();
    if (env->fontno >= 0) {
      style sty(get_font_from_index(env->fontno), env->size,
		env->height, env->slant, env->fontno, *env->col);
      sbuf_style = sty;
    }

    if (strncmp(s, "html:", 5) == 0) {
      int r=font::res;   /* resolution of the device */
      font *f=sbuf_style.f;

      if (0 /* nullptr */ == f)
	f = font::load_font("TR");

      /*
       *  pass rest of string through to html output during flush
       */
      page_contents->add_and_encode(&sbuf_style, string(&s[5]),
				    line_number,
				    env->vpos-env->size*r/72, env->hpos,
				    env->vpos               , env->hpos,
				    FALSE);

      /*
       * assume that the html command has no width, if it does then
       * hopefully troff will have fudged this in a macro by requesting
       * that the formatting move right by the appropriate amount.
       */
    } else if ((strncmp(s, "html</p>:", 9) == 0) ||
	       (strncmp(s, "html<?p>:", 9) == 0) ||
	       (strncmp(s, "math<?p>:", 9) == 0)) {
      int r=font::res;   /* resolution of the device */
      font *f=sbuf_style.f;
      string t;

      if (0 /* nullptr */ == f)
	f = font::load_font("TR");

      if (strncmp(s, "math<?p>:", 9) == 0) {
	if (strncmp((char *)&s[9], "<math>", 6) == 0) {
	  s[9] = '\0';
	  t = s;
	  t += "<math xmlns=\"http://www.w3.org/1998/Math/MathML\">";
	  t += (char *)&s[15];
	  t += '\0';
	  s = (char *)&t[0];
	}
      }

      /*
       *  need to pass all of string through to html output during flush
       */
      page_contents->add_and_encode(&sbuf_style, string(s),
				    line_number,
				    env->vpos-env->size*r/72, env->hpos,
				    env->vpos               , env->hpos,
				    TRUE);

      /*
       * assume that the html command has no width, if it does then
       * hopefully troff will have fudged this in a macro by
       * requesting that the formatting move right by the appropriate
       * amount.
       */

    } else if (strncmp(s, "index:", 6) == 0) {
      cutoff_heading = atoi(&s[6]);
    } else if (strncmp(s, "assertion:[", 11) == 0) {
      int r=font::res;   /* resolution of the device */

      handle_assertion(env->vpos-env->size*r/72, env->hpos,
		       env->vpos, env->hpos, s);
    }
  }
}

/*
 *  devtag - handles device troff tags sent from the 'troff'.
 *           These include the troff state machine tags:
 *           .br, .sp, .in, .tl, .ll etc
 *
 *           (see man 5 grohtml_tags).
 */

void html_printer::devtag (char *s, const environment *env, char type)
{
  if (type != 'p')
    return;

  if (s != 0) {
    flush_sbuf();
    if (env->fontno >= 0) {
      style sty(get_font_from_index(env->fontno), env->size,
		env->height, env->slant, env->fontno, *env->col);
      sbuf_style = sty;
    }

    if (strncmp(s, "devtag:", strlen("devtag:")) == 0) {
      int r=font::res;   /* resolution of the device */

      page_contents->add_tag(&sbuf_style, string(s),
			     line_number,
			     env->vpos-env->size*r/72, env->hpos,
			     env->vpos               , env->hpos);
    }
  }
}


/*
 *  taken from number.cpp in src/roff/troff, [hunits::hunits(units x)]
 */

int html_printer::round_width(int x)
{
  int r = font::hor;
  int n;

  // don't depend on rounding direction for division of negative ints
  if (r == 1)
    n = x;
  else
    n = (x < 0
	 ? -((-x + r/2 - 1)/r)
	 : (x + r/2 - 1)/r);
  return n * r;
}

/*
 *  handle_valid_flag - emits a valid XHTML 1.1 or HTML 4.01 button,
 *                      provided -V was supplied on the command line.
 */

void html_printer::handle_valid_flag (int needs_para)
{
  if (valid_flag) {
    if (needs_para)
      fputs("<p>", stdout);
    if (dialect == xhtml)
      fputs("<a href=\"http://validator.w3.org/check?uri=referer\">"
	    "<img src=\"http://www.w3.org/Icons/valid-xhtml11-blue\" "
	    "alt=\"Valid XHTML 1.1 Transitional\" "
	    "height=\"31\" width=\"88\" /></a>\n", stdout);
    else
      fputs("<a href=\"http://validator.w3.org/check?uri=referer\">"
	    "<img src=\"http://www.w3.org/Icons/valid-html401-blue\" "
	    "alt=\"Valid HTML 4.01 Transitional\" "
	    "height=\"31\" width=\"88\"></a>\n", stdout);
    if (needs_para)
      fputs("</p>", stdout);
  }
}

int main(int argc, char **argv)
{
  program_name = argv[0];
  static char stderr_buf[BUFSIZ];
  setbuf(stderr, stderr_buf);
  int c;
  static const struct option long_options[] = {
    { "help", no_argument, 0, CHAR_MAX + 1 },
    { "version", no_argument, 0, 'v' },
    { NULL, 0, 0, 0 }
  };
  while ((c = getopt_long(argc, argv,
	  "a:bCdD:eF:g:Ghi:I:j:lno:prs:S:vVx:y", long_options, NULL))
	 != EOF)
    switch(c) {
    case 'a':
      /* text antialiasing bits - handled by pre-html */
      break;
    case 'b':
      // set background color to white
      default_background = new color;
      default_background->set_gray(color::MAX_COLOR_VAL);
      break;
    case 'C':
      // Don't write CreationDate HTML comments.
      do_write_date_comment = FALSE;
      break;
    case 'd':
      /* handled by pre-html */
      break;
    case 'D':
      /* handled by pre-html */
      break;
    case 'e':
      /* handled by pre-html */
      break;
    case 'F':
      font::command_line_font_dir(optarg);
      break;
    case 'g':
      /* graphic antialiasing bits - handled by pre-html */
      break;
    case 'G':
      // Don't write Creator HTML comments.
      do_write_creator_comment = FALSE;
      break;
    case 'h':
      /* do not use the Hn headings of html, but manufacture our own */
      manufacture_headings = TRUE;
      break;
    case 'i':
      /* handled by pre-html */
      break;
    case 'I':
      /* handled by pre-html */
      break;
    case 'j':
      multiple_files = TRUE;
      job_name = optarg;
      break;
    case 'l':
      auto_links = FALSE;
      break;
    case 'n':
      simple_anchors = TRUE;
      break;
    case 'o':
      /* handled by pre-html */
      break;
    case 'p':
      /* handled by pre-html */
      break;
    case 'r':
      auto_rule = FALSE;
      break;
    case 's':
      base_point_size = atoi(optarg);
      break;
    case 'S':
      split_level = atoi(optarg) + 1;
      break;
    case 'v':
      printf("GNU post-grohtml (groff) version %s\n", Version_string);
      exit(0);
      break;
    case 'V':
      valid_flag = TRUE;
      break;
    case 'x':
      if (strcmp(optarg, "x") == 0) {
	dialect = xhtml;
	simple_anchors = TRUE;
      } else if (strcmp(optarg, "4") == 0)
	dialect = html4;
      else
	warning("unsupported HTML dialect: '%1'", optarg);
      break;
    case 'y':
      groff_sig = TRUE;
      break;
    case CHAR_MAX + 1: // --help
      usage(stdout);
      exit(0);
      break;
    case '?':
      usage(stderr);
      exit(1);
      break;
    default:
      assert(0 == "unhandled getopt_long return value");
    }
  if (optind >= argc) {
    do_file("-");
  } else {
    for (int i = optind; i < argc; i++)
      do_file(argv[i]);
  }
  return 0;
}

static void usage(FILE *stream)
{
  fprintf(stream,
"usage: %s [-bCGhlnrVy] [-F font-directory] [-j output-stem]"
" [-s base-type-size] [-S heading-level] [-x html-dialect] [file ...]\n"
"usage: %s {-v | --version}\n"
"usage: %s --help\n",
	  program_name, program_name, program_name);
}

// Local Variables:
// fill-column: 72
// mode: C++
// End:
// vim: set cindent noexpandtab shiftwidth=2 textwidth=72:
