#!/bin/sh
#
# Copyright (C) 2022-2024 Free Software Foundation, Inc.
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

# test for upTeX dvi with Japanese, grout output
#   "さざ波" -> \343\201\225\343\201\226\346\263\242
jstr='\343\201\225\343\201\226\346\263\242'
echo "checking 'groff -Kutf8 -Tdvi -Z'" >&2
printf ".ft JPM\n$jstr\n" | "$groff" -Kutf8 -Tdvi -Z | tr '\n' ';' \
  | grep -q \
    ';C *u3055;h *8000;C *u3055_3099;h *8000;C *u6CE2;h *8000;' \
  || wail

# test for upTeX dvi with Japanese, DVI output
echo "checking 'groff -Kutf8 -Tdvi'" >&2
# 2 spaces before each '*'
printf ".ft JPM\n$jstr" | "$groff" -Kutf8 -Tdvi | od -tx1 \
  | grep -q '81  *30  *55  *81  *30  *56  *81  *6c  *e2' || wail

test -z "$fail"

# vim:set autoindent expandtab shiftwidth=2 tabstop=2 textwidth=72:
