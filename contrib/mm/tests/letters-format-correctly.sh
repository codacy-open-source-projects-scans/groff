#!/bin/sh
#
# Copyright (C) 2024 Free Software Foundation, Inc.
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

groff="${abs_top_builddir:-.}/test-groff"

fail=

wail () {
    echo FAILED >&2
    fail=YES
}

# Locate directory containing our examples.
examples_dir=

for buildroot in . .. ../..
do
    e=$buildroot/contrib/mm/examples
    if [ -d "$e" ]
    then
        examples_dir=$e
        break
    fi
done

# If we can't find it, we can't test.
test -z "$examples_dir" && exit 77 # skip

# Locate directory containing our test artifacts.
artifacts_dir=

for buildroot in . .. ../..
do
    a=$buildroot/contrib/mm/tests/artifacts
    if [ -d "$a" ]
    then
        artifacts_dir=$a
        break
    fi
done

# If we can't find it, we can't test.
test -z "$artifacts_dir" && exit 77 # skip

input="$examples_dir"/letter.mm

for t in BL SB FB SP
do
    echo "checking formatting of LT type '$t'" >&2
    expected=$(cksum "$artifacts_dir"/letter.$t | cut -d' ' -f1-2)
    actual=$("$groff" -ww -mm -dlT=$t -Tascii -P-cbou "$input" | cksum \
        | cut -d' ' -f1-2)
    test "$actual" = "$expected" || wail
done

test -z "$fail"

# vim:set ai et sw=4 ts=4 tw=72:
