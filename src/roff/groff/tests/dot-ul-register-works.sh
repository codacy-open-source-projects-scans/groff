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

# Unit test .ul register.

input='.
.ec @
.nf
.ul 2
foo @n[.ul]
bar @n[.ul]
baz @n[.ul]
.cu 1
qux @n[.ul]
jat @n[.ul]
.'

# Omitting `-P -cbou` would better illustrate the `cu` and `ul` requests
# themselves, but is more tedious to pattern-match.
#
# Expected output:
#  foo 2
#  bar 1
#  baz 0
#  qux 1
#  jat 0
output=$(echo "$input" | $groff -T ascii -P -cbou)
echo "$output"
echo "$output" | grep -qx "foo 2" || exit 1
echo "$output" | grep -qx "bar 1" || exit 1
echo "$output" | grep -qx "baz 0" || exit 1
# Backspace-overstriking still occurs when the continuous underlining
# device extension is used.  That's probably correct given the
# long-standing semantics of grotty(1)'s `-o` and `-u` options, but it
# may also militate for a means of obtaining "least common denominator
# of video terminal and Teletype Model 37" output, which is what some
# external tools expect of grotty anyway.  See Savannah #67947.
echo "$output" | grep -qx "qux.*1" || exit 1
echo "$output" | grep -qx "jat 0" || exit 1

# vim:set autoindent expandtab shiftwidth=2 tabstop=2 textwidth=72:
