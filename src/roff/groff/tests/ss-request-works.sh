#!/bin/sh
#
# Copyright 2026 G. Branden Robinson
#
# This file is part of groff, the GNU roff typesetting system.
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

groff="${abs_top_builddir:-.}/test-groff"

fail=

wail () {
   echo "...FAILED"
   fail=yes
}

input='.
.ll 48n
.ss 12
1.\~J. Fict. Ch. Soc. 6 (2020), 3\[en]14.
.ss 12 48 \" applies to next sentence ending
Reprints no longer available through FCS.
2.\~Better known for other work.
.'

# Expected output:
#
# 1.  J.  Fict. Ch. Soc. 6 (2020), 3-14.  Reprints
# no longer available through FCS.      2.  Better
# known for other work.

output=$(printf '%s\n' "$input" | "$groff" -T ascii)
echo "$output"

echo "checking operation of 'ss' request with one argument"
echo "$output" | grep -q "Soc\. 6 (2020), 3-14\.  Reprints$" || wail

echo "checking operation of 'ss' request with two arguments"
echo "$output" | grep -q "through FCS\.      2\.  Better$" || wail

test -z "$fail"

# vim:set autoindent expandtab shiftwidth=4 tabstop=4 textwidth=72:
