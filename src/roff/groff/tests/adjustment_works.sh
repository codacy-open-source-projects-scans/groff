#!/bin/sh
#
# Copyright (C) 2021-2024 Free Software Foundation, Inc.
#
# This file is part of groff.
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
#

groff="${abs_top_builddir:-.}/test-groff"

fail=

wail () {
  echo "...FAILED" >&2
  fail=yes
}

input='.pl 1v
.ll 9n
foo bar\p
.na
foo bar\p
.ad l
foo bar\p
.na
foo bar\p
.ad
foo bar\p
.ad l
.ad b
foo bar\p
.na
foo bar\p
.ad c
foo bar\p
.na
foo bar\p
.ad
foo bar\p
.ad r
foo bar\p
.na
foo bar\p
.ad
foo bar\p
.ad b
.ad 100
foo bar\p'

output=$(echo "$input" | "$groff" -T ascii)
echo "$output"

# Expected output:
#
#   foo   bar
#   foo bar
#   foo bar
#   foo bar
#   foo   bar
#   foo bar
#    foo bar
#   foo bar
#     foo bar
#   foo bar
#     foo bar
#     foo bar

B='foo   bar' # 3 spaces
L='foo bar' # left or off
C=' foo bar' # trailing space truncated
R='  foo bar' # 2 leading spaces

echo "verifying default adjustment mode 'b'" >&2
echo "$output" | sed -n '1p' | grep -Fqx "$B" || wail

echo "verifying that '.na' turns off adjustment and aligns left" >&2
echo "$output" | sed -n '2p' | grep -Fqx "$L" || wail

echo "verifying that '.ad l' aligns left" >&2
echo "$output" | sed -n '3p' | grep -Fqx "$L" || wail

echo "verifying that '.na' turns off adjustment and aligns left" >&2
echo "$output" | sed -n '4p' | grep -Fqx "$L" || wail

echo "verifying that '.ad' restores adjustment" >&2
echo "$output" | sed -n '5p' | grep -Fqx "$B" || wail

echo "verifying that '.ad b' enables adjustment" >&2
echo "$output" | sed -n '6p' | grep -Fqx "$B" || wail

echo "verifying that '.na' turns off adjustment and aligns left" >&2
echo "$output" | sed -n '7p' | grep -Fqx "$L" || wail

echo "verifying that '.ad c' aligns to the center" >&2
echo "$output" | sed -n '8p' | grep -Fqx "$C" || wail

echo "verifying that '.na' turns off adjustment and aligns left" >&2
echo "$output" | sed -n '9p' | grep -Fqx "$L" || wail

echo "verifying that '.ad' restores previous alignment (center)" >&2
echo "$output" | sed -n '10p' | grep -Fqx "$C" || wail

echo "verifying that '.ad r' aligns right" >&2
echo "$output" | sed -n '11p' | grep -Fqx "$R" || wail

echo "verifying that '.na' turns off adjustment and aligns left" >&2
echo "$output" | sed -n '12p' | grep -Fqx "$L" || wail

echo "verifying that '.ad' restores previous alignment (right)" >&2
echo "$output" | sed -n '13p' | grep -Fqx "$R" || wail

echo "verifying that out-of-range mode works like '.ad b'" >&2
echo "$output" | sed -n '14p' | grep -Fqx "$B" || wail

test -z "$fail"

# vim:set ai et sw=4 ts=4 tw=72:
