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

soelim="${abs_top_builddir:-.}/soelim"

fail=

wail () {
  echo "...FAILED" >&2
  fail=yes
}

tmpfile="bar baz.groff"

cleanup () {
  rm -f "$tmpfile"
}

# A process handling a fatal signal should:
#   1.  Mask _all_ fatal signals.
#   2.  Perform cleanup operations.
#   3.  Unmask the signal (removing the handler).
#   4.  Signal its own process group with the signal caught so that the
#       the children exit and shell accurately reports how the process
#       died.
fatals="HUP INT QUIT TERM"
for s in $fatals
do
  trap "trap '' $fatals; cleanup; trap - $fatals; kill -$s -$$" $s
done

printf 'bar\nbaz\n' > "$tmpfile"

input='.
foo
.so bar baz.groff
qux
.'

output=$(echo "$input" | "$soelim" -r | tr '\n' ' ')
echo "$output"
# Note trailing space after second dot.
echo "$output" | grep -Fqx ". foo bar baz qux . " || wail

cleanup
test -z "$fail"

# vim:set autoindent expandtab shiftwidth=2 tabstop=2 textwidth=72:
