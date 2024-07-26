#!/bin/sh
#
# Copyright (C) 2024 Free Software Foundation, Inc.
#
# This file is part of groff.
#
# groff is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free
# Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# groff is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.
#

groff="${abs_top_builddir:-.}/test-groff"

fail=

wail () {
  echo "...FAILED" >&2
  fail=yes
}

# Unit-test saturating arithmetic.

# Start with a couple of cases that proved fragile when developing it.
#
# Incrementing a formatter internal via a request should produce the
# expected changes in magnitude (not repeat or square them).
#
# Vertical spacing exercises `vunits` and line length, `hunits`.

input='.
.ec @
.nf
.vs +40u
1: @&.v=@n(.v
.ll -2
2: @&.l=@n(.l
.ll 2147483647u
3: @&.l=@n(.l, .H=@n(.H
.ll
.pl 2147483647u
4: @&.p=@n(.p, .V=@n(.V
.pl
.'

# Expected:
#
# 1: .v=80
#
# 2: .l=1512
#
# 3: .l=24, .H=24
#
# 4: .p=40, .V=40
#
# The blank lines are due to the `vs` increase.

output=$(echo "$input" | "$groff" -T ascii)
echo "$output"

echo "checking that vertical spacing is correctly incremented" >&2
echo "$output" | grep -Fqx '1: .v=80' || wail

echo "checking that line length is correctly incremented" >&2
echo "$output" | grep -Fqx '2: .l=1512' || wail

echo "checking that setting huge line length does not overflow" >&2
echo "$output" | grep -Fqx '3: .l=24, .H=24' || wail

echo "checking that setting huge page length does not overflow" >&2
echo "$output" | grep -Fqx '4: .p=40, .V=40' || wail

# Exercise boundary values.

input='.
.ec @
.nf
.nr i 99999999999999999999
1: 99999999999999999999 -> @ni
.nr i (-99999999999999999999)
2: (-99999999999999999999) -> @ni
.nr i 2147483647
3: assign 2147483647 -> @ni
.nr i +1
4: incr 2147483647 -> @ni
.nr i 2147483647
.nr i (-1)*@ni
5: (-1)*2147483647 -> @ni
.nr i -1
6: decr -2147483648 -> @ni
.nr i -2147483648
.nr i (-1)*@ni
7: (-1)*(-2147483648) -> @ni
.'

output=$(echo "$input" | "$groff" -T ascii)
echo "$output"

# saturating: 2147483647
echo "checking assignment of huge value to register" >&2
echo "$output" \
  | grep -Fqx '1: 99999999999999999999 -> 1410065407' || wail

# saturating: -2147483648
echo "checking assignment of huge negative value to register" >&2
echo "$output" \
  | grep -Fqx '2: (-99999999999999999999) -> -1410065407' || wail

echo "checking assignment of 2^31 - 1 to register" >&2
echo "$output" | grep -Fqx '3: assign 2147483647 -> 2147483647' || wail

# saturating: -2147483648
echo "checking saturating incrementation of register" >&2
echo "$output" | grep -Fqx '4: incr 2147483647 -> -2147483648' || wail

echo "checking negation of positive register" >&2
echo "$output" | grep -Fqx '5: (-1)*2147483647 -> -2147483647' || wail

echo "checking saturating decrementation of register" >&2
echo "$output" | grep -Fqx '6: decr -2147483648 -> -2147483648' || wail

# saturating: 2147483647
echo "checking negation of negatively saturated register" >&2
echo "$output" | grep -Fqx '7: (-1)*(-2147483648) -> 0' \
  || wail

test -z "$fail"

# vim:set autoindent expandtab shiftwidth=2 tabstop=2 textwidth=72:
