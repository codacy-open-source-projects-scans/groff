/* Copyright (C) 1989-2020 Free Software Foundation, Inc.
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

#include "errarg.h"
#include "error.h"

#include "defs.h"
#include "refid.h"
#include "search.h"

extern "C" const char *Version_string;

static void usage(FILE *stream)
{
  fprintf(stream,
          "usage: %s [-n] [-p database] [-i XYZ] [-t N] key ...\n"
          "usage: %s {-v | --version}\n"
          "usage: %s --help\n",
	  program_name, program_name, program_name);
}

int main(int argc, char **argv)
{
  program_name = argv[0];
  static char stderr_buf[BUFSIZ];
  setbuf(stderr, stderr_buf);
  int search_default = 1;
  search_list list;
  int opt;
  static const struct option long_options[] = {
    { "help", no_argument, 0, CHAR_MAX + 1 },
    { "version", no_argument, 0, 'v' },
    { NULL, 0, 0, 0 }
  };
  while ((opt = getopt_long(argc, argv, "nvVi:t:p:", long_options, NULL))
	 != EOF)
    switch (opt) {
    case 'V':
      do_verify = true;
      break;
    case 'n':
      search_default = 0;
      break;
    case 'i':
      linear_ignore_fields = optarg;
      break;
    case 't':
      {
	char *ptr;
	long n = strtol(optarg, &ptr, 10);
	if (ptr == optarg) {
	  error("bad integer '%1' in 't' option", optarg);
	  break;
	}
	if (n < 1)
	  n = 1;
	linear_truncate_len = int(n);
	break;
      }
    case 'v':
      {
	printf("GNU lkbib (groff) version %s\n", Version_string);
	exit(0);
	break;
      }
    case 'p':
      list.add_file(optarg);
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
      assert(0);
    }
  if (optind >= argc) {
    usage(stderr);
    exit(1);
  }
  char *filename = getenv("REFER");
  if (filename)
    list.add_file(filename);
  else if (search_default)
    list.add_file(DEFAULT_INDEX, 1);
  if (list.nfiles() == 0)
    fatal("no databases");
  int total_len = 0;
  int i;
  for (i = optind; i < argc; i++)
    total_len += strlen(argv[i]);
  total_len += argc - optind - 1 + 1; // for spaces and '\0'
  char *buffer = new char[total_len];
  char *ptr = buffer;
  for (i = optind; i < argc; i++) {
    if (i > optind)
      *ptr++ = ' ';
    strcpy(ptr, argv[i]);
    ptr = strchr(ptr, '\0');
  }
  search_list_iterator iter(&list, buffer);
  const char *start;
  int len;
  int count;
  for (count = 0; iter.next(&start, &len); count++) {
    if (fwrite(start, 1, len, stdout) != (size_t)len)
      fatal("write error on stdout: %1", strerror(errno));
    // Can happen for last reference in file.
    if (start[len - 1] != '\n')
      putchar('\n');
    putchar('\n');
  }
  return !count;
}

// Local Variables:
// fill-column: 72
// mode: C++
// End:
// vim: set cindent noexpandtab shiftwidth=2 textwidth=72:
