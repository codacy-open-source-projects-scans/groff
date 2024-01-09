/* Copyright (C) 2002-2020 Free Software Foundation, Inc.
     Written by Werner Lemberg <wl@gnu.org>

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

#include "lib.h"
#include "cset.h"
#include "stringclass.h"

#include "unicode.h"

const char *valid_unicode_code_sequence(const char *u)
{
  if (*u != 'u')
    return 0 /* nullptr */;
  const char *p = ++u;
  for (;;) {
    int val = 0;
    const char *start = p;
    for (;;) {
      // only uppercase hex digits allowed
      if (!csxdigit(*p))
	return 0 /* nullptr */;
      if (csdigit(*p))
	val = val*0x10 + (*p-'0');
      else if (csupper(*p))
	val = val*0x10 + (*p-'A'+10);
      else
	return 0 /* nullptr */;
      // biggest Unicode value is U+10FFFF
      if (val > 0x10FFFF)
	return 0 /* nullptr */;
      p++;
      if (*p == '\0' || *p == '_')
	break;
    }
    // surrogates not allowed
    if ((val >= 0xD800 && val <= 0xDBFF) || (val >= 0xDC00 && val <= 0xDFFF))
      return 0 /* nullptr */;
    if (val > 0xFFFF) {
      if (*start == '0')	// no leading zeros allowed if > 0xFFFF
	return 0 /* nullptr */;
    }
    else if (p - start != 4)	// otherwise, check for exactly 4 hex digits
      return 0 /* nullptr */;
    if (*p == '\0')
      break;
    p++;
  }
  return u;
}

// Local Variables:
// fill-column: 72
// mode: C++
// End:
// vim: set cindent noexpandtab shiftwidth=2 textwidth=72:
