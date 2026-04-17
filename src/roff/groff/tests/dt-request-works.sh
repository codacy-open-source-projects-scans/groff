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

# Unit-test `dt` request.

# Test trap planting.

input='.
.de TT
BOING
.br
..
.di DD
.dt 3v TT
.nf
foo
bar
.sp
qux
.di
.DD
.'

output=$(printf '%s\n' "$input" | "$groff" -a 2>/dev/null)
echo "$output"
output=$(echo $output) # condense onto one line
echo "$output" | grep -q "bar BOING qux" || wail

# Test trap removal.

input2='.
.de TT
WHOOPS
.br
..
.di DD
.dt 3v TT
.nf
foo
bar
.dt
.sp
baz
.di
.DD
.'

output2=$(printf '%s\n' "$input2" | "$groff" -a 2>/dev/null)
echo "$output2"
output2=$(echo $output2) # condense onto one line
echo "$output2" | grep -q "foo bar baz" || wail

test -z "$fail"

# vim:set autoindent expandtab shiftwidth=4 tabstop=4 textwidth=72:
