#!/bin/sh
#
# Copyright (C) 2020-2025 Free Software Foundation, Inc.
#
# This file is part of groff.
#
# groff is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your
# option) any later version.
#
# groff is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.
#

preconv="${abs_top_builddir:-.}/preconv"

fail=

wail () {
    echo ...FAILED >&2
    fail=YES
}

# Ensure a predictable character encoding.
export LC_ALL=C

echo "testing -e flag override of BOM detection" >&2
printf '\376\377\0\100\0\n' \
    | "$preconv" -d -e euc-kr 2>&1 > /dev/null \
    | grep -q "no search for coding tag" || wail

echo "testing detection of UTF-32BE BOM" >&2
printf '\0\0\376\377\0\0\0\100\0\0\0\n' \
    | "$preconv" -d 2>&1 > /dev/null \
    | grep -q "found BOM" || wail

echo "testing detection of UTF-32LE BOM" >&2
printf '\377\376\0\0\100\0\0\0\n\0\0\0' \
    | "$preconv" -d 2>&1 > /dev/null \
    | grep -q "found BOM" || wail

echo "testing detection of UTF-16BE BOM" >&2
printf '\376\377\0\100\0\n' \
    | "$preconv" -d 2>&1 > /dev/null \
    | grep -q "found BOM" || wail

echo "testing detection of UTF-16LE BOM" >&2
printf '\377\376\100\0\n\0' \
    | "$preconv" -d 2>&1 > /dev/null \
    | grep -q "found BOM" || wail

echo "testing detection of UTF-8 BOM" >&2
printf '\357\273\277@\n' \
    | "$preconv" -d 2>&1 > /dev/null \
    | grep -q "found BOM" || wail

# We do not find a coding tag on piped input because it isn't seekable.
echo "testing detection of Emacs coding tag in piped input" >&2
printf '.\\" -*- coding: euc-kr; -*-\\n' \
    | "$preconv" -d 2>&1 >/dev/null \
    | grep -q "no coding tag" || wail

test -z "$fail" || exit

# We need uchardet to work to get past this point.
if ! "$preconv" -v | grep -q 'with uchardet support'
then
    echo "$0: preconv lacks uchardet support; skipping" >&2
    exit 77 # skip
fi

# Instead of using temporary files, which in all fastidiousness means
# cleaning them up even if we're interrupted, which in turn means
# setting up signal handlers, we use files in the build tree.

#doc=contrib/mm/groff_mmse.7
#echo "testing uchardet detection on UTF-8 document $doc" >&2
#"$preconv" -d -D us-ascii 2>&1 >/dev/null $doc \
#    | grep -q 'charset: UTF-8' || wail

# uchardet can't seek on a pipe either.
echo "testing uchardet detection on pipe (expect fallback to -D)" >&2
printf 'Eat at the caf\351.\n' \
    | "$preconv" -d -D euc-kr 2>&1 > /dev/null \
    | grep -q "encoding used: 'EUC-KR'" || wail

test -z "$fail" || exit

has_glibc=

if command -v locale > /dev/null
then
    has_glibc=yes
fi

# Fall back to the locale.
#
# On glibc systems, the 'C' locale uses "ANSI_X3.4-1968" for the
# character set, but preconv assumes Latin-1 instead of US-ASCII.
#
# On non-glibc systems, who knows?  But at least some use UTF-8.

if [ -n "$has_glibc" ]
then
    charset=ISO-8859-1
else
    charset=UTF-8
fi

echo "testing fallback to locale setting in environment" >&2
printf 'Eat at the caf\351.\n' \
    | "$preconv" -d 2>&1 > /dev/null \
    | grep -q "encoding used: '$charset'" || wail

test -z "$fail"

# vim:set autoindent expandtab shiftwidth=4 tabstop=4 textwidth=72:
