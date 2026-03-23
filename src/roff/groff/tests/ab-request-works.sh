#!/bin/sh
#
# Copyright 2021 G. Branden Robinson
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

groff="${abs_top_builddir:-.}/test-groff"

fail=

wail () {
   echo "...FAILED"
   fail=yes
}

# Verify exit status and regression-test Savannah #60782.
#
# We don't test the X11 devices because groff launches an X client,
# which has to be killed.  Using "-z" to avoid this masks the bug.

# Keep preconv from being run.
#
# The "unset" in Solaris /usr/xpg4/bin/sh can actually fail.
if ! unset GROFF_ENCODING
then
    echo "unable to clear environment; skipping" >&2
    exit 77 # skip
fi

for d in ascii dvi html latin1 lbp lj4 pdf ps utf8
do
    echo "verifying exit status of 'ab' request using $d device" >&2
    printf '.ab\n' | "$groff" -Z -T$d
    # 4 = (1 << 2)
    test $? -eq 4 || wail
done

echo "verifying empty output of 'ab' request with no arguments" >&2
output=$(printf '.ab\n' | "$groff" -Z -Tascii 2>&1)
test "$output" = "" || wail

echo "verifying that arguments to 'ab' request go to stderr" >&2
output=$(printf '.ab foo\n' | "$groff" -Z -Tascii 2>&1 > /dev/null)
test "$output" = "foo" || wail

test -z "$fail"

# vim:set autoindent expandtab shiftwidth=4 tabstop=4 textwidth=72:
