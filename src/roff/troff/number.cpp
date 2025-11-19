/* Copyright 1989-2025 Free Software Foundation, Inc.
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

#include <stdckdint.h>

#include "troff.h"
#include "hvunits.h"
#include "stringclass.h"
#include "mtsm.h"
#include "env.h"
#include "token.h"
#include "div.h"

const vunits V0; // zero in vertical units
const hunits H0; // zero in horizontal units

int hresolution = 1;
int vresolution = 1;
int units_per_inch;
int sizescale;

static bool is_valid_expression(units *u, int scaling_unit,
				bool is_parenthesized,
				bool is_mandatory = false);
static bool is_valid_expression_start();

bool get_vunits(vunits *res, unsigned char si)
{
  if (!is_valid_expression_start())
    return false;
  units x;
  if (is_valid_expression(&x, si, false /* is_parenthesized */)) {
    *res = vunits(x);
    return true;
  }
  else
    return false;
}

bool get_hunits(hunits *res, unsigned char si)
{
  if (!is_valid_expression_start())
    return false;
  units x;
  if (is_valid_expression(&x, si, false /* is_parenthesized */)) {
    *res = hunits(x);
    return true;
  }
  else
    return false;
}

// for \B

bool get_number_rigidly(units *res, unsigned char si)
{
  if (!is_valid_expression_start())
    return false;
  units x;
  if (is_valid_expression(&x, si, false /* is_parenthesized */,
			  true /* is_mandatory */)) {
    *res = x;
    return true;
  }
  else
    return false;
}

bool read_measurement(units *res, unsigned char si)
{
  if (!is_valid_expression_start())
    return false;
  units x;
  if (is_valid_expression(&x, si, false /* is_parenthesized */)) {
    *res = x;
    return true;
  }
  else
    return false;
}

bool read_integer(int *res)
{
  if (!is_valid_expression_start())
    return false;
  units x;
  if (is_valid_expression(&x, 0, false /* is_parenthesized */)) {
    *res = x;
    return true;
  }
  else
    return false;
}

enum incr_number_result { INVALID, ASSIGN, INCREMENT, DECREMENT };

static incr_number_result get_incr_number(units *res, unsigned char);

bool get_vunits(vunits *res, unsigned char si, vunits prev_value)
{
  units v;
  // Use a primitive temporary because having the ckd macros store to
  // &(res->n) requires `friend` access and produces wrong results.
  int i;
  switch (get_incr_number(&v, si)) {
  case INVALID:
    return false;
  case ASSIGN:
    *res = v;
    break;
  case INCREMENT:
    if (ckd_add(&i, prev_value.to_units(), v))
      warning(WARN_RANGE, "integer incrementation saturated");
    *res = i;
    break;
  case DECREMENT:
    if (ckd_sub(&i, prev_value.to_units(), v))
      warning(WARN_RANGE, "integer decrementation saturated");
    *res = i;
    break;
  default:
    assert(0 == "unhandled case in get_vunits()");
  }
  return true;
}

bool get_hunits(hunits *res, unsigned char si, hunits prev_value)
{
  units h;
  // Use a primitive temporary because having the ckd macros store to
  // &(res->n) requires `friend` access and produces wrong results.
  int i;
  switch (get_incr_number(&h, si)) {
  case INVALID:
    return false;
  case ASSIGN:
    *res = h;
    break;
  case INCREMENT:
    if (ckd_add(&i, prev_value.to_units(), h))
      warning(WARN_RANGE, "integer incrementation saturated");
    *res = i;
    break;
  case DECREMENT:
    if (ckd_sub(&i, prev_value.to_units(), h))
      warning(WARN_RANGE, "integer decrementation saturated");
    *res = i;
    break;
  default:
    assert(0 == "unhandled case in get_hunits()");
  }
  return true;
}

bool read_measurement(units *res, unsigned char si, units prev_value)
{
  units u;
  switch (get_incr_number(&u, si)) {
  case INVALID:
    return false;
  case ASSIGN:
    *res = u;
    break;
  case INCREMENT:
    if (ckd_add(res, prev_value, u))
      warning(WARN_RANGE, "integer incrementation saturated");
    break;
  case DECREMENT:
    if (ckd_sub(res, prev_value, u))
      warning(WARN_RANGE, "integer decrementation saturated");
    break;
  default:
    assert(0 == "unhandled case in read_measurement()");
  }
  return true;
}

bool read_integer(int *res, int prev_value)
{
  units i;
  switch (get_incr_number(&i, 0)) {
  case INVALID:
    return false;
  case ASSIGN:
    *res = i;
    break;
  case INCREMENT:
    if (ckd_add(res, prev_value, i))
      warning(WARN_RANGE, "integer incrementation saturated");
    break;
  case DECREMENT:
    if (ckd_sub(res, prev_value, i))
      warning(WARN_RANGE, "integer decrementation saturated");
    break;
  default:
    assert(0 == "unhandled case in read_integer()");
  }
  return true;
}


static incr_number_result get_incr_number(units *res, unsigned char si)
{
  if (!is_valid_expression_start())
    return INVALID;
  incr_number_result result = ASSIGN;
  if (tok.ch() == '+') {
    tok.next();
    result = INCREMENT;
  }
  else if (tok.ch() == '-') {
    tok.next();
    result = DECREMENT;
  }
  if (is_valid_expression(res, si, false /* is_parenthesized */))
    return result;
  else
    return INVALID;
}

static bool is_valid_expression_start()
{
  while (tok.is_space())
    tok.next();
  if (tok.is_newline()) {
    warning(WARN_MISSING, "numeric expression missing");
    return false;
  }
  return true;
}

enum { OP_LEQ = 'L', OP_GEQ = 'G', OP_MAX = 'X', OP_MIN = 'N' };

#define SCALING_UNITS "icfPmnpuvMsz"

static bool is_valid_term(units *u, int scaling_unit,
			  bool is_parenthesized, bool is_mandatory);

static bool is_valid_expression(units *u, int scaling_unit,
				bool is_parenthesized,
				bool is_mandatory)
{
  int result = is_valid_term(u, scaling_unit, is_parenthesized,
			     is_mandatory);
  while (result) {
    if (is_parenthesized)
      tok.skip();
    int op = tok.ch();
    switch (op) {
    case '+':
    case '-':
    case '/':
    case '*':
    case '%':
    case ':':
    case '&':
      tok.next();
      break;
    case '>':
      tok.next();
      if (tok.ch() == '=') {
	tok.next();
	op = OP_GEQ;
      }
      else if (tok.ch() == '?') {
	tok.next();
	op = OP_MAX;
      }
      break;
    case '<':
      tok.next();
      if (tok.ch() == '=') {
	tok.next();
	op = OP_LEQ;
      }
      else if (tok.ch() == '?') {
	tok.next();
	op = OP_MIN;
      }
      break;
    case '=':
      tok.next();
      if (tok.ch() == '=')
	tok.next();
      break;
    default:
      return result;
    }
    units u2;
    if (!is_valid_term(&u2, scaling_unit, is_parenthesized,
		       is_mandatory))
      return false;
    switch (op) {
    case '<':
      *u = *u < u2;
      break;
    case '>':
      *u = *u > u2;
      break;
    case OP_LEQ:
      *u = *u <= u2;
      break;
    case OP_GEQ:
      *u = *u >= u2;
      break;
    case OP_MIN:
      if (*u > u2)
	*u = u2;
      break;
    case OP_MAX:
      if (*u < u2)
	*u = u2;
      break;
    case '=':
      *u = *u == u2;
      break;
    case '&':
      *u = *u > 0 && u2 > 0;
      break;
    case ':':
      *u = *u > 0 || u2 > 0;
      break;
    case '+':
      if (ckd_add(u, *u, u2)) {
	warning(WARN_RANGE, "integer addition saturated");
	return false;
      }
      break;
    case '-':
      if (ckd_sub(u, *u, u2)) {
	warning(WARN_RANGE, "integer subtraction saturated");
	return false;
      }
      break;
    case '*':
      if (ckd_mul(u, *u, u2)) {
	warning(WARN_RANGE, "integer multiplication saturated");
	return false;
      }
      break;
    case '/':
      if (u2 == 0) {
	error("division by zero");
	return false;
      }
      *u /= u2;
      break;
    case '%':
      if (u2 == 0) {
	error("modulus by zero");
	return false;
      }
      *u %= u2;
      break;
    default:
      assert(0 == "unhandled case of operator");
    }
  }
  return result;
}

static bool is_valid_term(units *u, int scaling_unit,
			  bool is_parenthesized, bool is_mandatory)
{
  bool is_negative = false;
  bool is_overflowing = false;
  units saved_u = 0; // for use when reading an overlong number
  for (;;)
    if (is_parenthesized && tok.is_space())
      tok.next();
    else if (tok.ch() == '+')
      tok.next();
    else if (tok.ch() == '-') {
      tok.next();
      is_negative = !is_negative;
    }
    else
      break;
  unsigned char c = tok.ch();
  switch (c) {
  case '|':
    // | is not restricted to the outermost level
    // tbl uses this
    tok.next();
    if (!is_valid_term(u, scaling_unit, is_parenthesized, is_mandatory))
      return false;
    int tmp, position;
    position = (scaling_unit == 'v'
		? curdiv->get_vertical_position().to_units()
		: curenv->get_input_line_position().to_units());
    if (ckd_sub(&tmp, *u, position)) {
      tmp = INT_MAX;
      warning(WARN_RANGE, "integer value saturated");
    }
    *u = tmp;
    if (is_negative)
      *u = -*u;
    return true;
  case '(':
    tok.next();
    c = tok.ch();
    if (c == ')') {
      if (is_mandatory)
	return false;
      warning(WARN_SYNTAX, "empty parentheses");
      tok.next();
      *u = 0;
      return true;
    }
    else if (c != 0 && strchr(SCALING_UNITS, c) != 0) {
      tok.next();
      if (tok.ch() == ';') {
	tok.next();
	scaling_unit = c;
      }
      else {
	error("expected ';' after scaling unit, got %1",
	      tok.description());
	return false;
      }
    }
    else if (c == ';') {
      scaling_unit = 0;
      tok.next();
    }
    if (!is_valid_expression(u, scaling_unit,
			     true /* is_parenthesized */, is_mandatory))
      return false;
    tok.skip();
    if (tok.ch() != ')') {
      if (is_mandatory)
	return false;
      warning(WARN_SYNTAX, "expected ')', got %1", tok.description());
    }
    else
      tok.next();
    if (is_negative) {
      // Why?  Consider -(INT_MIN) in two's complement.
      if (ckd_mul(u, *u, -1))
	warning(WARN_RANGE, "integer multiplication saturated");
    }
    return true;
  case '.':
    *u = 0;
    break;
  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9':
    *u = 0;
    do {
      // If wrapping, don't `break`; eat and discard further digits.
      if (!is_overflowing) {
	  saved_u = *u;
	  if (ckd_mul(u, *u, 10))
	    is_overflowing = true;
	  if (ckd_add(u, *u, c - '0'))
	    is_overflowing = true;
	  if (is_overflowing)
	    *u = saved_u;
	}
      tok.next();
      c = tok.ch();
    } while (csdigit(c));
    if (is_overflowing)
      warning(WARN_RANGE, "integer value saturated");
    break;
  case '/':
  case '*':
  case '%':
  case ':':
  case '&':
  case '>':
  case '<':
  case '=':
    warning(WARN_SYNTAX, "empty left operand to '%1' operator", c);
    *u = 0;
    return !is_mandatory;
  default:
    error("ignoring invalid numeric expression starting with %1",
	  tok.description());
    return false;
  }
  int divisor = 1;
  if (tok.ch() == '.') {
    tok.next();
    for (;;) {
      c = tok.ch();
      if (!csdigit(c))
	break;
      // we may multiply the divisor by 254 later on
      if (divisor <= INT_MAX / 2540 && *u <= (INT_MAX - 9) / 10) {
	*u *= 10;
	*u += c - '0';
	divisor *= 10;
      }
      tok.next();
    }
  }
  int si = scaling_unit;
  bool do_next = false;
  if (((c = tok.ch()) != 0)
      && (strchr(SCALING_UNITS, c) != 0 /* nullptr */)) {
    switch (scaling_unit) {
    case 0:
      warning(WARN_SCALE, "scaling unit '%1' invalid in context", c);
      break;
    case 'f':
      if (c != 'f' && c != 'u') {
	warning(WARN_SCALE, "'%1' scaling unit invalid in context;"
		" use 'f' or 'u'", c);
	break;
      }
      si = c;
      break;
    case 'z':
      if (c != 'u' && c != 'z' && c != 'p' && c != 's') {
	warning(WARN_SCALE, "'%1' scaling unit invalid in context;"
		" use 'z', 'p', 's', or 'u'", c);
	break;
      }
      si = c;
      break;
    case 'u':
      si = c;
      break;
    default:
      if (c == 'z') {
	warning(WARN_SCALE, "'z' scaling unit invalid in context");
	break;
      }
      si = c;
      break;
    }
    // Don't do tok.next() here because the next token might be \s,
    // which would affect the interpretation of 'm'.
    do_next = true;
  }
  switch (si) {
  case 'i':
    *u = scale(*u, units_per_inch, divisor);
    break;
  case 'c':
    *u = scale(*u, units_per_inch * 100, divisor * 254);
    break;
  case 0:
  case 'u':
    if (divisor != 1)
      *u /= divisor;
    break;
  case 'f':
    *u = scale(*u, 65536, divisor);
    break;
  case 'p':
    *u = scale(*u, units_per_inch, divisor * 72);
    break;
  case 'P':
    *u = scale(*u, units_per_inch, divisor * 6);
    break;
  case 'm':
    {
      // Convert to hunits so that with -Tascii 'm' behaves as in nroff.
      hunits em = curenv->get_size();
      *u = scale(*u, em.is_zero() ? hresolution : em.to_units(),
		 divisor);
    }
    break;
  case 'M':
    {
      hunits em = curenv->get_size();
      *u = scale(*u, em.is_zero() ? hresolution : em.to_units(),
		 (divisor * 100));
    }
    break;
  case 'n':
    {
      // Convert to hunits so that with -Tascii 'n' behaves as in nroff.
      hunits en = curenv->get_size() / 2;
      *u = scale(*u, en.is_zero() ? hresolution : en.to_units(),
		 divisor);
    }
    break;
  case 'v':
    *u = scale(*u, curenv->get_vertical_spacing().to_units(), divisor);
    break;
  case 's':
    while (divisor > INT_MAX / (sizescale * 72)) {
      divisor /= 10;
      *u /= 10;
    }
    *u = scale(*u, units_per_inch, divisor * sizescale * 72);
    break;
  case 'z':
    *u = scale(*u, sizescale, divisor);
    break;
  default:
    assert(0 == "unhandled case of scaling unit");
  }
  if (do_next)
    tok.next();
  if (is_negative) {
    if (ckd_mul(u, *u, -1))
      warning(WARN_RANGE, "integer multiplication saturated");
  }
  return true;
}

units scale(units n, units x, units y)
{
  assert(x >= 0 && y > 0);
  if (x == 0)
    return 0;
  if (n >= 0) {
    if (n <= INT_MAX / x)
      return (n * x) / y;
  }
  else {
    // I'd prefer to say "(unsigned int(n))", but C++ doesn't seem to
    // permit function-style construction with a type qualifier.  --GBR
    if (-(unsigned(n)) <= -(unsigned(INT_MIN)) / x)
      return (n * x) / y;
  }
  double res = n * double(x) / double(y);
  if (res > INT_MAX) {
    warning(WARN_RANGE, "integer value saturated");
    return INT_MAX;
  }
  else if (res < INT_MIN) {
    warning(WARN_RANGE, "integer value saturated");
    return INT_MIN;
  }
  return int(res);
}

vunits::vunits(units x)
{
  if (vresolution == 1)
    n = x;
  else {
    // Don't depend on rounding direction when dividing neg integers.
    int vcrement = (vresolution / 2) - 1;
    bool is_overflowing = false;
    if (x < 0) {
      if (ckd_add(&n, -x, vcrement))
	is_overflowing = true;
      n = -n;
    }
    else {
      if (ckd_add(&n, x, vcrement))
	is_overflowing = true;
    }
    if (is_overflowing) {
      if (x < 0) {
	warning(WARN_RANGE, "integer value saturated");
	n = INT_MIN;
      }
      else {
	warning(WARN_RANGE, "integer value saturated");
	n = INT_MAX;
      }
    }
    n /= vresolution;
  }
}

hunits::hunits(units x)
{
  if (hresolution == 1)
    n = x;
  else {
    // Don't depend on rounding direction when dividing neg integers.
    int hcrement = (hresolution / 2) - 1;
    bool is_overflowing = false;
    if (x < 0) {
      if (ckd_add(&n, -x, hcrement))
	is_overflowing = true;
      n = -n;
    }
    else {
      if (ckd_add(&n, x, hcrement))
	is_overflowing = true;
    }
    if (is_overflowing) {
      if (x < 0) {
	warning(WARN_RANGE, "integer value saturated");
	n = INT_MIN;
      }
      else {
	warning(WARN_RANGE, "integer value saturated");
	n = INT_MAX;
      }
    }
    n /= hresolution;
  }
}

// Local Variables:
// fill-column: 72
// mode: C++
// End:
// vim: set cindent noexpandtab shiftwidth=2 textwidth=72:
