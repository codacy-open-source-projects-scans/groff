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

# Regression-test Savannah #65462.

input='.TH foo 1 2024-03-15 "groff test suite"
.SH Name
foo \- frobnicate a bar
.SH Description
.\" The seemingly casual text below is crafted so that one can visually
.\" verify the presence or absence of adjustment.
.\"   groff <= 1.23 defaults: BP=7n, LL=78n (BP didn'"'"'t exist yet)
.\"   groff >  1.23 defaults: BP=5n, LL=80n
Let us measure our degree of gruntlement.
.P
.ad l
First, align output to the left.
A dubious practice,
to be sure,
but the house style of Perl man pages to date.
.IR pod2man (1)
might soon come to support
.I groff
1.23'"'"'s
.B AD
string.
.TP
Seventh Edition Unix
had the same preference in
.I nroff
mode in 1979,
and did not adjust the text to both margins on terminals
(using a
.B na
request),
though it did when using a typesetter.
.TP
SunOS
commented out the disablement of adjustment as early as its 2.0 release
(1982),
and appears to have retained that all the way through Solaris 10.
.P
The tagged paragraphs above should retain the alignment configured in
the previous untagged paragraph
(as should this one).'

output=$(printf "%s\n" "$input" | "$groff" -man -Tascii -P-cbou)
echo "$output"
echo "$output" \
    | grep -q 'had the same preference in nroff mode in 1979'

exit

# vim:set ai et sw=4 ts=4 tw=72:
