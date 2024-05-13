#!/bin/sh
#
# Copyright (C) 2023-2024 Free Software Foundation, Inc.
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

input='.TH foo 1 2023-05-13 "groff test suite"
.SH Name
foo \\- a command with a very short name
.
.
.SH Synopsis
.
This pre-synopsis text is not often used in practice.
.
.
.P
.SY foo
.I "operand1 operand2 operand3 operand4 operand5 operand6 operand6"
.I "operand7 operand8 operand9"
.YS
.
.
.P
.SY foo
.B \\-h
.YS
.
.SY foo
.B \\-\\-help
.YS
.
This post-synopsis text is not often used in practice.
.
.
.SH Description
The real work is done by
.MR bar 1 .'

output=$(echo "$input" | "$groff" -man -T ascii -P -cbou)
echo "$output"

echo 'checking for 1v of space before non-nested SY request' >&2
echo "$output" \
    | sed -n -e '/pre-synopsis/{' \
        -e 'n;/^$/{' \
        -e 'n;/foo.*operand/p' \
        -e '}' \
        -e '}' \
    | grep -q . || wail

# 9 spaces in the spaceful sed expression below
echo 'checking for correct indentation of broken synopsis lines' >&2
echo "$output" \
    | sed -n -e '/foo operand1/{' \
        -e 'n;/         operand8.*/p' \
        -e '}' \
    | grep -q . || wail

echo 'checking for lack of space before nested SY request' >&2
echo "$output" \
    | sed -n -e '/foo -h$/{' \
        -e 'n;/foo --help$/p' \
        -e '}' \
    | grep -q . || wail

echo 'checking for lack of space after YS request' >&2
echo "$output" \
    | sed -n -e '/foo --help$/{' \
        -e 'n;/post-synopsis/p' \
        -e '}' \
    | grep -q . || wail

# Thanks to Alex Colomar for the meat of the following test case.

input2='.TH foo 3 2024-05-06 "groff test suite"
.SH Name
foo \\- a small library for converting strings to integers
.
.
.SH Synopsis
.
.B int
.SY a2i (
.B TYPE,
.BI TYPE\~*restrict\~ n ,
.BI const\~char\~* s ,
.BI char\~**_Nullable\~restrict\~ endp ,
.BI int\~ base ,
.BI TYPE\~ min ,
.BI TYPE\~ max );
.YS .
.
.B int
.SY a2s (
.B TYPE,
.BI TYPE\~*restrict\~ n ,
.BI const\~char\~* s ,
.BI char\~**_Nullable\~restrict\~ endp ,
.BI int\~ base ,
.BI TYPE\~ min ,
.BI TYPE\~ max );
.YS .
.
.B int
.SY a2u (
.B TYPE,
.BI TYPE\~*restrict\~ n ,
.BI const\~char\~* s ,
.BI char\~**_Nullable\~restrict\~ endp ,
.BI int\~ base ,
.BI TYPE\~ min ,
.BI TYPE\~ max );
.YS'

output2=$(echo "$input2" | "$groff" -rLL=80n -man -T ascii -P -cbou)
echo "$output2"

echo 'checking for automatic hyphenation disablement inside synopsis' \
    >&2
echo "$output2" | grep -q 're-$' && wail

test -z "$fail"

# vim:set ai et sw=4 ts=4 tw=72:
