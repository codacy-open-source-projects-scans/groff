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

# Regression-test Savannah #68260.  Brace escape sequences should not be
# stored in a string definition.

input='.
.ec @
.ds s1 AB@{CD@}EF
.tm s1=@*(s1
.ie 1 @{@
.  ds s2 abc@}def
.tm s2=@*(s2
.el .tm OOPS
.'

output=$(printf '%s\n' "$input" | "$groff" 2>&1)
echo "$output"

echo "checking that brace escape sequences are discarded" \
    "from string definition outside of macro definition" >&2
echo "$output" | grep -qx 's1=ABCDEF' || wail

# The formatter honors the closing brace escape sequence for control
# flow purposes, but does not store it in the string, and doesn't
# terminate the branch being taken until the end of the input line.
# The last element of behavior is shocking to C programmers, but it's
# what troff has always done.  See "Conditional Blocks" in groff's
# Texinfo manual.

echo "checking that brace escape sequences are discarded" \
    "from string definition inside of macro definition" >&2
output=$(printf '%s\n' "$input" | "$groff" 2>&1)
echo "$output" | grep -qx 's2=abcdef' || wail

test -z "$fail"

# vim:set autoindent expandtab shiftwidth=4 tabstop=4 textwidth=72:
