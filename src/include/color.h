/* Copyright (C) 2001-2016 Free Software Foundation, Inc.
     Written by Gaius Mulley <gaius@glam.ac.uk>

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

#include "symbol.h"

enum color_scheme {DEFAULT, CMY, CMYK, RGB, GRAY};

class color {
private:
  color_scheme scheme;
  unsigned int components[4];
  color *next;

  int read_encoding(const color_scheme, const char * const,
		    const size_t);

public:
  symbol nm;
  enum {MAX_COLOR_VAL = 0xffff};
  color(symbol s = default_symbol) : scheme(DEFAULT), nm(s) {}
  color(const color * const);
  ~color();

  int operator==(const color & c) const;
  int operator!=(const color & c) const;

  int is_default() { return scheme == DEFAULT; }

  // set color from given color component values
  void set_default();
  void set_rgb(const unsigned int r, const unsigned int g,
	       const unsigned int b);
  void set_cmy(const unsigned int c, const unsigned int m,
	       const unsigned int y);
  void set_cmyk(const unsigned int c, const unsigned int m,
		const unsigned int y, const unsigned int k);
  void set_gray(const unsigned int g);

  // set color from a color string
  int read_rgb(const char * const s);
  int read_cmy(const char * const s);
  int read_cmyk(const char * const s);
  int read_gray(const char * const s);

  // Return the actual color scheme and retrieve the color components
  // into a predefined vector (of length at least 4).
  color_scheme get_components(unsigned int *c) const;

  // retrieve the components of a color
  void get_rgb(unsigned int *r, unsigned int *g, unsigned int *b) const;
  void get_cmy(unsigned int *c, unsigned int *m, unsigned int *y) const;
  void get_cmyk(unsigned int *c, unsigned int *m,
		unsigned int *y, unsigned int *k) const;
  void get_gray(unsigned int *g) const;

  char *print_color();
};

#define Cyan components[0]
#define Magenta components[1]
#define Yellow components[2]
#define Black components[3]

#define Red components[0]
#define Green components[1]
#define Blue components[2]

#define Gray components[0]

extern color default_color;

// Local Variables:
// fill-column: 72
// mode: C++
// End:
// vim: set cindent noexpandtab shiftwidth=2 textwidth=72:
