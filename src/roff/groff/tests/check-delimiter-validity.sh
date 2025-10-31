#!/bin/sh
#
# Copyright (C) 2024-2025 Free Software Foundation, Inc.
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

for c in A B C D E F G H I J K L M N O P Q R S T U V W X Y Z \
         a b c d e f g h i j k l m n o p q r s t u v w x y z
do
  echo "checking validity of '$c' as delimiter in normal mode" \
    >&2
  output=$(printf '\\l%c1n+2n\\&_%c\n' "$c" "$c" \
    | "$groff" -T ascii | sed '/^$/d')
  echo "$output"
  echo "$output" | grep -Fqx ___ || wail
done

for c in 0 1 2 3 4 5 6 7 8 9 + - '(' . '|'
do
  echo "checking invalidity of '$c' as delimiter in normal mode" \
    >&2
  output=$(printf '\\l%c1n+2n\\&_%c\n' "$c" "$c" \
    | "$groff" -T ascii | sed '/^$/d')
  echo "$output"
  echo "$output" | grep -qx 1n+2n_. || wail
done

# All of these work in DWB nroff.
for c in A B C D E F G H I J K L M N O P Q R S T U V W X Y Z \
         a b c d e f g h i j k l m n o p q r s t u v w x y z \
         0 1 2 3 4 5 6 7 8 9 + - / '*' % '<' '>' = '&' : '(' ')' . '|'
do
  echo "checking validity of '$c' as delimiter in compatibility mode" \
    >&2
  output=$(printf '\\l%c1n+2n\\&_%c\n' "$c" "$c" \
    | "$groff" -C -T ascii | sed '/^$/d')
  echo "$output"
  echo "$output" | grep -Fqx ___ || wail
done

test -z "$fail"

# vim:set autoindent expandtab shiftwidth=2 tabstop=2 textwidth=72:
