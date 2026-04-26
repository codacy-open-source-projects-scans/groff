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

# Unit-test roman numeral-based register interpolation.
#
# There are two cases: normal/GNU style, representing values up to 3,999
# ("input1"); and AT&T style, using unorthodox 'W' and 'Z' numerals to
# represent values up to 39,999 ("input2").

input1='.
.nf
.nr r 0 +1
.while \nr<3999 \{\
.ie \nU .af r I
.el     .af r i
\n+r
.af r 0
.\}
.'

output1a=$(printf '%s\n' "$input1" | "$groff" -T ascii | nl -ba)

echo "checking that normal lowercase roman numeral register" \
    "interpolation works" >&2
echo "$output1a" | grep -Eq "3999[[:space:]]+mmmcmxcix" || wail

output1b=$(printf '%s\n' "$input1" | "$groff" -rU1 -T ascii | nl -ba)

echo "checking that normal uppercase roman numeral register" \
    "interpolation works" >&2
echo "$output1b" | grep -Eq "3999[[:space:]]+MMMCMXCIX" || wail

# There is no looping construct in AT&T troff.  Its alternative idiom
# is macro call recursion, but that can be a problem when it demands
# ~40k nested stack frames ("interpolation depths" in groff parlance),
# never mind whatever that does to the stack in the C/C++ runtime.
#
# So we just lazily (and speedily) check a few values of interest.
input2='.
.ec @
.de PR
.nr r @@$1
.af r 0
@@nr
.ie @nU .af r I
.el     .af r i
@@nr
.br
..
.PR 4000
.PR 9000
.PR 39999
.'

output2a=$(printf '%s\n' "$input2" | "$groff" -C -T ascii)
output2a=$(echo $output2a) # condense onto one line

echo "checking that AT&T-style lowercase roman numeral register" \
    "interpolation works" >&2
echo "$output2a" | grep -qx "4000 mw 9000 mz 39999 zzzmzcmxcix" || wail

output2b=$(printf '%s\n' "$input2" | "$groff" -C -rU1 -T ascii)
output2b=$(echo $output2b) # condense onto one line

echo "checking that AT&T-style uppercase roman numeral register" \
    "interpolation works" >&2
echo "$output2b" | grep -qx "4000 MW 9000 MZ 39999 ZZZMZCMXCIX" || wail

echo "checking that lowercase roman numeral register interpolation" \
    "format works with nonpositive values" >&2
printf '.nr r0\n.nr r1 -1\n\n.af r0 i\n.af r1 i\n\\n(r0 \\n(r1\n' \
    | "$groff" -T ascii | grep -qx -- '0 -i' || wail

echo "checking that uppercase roman numeral register interpolation" \
    "format works with nonpositive values" >&2
printf '.nr r0\n.nr r1 -1\n\n.af r0 I\n.af r1 I\n\\n(r0 \\n(r1\n' \
    | "$groff" -T ascii | grep -qx -- '0 -I' || wail

test -z "$fail"

# vim:set autoindent expandtab shiftwidth=4 tabstop=4 textwidth=72:
