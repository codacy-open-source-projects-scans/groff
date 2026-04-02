/* Copyright 1989-1990 Free Software Foundation, Inc.

Written by James Clark (jjc@jclark.com)

This file is part of groff, the GNU roff typesetting system.

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

// TODO: Migrate all callers to `strdup()`.  See Savannah #66518.

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h> // strlen()
#include <stdlib.h> // malloc()

// libgroff
#include "lib.h" // strsave()

char *strsave(const char *s)
{
  if (0 == s /* nullptr */)
    return 0 /* nullptr */;
  char *p = static_cast<char *>(malloc(strlen(s) + 1 /* '\0' */));
  if (p != 0 /* nullptr */)
    (void) strcpy(p, s);
  return p;
}

// Local Variables:
// fill-column: 72
// mode: C++
// End:
// vim: set cindent noexpandtab shiftwidth=2 textwidth=72:
