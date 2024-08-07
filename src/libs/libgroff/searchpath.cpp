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

#include "lib.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <errno.h>
#include <stdlib.h>

// for stat(2)
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "searchpath.h"
#include "nonposix.h"

#ifdef _WIN32
# include "relocate.h"
#else
# define relocate(path) strsave(path)
#endif

static bool is_directory(const char *name)
{
  struct stat statbuf;
  // If stat() fails, a later fopen() will fail anyway (he said
  // TOCTTOUishly).
  if ((stat(name, &statbuf) == 0)
      && ((statbuf.st_mode & S_IFMT) == S_IFDIR))
    return true;
  return false;
}

search_path::search_path(const char *envvar, const char *standard,
			 int add_home, int add_current)
{
  char *home = 0 /* nullptr */;
  if (add_home)
    home = getenv("HOME");
  char *e = 0 /* nullptr */;
  if (envvar != 0 /* nullptr */)
    e = getenv(envvar);
  dirs = new char[((e && *e) ? strlen(e) + 1 : 0)
		  + (add_current ? 1 + 1 : 0)
		  + ((home && *home) ? strlen(home) + 1 : 0)
		  + ((standard && *standard) ? strlen(standard) : 0)
		  + 1];
  *dirs = '\0';
  if (e && *e) {
    strcat(dirs, e);
    strcat(dirs, PATH_SEP);
  }
  if (add_current) {
    strcat(dirs, ".");
    strcat(dirs, PATH_SEP);
  }
  if (home && *home) {
    strcat(dirs, home);
    strcat(dirs, PATH_SEP);
  }
  if (standard && *standard)
    strcat(dirs, standard);
  init_len = strlen(dirs);
}

search_path::~search_path()
{
  // dirs is always allocated
  delete[] dirs;
}

void search_path::command_line_dir(const char *s)
{
  char *old = dirs;
  unsigned old_len = strlen(old);
  unsigned slen = strlen(s);
  dirs = new char[old_len + 1 + slen + 1];
  memcpy(dirs, old, old_len - init_len);
  char *p = dirs;
  p += old_len - init_len;
  if (init_len == 0)
    *p++ = PATH_SEP_CHAR;
  memcpy(p, s, slen);
  p += slen;
  if (init_len > 0) {
    *p++ = PATH_SEP_CHAR;
    memcpy(p, old + old_len - init_len, init_len);
    p += init_len;
  }
  *p++ = '\0';
  delete[] old;
}

FILE *search_path::open_file(const char *name, char **pathp)
{
  assert(name != 0 /* nullptr */);
  if (IS_ABSOLUTE(name) || *dirs == '\0') {
    if (is_directory(name)) {
      errno = EISDIR;
      return 0 /* nullptr */;
    }
    FILE *fp = fopen(name, "r");
    if (fp != 0 /* nullptr */) {
      if (pathp != 0 /* nullptr */)
	*pathp = strsave(name);
      return fp;
    }
    else
      return 0 /* nullptr */;
  }
  unsigned namelen = strlen(name);
  char *p = dirs;
  for (;;) {
    char *end = strchr(p, PATH_SEP_CHAR);
    if (0 /* nullptr */ == end)
      end = strchr(p, '\0');
    int need_slash = end > p && strchr(DIR_SEPS, end[-1]) == 0;
    char *origpath = new char[(end - p) + need_slash + namelen + 1];
    memcpy(origpath, p, end - p);
    if (need_slash)
      origpath[end - p] = '/';
    strcpy(origpath + (end - p) + need_slash, name);
#if 0
    fprintf(stderr, "origpath '%s'\n", origpath);
#endif
    char *path = relocate(origpath);
    delete[] origpath;
#if 0
    fprintf(stderr, "trying '%s'\n", path);
#endif
    if (is_directory(name)) {
      errno = EISDIR;
      return 0 /* nullptr */;
    }
    FILE *fp = fopen(path, "r");
    int err = errno;
    if (fp != 0 /* nullptr */) {
      if (pathp != 0 /* nullptr */)
	*pathp = path;
      else {
	free(path);
	errno = err;
      }
      return fp;
    }
    free(path);
    errno = err;
    if (*end == '\0')
      break;
    p = end + 1;
  }
  return 0 /* nullptr */;
}

FILE *search_path::open_file_cautious(const char *name, char **pathp,
				      const char *mode)
{
  if (0 /* nullptr */ == mode)
    mode = "r";
  bool reading = (strchr(mode, 'r') != 0 /* nullptr */);
  if (0 /* nullptr */ == name || strcmp(name, "-") == 0) {
    if (pathp != 0)
      *pathp = strsave(reading ? "stdin" : "stdout");
    return (reading ? stdin : stdout);
  }
  if (!reading || IS_ABSOLUTE(name) || *dirs == '\0') {
    if (is_directory(name)) {
      errno = EISDIR;
      return 0 /* nullptr */;
    }
    FILE *fp = fopen(name, mode);
    if (fp != 0 /* nullptr */) {
      if (pathp != 0 /* nullptr */)
	*pathp = strsave(name);
      return fp;
    }
    else
      return 0 /* nullptr */;
  }
  unsigned namelen = strlen(name);
  char *p = dirs;
  for (;;) {
    char *end = strchr(p, PATH_SEP_CHAR);
    if (0 /* nullptr */ == end)
      end = strchr(p, '\0');
    int need_slash = (end > p
		      && strchr(DIR_SEPS, end[-1]) == 0 /* nullptr */);
    char *origpath = new char[(end - p) + need_slash + namelen + 1];
    memcpy(origpath, p, end - p);
    if (need_slash)
      origpath[end - p] = '/';
    strcpy(origpath + (end - p) + need_slash, name);
#if 0
    fprintf(stderr, "origpath '%s'\n", origpath);
#endif
    char *path = relocate(origpath);
    delete[] origpath;
#if 0
    fprintf(stderr, "trying '%s'\n", path);
#endif
    if (is_directory(name)) {
      errno = EISDIR;
      return 0 /* nullptr */;
    }
    FILE *fp = fopen(path, mode);
    int err = errno;
    if (fp != 0 /* nullptr */) {
      if (pathp != 0 /* nullptr */)
	*pathp = path;
      else {
	free(path);
	errno = err;
      }
      return fp;
    }
    free(path);
    errno = err;
    if (err != ENOENT)
      return 0 /* nullptr */;
    if (*end == '\0')
      break;
    p = end + 1;
  }
  errno = ENOENT;
  return 0 /* nullptr */;
}

// Local Variables:
// fill-column: 72
// mode: C++
// End:
// vim: set cindent noexpandtab shiftwidth=2 textwidth=72:
