#!/bin/sh
#
# Copyright (C) 2025 Free Software Foundation, Inc.
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
#

soelim="${abs_top_builddir:-.}/soelim"

# soelim should not strip "invalid" characters from parts of a document
# it does not interpret.

output=$(printf '.\\" degree sign: \313\232\n' | "$soelim")
printf "%s\n" "$output"
printf "%s\n" "$output" | od -c | grep -q ' 232'

# vim:set ai et sw=4 ts=4 tw=72:
