#!/bin/sh
#
# Copyright (C) 2024 Free Software Foundation, Inc.
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
  echo ...FAILED >&2
  fail=YES
}

input='.
.nf
\X#bogus1: esc \%\[u1F63C]\\[u1F00]\-\[`a]#
.device bogus1: req \%\[u1F63C]\\[u1F00]\-\[`a]
.ec @
@X#bogus2: esc @%@[u1F63C]@@[u1F00]@-@[`a]#
.device bogus2: req @%@[u1F63C]@@[u1F00]@-@[`a]
.'

output=$(printf '%s\n' "$input" | "$groff" -T ps -Z 2> /dev/null \
  | grep '^x X')
error=$(printf '%s\n' "$input" | "$groff" -T ps -Z 2>&1 > /dev/null)

echo "$output"
echo "$error"

# Expected:
#
# x X bogus1: esc \[u1F63C]\[u1F00]-\[u00E0]
# x X bogus1: req @%\[u1F63C]\[u1F00]@-\[`a]
# x X bogus2: esc \[u1F63C]\[u1F00]-\[u00E0]
# x X bogus2: req @%@[u1F63C]@[u1F00]@-@[`a]

echo "checking X escape sequence, default escape character" >&2
echo "$output" \
  | grep -Fqx 'x X bogus1: esc \[u1F63C]\[u1F00]-\[u00E0]' || wail

#echo "checking device request, default escape character" >&2
#echo "$output" \
#  | grep -qx 'x X bogus1: req \\\[u1F00\] -'"'"'"`^\\~' \
#  || wail

echo "checking X escape sequence, alternate escape character" >&2
echo "$output" \
  | grep -Fqx 'x X bogus2: esc \[u1F63C]\[u1F00]-\[u00E0]' || wail

#echo "checking device request, alternate escape character" >&2
#echo "$output" \
#  | grep -qx 'x X bogus2: req \\\[u1F00\] -'"'"'"`^\\~' \
#  || wail

input='.
.nf
\X#bogus3: \[dq]\[sh]\[Do]\[aq]\[sl]\[at]#
\X#bogus4: \[lB]\[rs]\[rB]\[ha]#
\X#bogus5: \[lC]\[ba]\[or]\[rC]\[ti]#
.\"device bogus3: \[dq]\[sh]\[Do]\[aq]\[sl]\[at]
.\"device bogus4: \[lB]\[rs]\[rB]\[ha]
.\"device bogus5: \[lC]\[ba]\[or]\[rC]\[ti]
.'

# Expected:
#
# x X bogus3: "#$'/@
# x X bogus4: [\]^
# x X bogus5: {||}~

output=$(printf '%s\n' "$input" | "$groff" -T ps -Z 2> /dev/null \
  | grep '^x X')
echo "$output"

echo "checking X escape sequence, conversions to basic Latin (1/3)" >&2
echo "$output" | grep -Fqx 'x X bogus3: "#$'"'"'/@' || wail

echo "checking X escape sequence, conversions to basic Latin (2/3)" >&2
echo "$output" | grep -Fqx 'x X bogus4: [\]^' || wail

echo "checking X escape sequence, conversions to basic Latin (3/3)" >&2
echo "$output" | grep -Fqx 'x X bogus5: {||}~' || wail

input='.
.nf
\X#bogus6: '"'"'-^`~#
.\"device bogus6: '"'"'-^`~
.'

# Expected:
#
# x X bogus6: \[u2019]\[u2010]\[u0302]\[u0300]\[u0303]

output=$(printf '%s\n' "$input" | "$groff" -T ps -Z 2> /dev/null \
  | grep '^x X')
echo "$output"

echo "checking X escape sequence, conversions from basic Latin" >&2
echo "$output" \
  | grep -Fqx 'x X bogus6: \[u2019]\[u2010]\[u0302]\[u0300]\[u0303]' \
  || wail

test -z "$fail"

# vim:set autoindent expandtab shiftwidth=2 tabstop=2 textwidth=72:
