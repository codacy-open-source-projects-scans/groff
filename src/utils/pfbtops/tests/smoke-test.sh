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

pfbtops="${abs_top_builddir:-.}/pfbtops"
pfbfont="${abs_top_builddir:-.}/font/devpdf/symbolsl.pfb"

# Smoke-test pfbtops command.

output=$("$pfbtops" < "$pfbfont")
if [ $? -ne 0 ]
then
    echo "$pfbtops" exited with status $?
    exit 1
fi

printf "%s\n" "$output"
printf "%s\n" "$output" | grep -Fqx cleartomark

# vim:set ai et sw=4 ts=4 tw=72:
