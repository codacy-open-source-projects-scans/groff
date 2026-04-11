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
.ll 50n
.na
.nf
I am not a Labor Leader;
I do not want you to follow me or anyone else;
if you are looking for a Moses
to lead you out of this capitalist wilderness,
you will stay right where you are.
.fi
I would not lead you into the promised land if I could,
because if I led you in,
someone else would lead you out.
You must use your heads as well as your hands,
and get yourself out of your present condition.
.'

output=$(printf '%s\n' "$input" | "$groff" -T ascii)
echo "$output"

echo "checking that 'nf' request works" >&2
echo "$output" | grep -qx "I am not a Labor Leader;" || wail

echo "checking that 'fi' request works" >&2
echo "$output" | grep -q 'you out\.  You must use your heads' || wail

test -z "$fail"

# vim:set autoindent expandtab shiftwidth=4 tabstop=4 textwidth=72:
