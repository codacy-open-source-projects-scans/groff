#!/bin/sh
#
# Copyright (C) 2025 Free Software Foundation, Inc.
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

# Ensure that "structural" macros work tolerably even in ill-formed
# documents.

input='.
.P
Paragraph.
.'

output=$(printf "%s\n" "$input" | "$groff" -man -Tascii -P -cbou)
echo "$output"
echo "checking P macro" >&2
echo "$output" | grep -Fq 'Paragraph.' || wail

input='.
.SH "Section heading"
.'

output=$(printf "%s\n" "$input" | "$groff" -man -Tascii -P -cbou)
echo "$output"
echo "checking SH macro" >&2
echo "$output" | grep -q 'Section heading' || wail

input='.
.SS "Subection heading"
.'

output=$(printf "%s\n" "$input" | "$groff" -man -Tascii -P -cbou)
echo "$output"
echo "checking SS macro" >&2
echo "$output" | grep -q 'Subection heading' || wail

test -z "$fail"

# vim:set ai et sw=4 ts=4 tw=72:
