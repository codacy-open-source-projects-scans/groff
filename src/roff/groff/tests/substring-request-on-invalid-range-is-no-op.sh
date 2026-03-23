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

# Verify that invoking the `substring` request with both index arguments
# invalid performs no operation.

input='
.nf
.ds r abc
.substring r -4 8
\*r
.ds s def
.substring s 4 -5
\*s
.ds t ghi
.substring t 4 8
\*t
.ds u jkl
.substring u -4 -5
\*u
.'


output=$(printf '%s\n' "$input" | "$groff" -a -ww)
echo "$output"

echo "verifying that 'substring' request with out-of-range index" \
    "arguments (starting index negative) performs no operation" >&2
echo "$output" | grep -qx "abc" || wail

echo "verifying that 'substring' request with out-of-range index" \
    "arguments (ending index negative) performs no operation" >&2
echo "$output" | grep -qx "def" || wail

echo "verifying that 'substring' request with out-of-range index" \
    "arguments (both indices positive) performs no operation" >&2
echo "$output" | grep -qx "ghi" || wail

echo "verifying that 'substring' request with out-of-range index" \
    "arguments (both indices negative) performs no operation" >&2
echo "$output" | grep -qx "jkl" || wail

test -z "$fail"

# vim:set autoindent expandtab shiftwidth=4 tabstop=4 textwidth=72:
