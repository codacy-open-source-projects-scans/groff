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

# Unit-test `pnr` request.
#
# Check a writable register, a read-only register, and a string-valued
# (read-only) register.
#
# The ordering of registers dumped by `pnr` when given no arguments is
# not deterministic.
#
# nl      -1 +0 0
# .fam    T
# .R      2147483647

echo "checking that 'pnr' works without arguments" >&2
output=$(echo .pnr | "$groff" 2>&1)
echo "$output" | grep -Eqx 'nl[[:space:]]-1[[:space:]]\+0[[:space:]]0' \
    || fail=yes
echo "$output" | grep -Eqx '\.fam[[:space:]]T'        || fail=yes
echo "$output" | grep -Eqx '\.R[[:space:]]2147483647' || fail=yes
test -z "$fail" || wail

echo "checking that 'pnr' works with arguments" >&2
output=$(echo .pnr .hla c. | "$groff" 2>&1)
output=$(echo $output) # condense onto one line, convert tab -> space
echo "$output" | grep -Eq '\.hla en c\. 1 \+0 0' || wail

test -z "$fail"

# vim:set autoindent expandtab shiftwidth=4 tabstop=4 textwidth=72:
