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

#include "troff.h"
#include "dictionary.h"
#include "token.h"
#include "request.h"
#include "reg.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h> // assert()

object_dictionary register_dictionary(101);

bool reg::get_value(units * /*d*/)
{
  return false;
}

void reg::increment()
{
  error("cannot increment read-only register");
}

void reg::decrement()
{
  error("cannot decrement read-only register");
}

void reg::set_increment(units /*n*/)
{
  error("cannot automatically increment read-only register");
}

void reg::alter_format(char /*f*/, int /*w*/)
{
  error("cannot assign format of read-only register");
}

const char *reg::get_format()
{
  return "0";
}

void reg::set_value(units /*n*/)
{
  error("cannot write read-only register");
}

general_reg::general_reg() : format('1'), width(0), inc(0)
{
}

static char uppercase_array[] = {
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
  'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
  'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
  'Y', 'Z',
};

static char lowercase_array[] = {
  'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h',
  'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p',
  'q', 'r', 's', 't', 'u', 'v', 'w', 'x',
  'y', 'z',
};

static const char *number_value_to_ascii(int value, char format,
					 int width)
{
  static char buf[128];		// must be at least 21
  switch (format) {
  case '1':
    if (width <= 0)
      return i_to_a(value);
    else if (width > int((sizeof buf) - 2))
      sprintf(buf, "%.*d", int((sizeof buf) - 2), int(value));
    else
      sprintf(buf, "%.*d", width, int(value));
    break;
  case 'i':
  case 'I':
    {
      char *p = buf;
      // troff uses z and w to represent 10000 and 5000 in Roman
      // numerals; I can find no historical basis for this usage
      const char *s = format == 'i' ? "zwmdclxvi" : "ZWMDCLXVI";
      int n = int(value);
      if (n >= 40000 || n <= -40000) {
	error("magnitude of '%1' too big for i or I format", n);
	return i_to_a(n);
      }
      if (n == 0) {
	*p++ = '0';
	*p = '\0';
	break;
      }
      if (n < 0) {
	*p++ = '-';
	n = -n;
      }
      while (n >= 10000) {
	*p++ = s[0];
	n -= 10000;
      }
      for (int i = 1000; i > 0; i /= 10, s += 2) {
	int m = n/i;
	n -= m*i;
	switch (m) {
	case 3:
	  *p++ = s[2];
	  /* falls through */
	case 2:
	  *p++ = s[2];
	  /* falls through */
	case 1:
	  *p++ = s[2];
	  break;
	case 4:
	  *p++ = s[2];
	  *p++ = s[1];
	  break;
	case 8:
	  *p++ = s[1];
	  *p++ = s[2];
	  *p++ = s[2];
	  *p++ = s[2];
	  break;
	case 7:
	  *p++ = s[1];
	  *p++ = s[2];
	  *p++ = s[2];
	  break;
	case 6:
	  *p++ = s[1];
	  *p++ = s[2];
	  break;
	case 5:
	  *p++ = s[1];
	  break;
	case 9:
	  *p++ = s[2];
	  *p++ = s[0];
	}
      }
      *p = '\0';
      break;
    }
  case 'a':
  case 'A':
    {
      int n = value;
      char *p = buf;
      if (n == 0) {
	*p++ = '0';
	*p = '\0';
      }
      else {
	if (n < 0) {
	  n = -n;
	  *p++ = '-';
	}
	// this is a bit tricky
	while (n > 0) {
	  int d = n % 26;
	  if (d == 0)
	    d = 26;
	  n -= d;
	  n /= 26;
	  *p++ = format == 'a' ? lowercase_array[d - 1] :
				 uppercase_array[d - 1];
	}
	*p-- = '\0';
	char *q = buf[0] == '-' ? buf + 1 : buf;
	while (q < p) {
	  char temp = *q;
	  *q = *p;
	  *p = temp;
	  --p;
	  ++q;
	}
      }
      break;
    }
  default:
    assert(0 == "unhandled case of register format");
    break;
  }
  return buf;
}

const char *general_reg::get_string()
{
  units n;
  if (!get_value(&n))
    return "";
  return number_value_to_ascii(n, format, width);
}


void general_reg::increment()
{
  int n;
  if (get_value(&n))
    set_value(n + inc);
}

void general_reg::decrement()
{
  int n;
  if (get_value(&n))
    set_value(n - inc);
}

void general_reg::set_increment(units n)
{
  inc = n;
}

void general_reg::alter_format(char f, int w)
{
  format = f;
  width = w;
}

static const char *number_format_to_ascii(char format, int width)
{
  static char buf[24];
  if (format == '1') {
    if (width > 0) {
      int n = width;
      if (n > int(sizeof buf) - 1)
	n = int(sizeof buf) - 1;
      sprintf(buf, "%.*d", n, 0);
      return buf;
    }
    else
      return "0";
  }
  else {
    buf[0] = format;
    buf[1] = '\0';
    return buf;
  }
}

const char *general_reg::get_format()
{
  return number_format_to_ascii(format, width);
}

class number_reg : public general_reg {
  units value;
public:
  number_reg();
  bool get_value(units *);
  void set_value(units);
};

number_reg::number_reg() : value(0)
{
}

bool number_reg::get_value(units *res)
{
  *res = value;
  return true;
}

void number_reg::set_value(units n)
{
  value = n;
}

variable_reg::variable_reg(units *p) : ptr(p)
{
}

void variable_reg::set_value(units n)
{
  *ptr = n;
}

bool variable_reg::get_value(units *res)
{
  *res = *ptr;
  return true;
}

void define_register()
{
  symbol nm = get_name(true /* required */);
  if (nm.is_null()) {
    skip_line();
    return;
  }
  reg *r = static_cast<reg *>(register_dictionary.lookup(nm));
  units v;
  units prev_value;
  if ((0 /* nullptr */ == r) || !r->get_value(&prev_value))
    prev_value = 0;
  if (read_measurement(&v, 'u', prev_value)) {
    if (0 /* nullptr */ == r) {
      r = new number_reg;
      register_dictionary.define(nm, r);
    }
    r->set_value(v);
    if (tok.is_space()) {
      if (has_arg() && read_measurement(&v, 'u'))
	r->set_increment(v);
    }
    else if (has_arg() && !tok.is_tab())
      warning(WARN_SYNTAX, "expected end of line or an auto-increment"
	      " argument in register definition request; got %1",
	      tok.description());
  }
  skip_line();
}

#if 0
void inline_define_register()
{
  token start_token;
  start_token.next();
  if (!start_token.is_usable_as_delimiter(true /* report error */))
    return;
  tok.next();
  symbol nm = get_name(true /* required */);
  if (nm.is_null())
    return;
  reg *r = static_cast<reg *>(register_dictionary.lookup(nm));
  if (0 /* nullptr */ == r) {
    r = new number_reg;
    register_dictionary.define(nm, r);
  }
  units v;
  units prev_value;
  if ((0 /* nullptr */ == r) || !r->get_value(&prev_value))
    prev_value = 0;
  if (read_measurement(&v, 'u', prev_value)) {
    r->set_value(v);
    if (start_token != tok) {
      if (read_measurement(&v, 'u')) {
	r->set_increment(v);
	if (start_token != tok) {
	  // token::description() writes to static, class-wide storage,
	  // so we must allocate a copy of it before issuing the next
	  // diagnostic.
	  char *delimdesc = strdup(start_token.description());
	  warning(WARN_DELIM, "closing delimiter does not match;"
		  " expected %1, got %2", delimdesc, tok.description());
	  free(delimdesc);
	}
      }
    }
  }
}
#endif

void set_register(symbol nm, units n)
{
  reg *r = static_cast<reg *>(register_dictionary.lookup(nm));
  if (0 /* nullptr */ == r) {
    r = new number_reg;
    register_dictionary.define(nm, r);
  }
  r->set_value(n);
}

reg *look_up_register(symbol nm)
{
  reg *r = static_cast<reg *>(register_dictionary.lookup(nm));
  if (0 /* nullptr */ == r) {
    warning(WARN_REG, "register '%1' not defined", nm.contents());
    r = new number_reg;
    register_dictionary.define(nm, r);
  }
  return r;
}

void alter_format()
{
  symbol nm = get_name(true /* required */);
  if (nm.is_null()) {
    skip_line();
    return;
  }
  reg *r = static_cast<reg *>(register_dictionary.lookup(nm));
  if (0 /* nullptr */ == r) {
    r = new number_reg;
    register_dictionary.define(nm, r);
  }
  tok.skip();
  char c = tok.ch();
  if (csdigit(c)) {
    int n = 0;
    do {
      ++n;
      tok.next();
    } while (csdigit(tok.ch()));
    r->alter_format('1', n);
  }
  else if (c == 'i' || c == 'I' || c == 'a' || c == 'A')
    r->alter_format(c);
  else if (tok.is_newline() || tok.is_eof())
    warning(WARN_MISSING, "missing register format");
  else
    error("invalid register format (got %1)", tok.description());
  skip_line();
}

void remove_reg()
{
  for (;;) {
    symbol s = get_name();
    if (s.is_null())
      break;
    register_dictionary.remove(s);
  }
  skip_line();
}

void alias_reg()
{
  symbol s1 = get_name(true /* required */);
  if (!s1.is_null()) {
    symbol s2 = get_name(true /* required */);
    if (!s2.is_null()) {
      if (!register_dictionary.alias(s1, s2))
	error("cannot alias undefined register '%1'", s2.contents());
    }
  }
  skip_line();
}

void rename_reg()
{
  symbol s1 = get_name(true /* required */);
  if (!s1.is_null()) {
    symbol s2 = get_name(true /* required */);
    if (!s2.is_null())
      register_dictionary.rename(s1, s2);
  }
  skip_line();
}

void print_registers()
{
  object_dictionary_iterator iter(register_dictionary);
  reg *r;
  symbol s;
  while (iter.get(&s, (object **)&r)) {
    assert(!s.is_null());
    errprint("%1\t", s.contents());
    const char *p = r->get_string();
    if (p)
      errprint(p);
    errprint("\n");
  }
  fflush(stderr);
  skip_line();
}

void init_reg_requests()
{
  init_request("rr", remove_reg);
  init_request("nr", define_register);
  init_request("af", alter_format);
  init_request("aln", alias_reg);
  init_request("rnn", rename_reg);
  init_request("pnr", print_registers);
}

// Local Variables:
// fill-column: 72
// mode: C++
// End:
// vim: set cindent noexpandtab shiftwidth=2 textwidth=72:
