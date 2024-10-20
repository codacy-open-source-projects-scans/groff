#!/bin/sh
#
# Copyright (C) 2019-2024 Free Software Foundation, Inc.
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

groff="${abs_top_builddir:-.}/test-groff"

fail=

wail() {
    echo ...FAILED >&2
    fail=yes
}

input='.
.TH sample 1 2020-10-31 "groff test suite"
.SH Name
sample \- test subject for groff
.'

output=$(printf "%s" "$input" | "$groff" -Tascii -P -cbou -man)
echo "$output"

echo "testing package default 'CS' register setting" >&2
echo "$output" | grep -q Name || fail

output=$(printf "%s" "$input" | "$groff" -Tascii -P -cbou -rCS=0 -man)
echo "$output"

echo "testing '-rCS=0' argument " >&2
echo "$output" | grep -q Name || fail

output=$(printf "%s" "$input" | "$groff" -Tascii -P -cbou -rCS=1 -man)
echo "$output"

echo "testing '-rCS=1' argument " >&2
echo "$output" | grep -q NAME || fail

test -z "$fail"

# vim:set ai et sw=4 ts=4 tw=72:
