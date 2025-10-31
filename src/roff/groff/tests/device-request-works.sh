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

fail=

wail () {
  echo ...FAILED >&2
  fail=YES
}

input_plain='.device ps: nop'

input_quoted='.device " ps: nop'

# We need to portably get a backslash into the string, and we can't
# change the escape character because despite appearances, this notation
# isn't a groff escape sequence: it's passed to the output device.
#
# Omit the trailing newline; it is supplied below.
#
# 8 backslashes
input_special=$(printf ".device pdf: \\\\\\\\[u007E]\n.br")

for device in ascii dvi html xhtml latin1 lbp lj4 pdf ps utf8 \
              X75 X75-12 X100 X100-12
do
  echo "checking 'device' request basic operation on $device device" >&2
  output=$(printf "%s\n" "$input_plain" | "$groff" -T $device -Z) \
    || wail
  echo "$output"
  echo "$output" | grep -Eq 'x * X *ps: nop' || wail

  echo "checking quoteful 'device' operation on $device device" >&2
  output=$(printf "%s\n" "$input_quoted" | "$groff" -T $device -Z) \
    || wail
  echo "$output"
  # 2 spaces before '*ps:'
  echo "$output" | grep -Eq 'x * X  *ps: nop' || wail

  echo "checking that Unicode escape sequence is preserved on $device" \
       "device" >&2
  output=$(printf "%s\n" "$input_special" | "$groff" -T $device -Z) \
    || wail
  echo "$output"
  # 3 backslashes
  echo "$output" | grep -Eq 'x * X *pdf: \\\[u007E]' || wail
done

test -z "$fail"

# vim:set autoindent expandtab shiftwidth=2 tabstop=2 textwidth=72:
