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
    echo FAILED >&2
    fail=YES
}

# Regression-test Savannah #65865 and other things that could go wrong.

input='.TL C123 F456
A World-Shaking Breakthrough
.AF "Yoyodyne Systems, Inc."
.AU "Art Vandelay" axv RDU cube2 Foo x99 Lab baz
.AU "H.\& E.\& Pennypacker" hep SFO cube3 Bar x77 Lab qux
.TM 78-9-ABC 98-7-DEF
.ND "June 10, 2024"
.AS
Our successful leverage of core competencies to achieve economies of
scale has transformed our entire sector of industry with exciting new
synergies in allocating more money to (already rich) people.
.AE
.MT'

output=$(echo "$input" | "$groff" -mm -Tascii -P-cbou)
echo "$output"

echo "checking for charging case number" >&2
echo "$output" | grep -q C123 || wail

echo "checking for filing case number" >&2
echo "$output" | grep -q F456 || wail

echo "checking for title" >&2
echo "$output" | grep -q Breakthrough || wail

echo "checking for affiliated firm" >&2
echo "$output" | grep -q Yoyodyne || wail

echo "checking for first author" >&2
echo "$output" | grep -q 'Art Vandelay' || wail

echo "checking for second author" >&2
echo "$output" | grep -Fq 'H. E. Pennypacker' || wail

echo "checking for first technical memorandum number" >&2
echo "$output" | grep -q '78-9-ABC' || wail

echo "checking for second technical memorandum number" >&2
echo "$output" | grep -q '98-7-DEF' || wail

echo "checking for date" >&2
echo "$output" | grep -q 'June 10, 2024' || wail

echo "checking for abstract content" >&2
echo "$output" | grep -q leverage || wail

echo "checking for memorandum type notation" >&2
echo "$output" | grep -iq 'technical memorandum' || wail

test -z "$fail"

# vim:set ai et sw=4 ts=4 tw=72:
