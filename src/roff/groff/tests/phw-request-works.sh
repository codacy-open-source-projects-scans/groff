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
.phw foobar
.hw baz-qux
.phw bazqux
.hw tab-le qqq-xxx-zzz
.phw t-a-b-l-e * qqqxxxzzz
.'

output=$(printf '%s\n' "$input" | "$groff" -W char 2>&1)
echo "$output"

# No exception word for 'foobar' is defined; expect no output.
echo "checking report of hyphenation exception word 'foobar'" >&2
echo "$output" | grep -qx "foo.*bar" && wail

echo "checking report of hyphenation exception word 'bazqux'" >&2
echo "$output" | grep -qx "baz-qux" || wail

echo "checking report of hyphenation exception word 'table'" >&2
echo "$output" | grep -qx "tab-le" || wail # deliberately incorrect

echo "checking report of hyphenation exception word 'qqqxxxzzz'" >&2
echo "$output" | grep -qx "qqq-xxx-zzz" || wail

echo "checking for error indicator character" >&2
echo "$output" | grep -qx "#" && wail

test -z "$fail"

# vim:set autoindent expandtab shiftwidth=4 tabstop=4 textwidth=72:
