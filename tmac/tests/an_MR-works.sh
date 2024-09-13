#!/bin/sh
#
# Copyright (C) 2021 Free Software Foundation, Inc.
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

# Keep preconv from being run.
unset GROFF_ENCODING

fail=

wail () {
    echo ...FAILED >&2
    fail=yes
}

input='.TH foo 1 2021-10-06 "groff test suite"
.SH Name
foo \\- a command with a very short name
.SH Description
The real work is done by
.MR bar 1 .'

output=$(echo "$input" | "$groff" -Tascii -rU1 -man -Z | nl)
echo "$output"

# Expected:
#      1  x T ascii
#      2  x res 240 24 40
#      3  x init
#      4  p1
#      5  x font 2 I
#      6  f2
#      7  s10
#      8  V40
#      9  H0
#     10  md
#     11  DFd
#     12  tfoo
#     13  x font 1 R
#     14  f1
#     15  t(1)
#     16  h552
#     17  tGeneral
#     18  wh24
#     19  tCommands
#     20  wh24
#     21  tManual
#     22  f2
#     23  h528
#     24  tfoo
#     25  f1
#     26  t(1)
#     27  n40 0
#     28  V120
#     29  H0
#     30  x X devtag:.NH 1
#     31  x font 3 B
#     32  f3
#     33  V120
#     34  H0
#     35  tName
#     36  wh24
#     37  x X devtag:.eo.h
#     38  V120
#     39  H120
#     40  n40 0
#     41  f1
#     42  V160
#     43  H120
#     44  tfoo
#     45  wh24
#     46  C\-
#     47  wh48
#     48  ta
#     49  wh24
#     50  tcommand
#     51  wh24
#     52  twith
#     53  wh24
#     54  ta
#     55  wh24
#     56  tvery
#     57  wh24
#     58  tshort
#     59  wh24
#     60  tname
#     61  n40 0
#     62  V240
#     63  H0
#     64  x X devtag:.NH 1
#     65  f3
#     66  V240
#     67  H0
#     68  tDescription
#     69  wh24
#     70  x X devtag:.eo.h
#     71  V240
#     72  H288
#     73  n40 0
#     74  f1
#     75  V280
#     76  H120
#     77  tThe
#     78  wh24
#     79  treal
#     80  wh24
#     81  twork
#     82  wh24
#     83  tis
#     84  wh24
#     85  tdone
#     86  wh24
#     87  tby
#     88  wh24
#     89  x X tty: link man:bar(1)
#     90  f2
#     91  V280
#     92  H720
#     93  tbar
#     94  f1
#     95  t(1)
#     96  x X tty: link
#     97  V280
#     98  H864
#     99  t.
#    100  n40 0
#    101  V360
#    102  H0
#    103  tgroff
#    104  wh24
#    105  ttest
#    106  wh24
#    107  tsuite
#    108  h456
#    109  t2021-10-06
#    110  f2
#    111  h696
#    112  tfoo
#    113  f1
#    114  t(1)
#    115  n40 0
#    116  x trailer
#    117  V360
#    118  x stop

echo "checking for opening 'link' device extension command" >&2
echo "$output" | grep -Eq '89[[:space:]]+x X tty: link man:bar\(1\)$' \
    || wail

echo "checking for correct man page title font style" >&2
echo "$output" | grep -Eq '90[[:space:]]+f2' \
    || wail
echo "$output" | grep -Eq '93[[:space:]]+tbar' \
    || wail

echo "checking for correct man page section font style" >&2
echo "$output" | grep -Eq '94[[:space:]]+f1' \
    || wail
echo "$output" | grep -Eq '95[[:space:]]+t\(1\)' \
    || wail

echo "checking for closing 'link' device extension command" >&2
echo "$output" | grep -Eq '96[[:space:]]+x X tty: link$' \
    || wail

output=$(echo "$input" | "$groff" -man -Thtml)
echo "$output"

echo "checking for correctly formatted man URI in HTML output" >&2
echo "$output" | grep -Fq '<a href="man:bar(1)"><i>bar</i>(1)</a>.' \
    || wail

test -z "$fail"

# vim:set ai et sw=4 ts=4 tw=72:
