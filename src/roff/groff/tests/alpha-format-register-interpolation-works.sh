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

# Unit-test alphabetic register interpolation format.
#
# Also test nonpositive register values.

input='.
.ec @
.de PR
.nr r @@$1
.af r 0
@@nr
.ie @nU .af r A
.el     .af r a
@@nr
.br
..
.PR -1
.PR 0
.PR 1
.PR 26
.PR 27
.'

output1=$(printf '%s\n' "$input" | "$groff" -T ascii)
output1=$(echo $output1) # condense onto one line

echo "checking that lowercase alphabetic register interpolation" \
    "format works" >&2
echo "$output1" | grep -qx -- "-1 -a 0 0 1 a 26 z 27 aa" || wail

output2=$(printf '%s\n' "$input" | "$groff" -rU1 -T ascii)
output2=$(echo $output2) # condense onto one line

echo "checking that uppercase alphabetic register interpolation" \
    "format works" >&2
echo "$output2" | grep -qx -- "-1 -A 0 0 1 A 26 Z 27 AA" || wail

test -z "$fail"

# vim:set autoindent expandtab shiftwidth=4 tabstop=4 textwidth=72:
