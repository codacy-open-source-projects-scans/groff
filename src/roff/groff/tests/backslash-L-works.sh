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

# Unit-test `\L` escape sequence.

input='.
.ec @
Draw a vertical rule from one baseline to another six vees below.
.br
foo
@L"6v"
bar
.'

output=$(printf '%s\n' "$input" | "$groff" -T ascii 2>&1)
echo "$output"

echo "checking that vertical rule starts at or below baseline" \
    "where it begins" >&2
echo "$output" | grep -qx "foo" || wail

echo "checking that vertical rule has correct length" >&2
test $(echo "$output" | grep -c "|") -eq 6 || wail

# test -z "$fail"

# vim:set autoindent expandtab shiftwidth=4 tabstop=4 textwidth=72:
