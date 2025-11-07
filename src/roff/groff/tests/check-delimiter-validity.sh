#!/bin/sh
#
# Copyright 2024-2025 G. Branden Robinson
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

groff="${abs_top_builddir:-.}/test-groff"

fail=

wail () {
  echo ...FAILED >&2
  fail=YES
}

# not tested: '_' (because it's part of our delimited expression)
for c in A B C D E F G H I J K L M N O P Q R S T U V W X Y Z \
         a b c d e f g h i j k l m n o p q r s t u v w x y z \
         '!' '"' '#' '$' "'" ',' ';' '?' \
         '@' '[' ']' '^' '`' '{' '}' '~'
do
    echo "checking validity of '$c' as delimiter in normal mode" \
         >&2
    output=$(printf '\\l%c1n+2n\\&_%c\n' "$c" "$c" \
      | "$groff" -w delim -T ascii | sed '/^$/d')
    echo "$output" | grep -Fqx ___ || wail
done

for octal in 001 002 003 004 005 006 007 010 011 014 177
do
    echo "checking validity of control character $octal (octal)" \
         "as delimiter in normal mode" >&2
    output=$(printf '\\l\'$octal'1n+2n\&_\'$octal'\n' \
      | "$groff" -w delim -T ascii | sed '/^$/d')
    echo "$output" | grep -Fqx ___ || wail
done

for c in 0 1 2 3 4 5 6 7 8 9 + - '(' . '|'
do
    echo "checking invalidity of '$c' as delimiter in normal mode" \
         >&2
    output=$(printf '\\l%c1n+2n\\&_%c\n' "$c" "$c" \
      | "$groff" -w delim -T ascii | sed '/^$/d')
    echo "$output" | grep -qx 1n+2n_. || wail
done

# Now test the context-dependent sets of delimiters of AT&T troff.

# not tested: '_' (because it's part of our delimited expression)
for c in A B C D E F G H I J K L M N O P Q R S T U V W X Y Z \
         a b c d e f g h i j k l m n o p q r s t u v w x y z \
         0 1 2 3 4 5 6 7 8 9 + - / '*' % '<' '>' = '&' : '(' ')' . '|' \
         '!' '"' '#' '$' "'" ',' ';' '?' \
         '@' '[' ']' '^' '`' '{' '}' '~'
do
    echo "checking validity of '$c' as string expression delimiter" \
         "in compatibility mode" >&2
    output=$(printf '\\o%c__%c__\n' "$c" "$c" \
      | "$groff" -C -w delim -T ascii -P -cbou | sed '/^$/d')
    echo "$output" | grep -Fqx ___ || wail
done

for octal in 002 003 005 006 007 177
do
    echo "checking validity of control character $octal (octal)" \
         "as string expression delimiter in compatibility mode" >&2
    output=$(printf '\\o\'$octal'__\'$octal'__\n' \
      | "$groff" -C -w delim -T ascii | sed '/^$/d')
    echo "$output" | grep -qx _.___ || wail
done

for c in A B C D E F G H I J K L M N O P Q R S T U V W X Y Z \
         a b c d e f g h i j k l m n o p q r s t u v w x y z \
         0 1 2 3 4 5 6 7 8 9 . '|' \
         '!' '"' '#' '$' "'" ',' ';' '?' \
         '@' '[' ']' '^' '`' '{' '}' '~'
do
    echo "checking validity of '$c' as numeric expression delimiter" \
         "in compatibility mode" >&2
    output=$(printf '_\\h%c1n+2n%c_\n' "$c" "$c" \
      | "$groff" -C -w delim -T ascii | sed '/^$/d')
    echo "$output" | grep -Fqx '_   _' || wail
done

for c in + - / '*' % '<' '>' = '&' : '(' ')'
do
    echo "checking invalidity of '$c' as numeric expression delimiter" \
         "in compatibility mode" >&2
    output=$(printf '_\\h%c1n+2n%c_\n' "$c" "$c" \
      | "$groff" -C -w delim -T ascii -P -cbou | sed '/^$/d')
    echo "$output" | grep -Fqx '_   _' && wail
done

for octal in 002 003 005 006 007 177
do
    echo "checking validity of control character $octal (octal)" \
         "as numeric expression delimiter in compatibility mode" >&2
    output=$(printf '\\o\'$octal'__\'$octal'__\n' \
      | "$groff" -C -w delim -T ascii | sed '/^$/d')
    echo "$output" | grep -qx _.___ || wail
done

# 'v' as a conditional expression operator is a vtroff extension, not a
# GNU one.  vtroff is also unobtainium in the 21st century.  DWB 3.3,
# Plan 9, and Solaris 10 troffs don't treat 'v' specially.
#
# TODO: Ideally, we should permit only AT&T-compatible (plus 'v'?)
# operators when in compatibility mode.  (If you want the extensions,
# use the `do` request.)  But that will demand hitting
# `is_conditional_expression_true()` in "input.cpp" with a hammer, and
# announcement of a behavior change in the "NEWS" file.
#
# If/when we do, change the following loop as shown.  And decide whether
# to treat 'v' as an AT&Tism or a GNUism.
#for c in A B C D E F G H I J K L M N O P Q R S T U V W X Y Z \
#         a b c d   f g h i j k l m     p q r s   u v w x y z
# not tested: '!' (because it's a conditional expression operator)
# not tested: '@' (because it's part of our delimited expression)
for c in A B C D E   G H I J K L M N O P Q R   T U V W X Y Z \
         a b       f g h i j k l       p q   s   u   w x y z \
             '"' '#' '$' "'" ',' ';' '?' \
             '[' ']' '^' '`' '{' '}' '~'
do
    echo "checking validity of '$c' as output comparison delimiter" \
         "in compatibility mode" >&2
    output=$(printf '.if %c@@@%c@@@%c ___\n' "$c" "$c" "$c" \
      | "$groff" -C -w delim -T ascii | sed '/^$/d')
    echo "$output" | grep -Fqx ___ || wail
done

for octal in 002 003 005 006 007 177
do
    echo "checking validity of control character $octal (octal)" \
         "as output comparison delimiter in compatibility mode" >&2
    output=$(printf '.if \'$octal'@@@\'$octal'@@@\'$octal' ___\n' \
      | "$groff" -C -w delim -T ascii -P -cbou | sed '/^$/d')
    echo "$output" | grep -Fqx ___ || wail
done

for c in e n o t \
         0 1 2 3 4 5 6 7 8 9 + - / '*' % '<' '>' = '&' : '(' ')' . '|' \
         '!'
do
  echo "checking invalidity of '$c' as output comparison delimiter" \
    "in compatibility mode" >&2
  output=$(printf '.if %c@@@%c@@@%c ___\n' "$c" "$c" "$c" \
    | "$groff" -C -w delim -T ascii -P -cbou | sed '/^$/d')
  echo "$output" | grep -Fqx ___ && wail
done

test -z "$fail"

# vim:set autoindent expandtab shiftwidth=4 tabstop=4 textwidth=72:
