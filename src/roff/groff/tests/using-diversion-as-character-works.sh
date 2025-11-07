#!/bin/sh
#
# Copyright 2024 G. Branden Robinson
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

# Regression-test Savannah #66587.  Based on a reproducer by Peter
# Schaffter.

input='.
.sp 6P-1v
.
.nr img-w 25p
.nr img-d 22p
.
.di img-div
.ev img-div
.nf
\X@pdf: pdfpic artifacts/small-gnu-head.png@
.ev
.di
.
.char \[img] \*[img-div]
.ds gnu \v@-\n[img-d]u@\[img]\h@\n[img-w]u@
.
.nop A GNU head \*[gnu] image.
.'

output=$(printf "%s\n" "$input" | "$groff" -T pdf -Z)
echo "$output"

# We should observe a horizontal motion after the word "head" and before
# the device extension command.
stream=$(echo "$output" | tr '\n' ' ')
echo "$stream" | grep -Eq 't *head +w *h *2500 +V *62000 +x * X *pdf:'

# vim:set autoindent expandtab shiftwidth=2 tabstop=2 textwidth=72:
