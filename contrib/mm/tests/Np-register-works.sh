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
    echo ...FAILED >&2
    fail=YES
}

input='.
.nr Np 1
.H 1 "De Redrum Unnatura"
.P
Sed ut perspiciatis,
unde omnis iste natus error sit voluptatem accusantium doloremque
laudantium,
totam rem aperiam eaque ipsa,
quae ab illo inventore veritatis et quasi architecto beatae vitae dicta
sunt,
explicabo.
.P
Nemo enim ipsam voluptatem,
quia voluptas sit,
aspernatur aut odit aut fugit,
sed quia consequuntur magni dolores eos,
qui ratione voluptatem sequi nesciunt,
neque porro quisquam est,
qui dolorem ipsum,
quia dolor sit amet consectetur adipiscivelit,
sed quia non-numquam eius modi tempora incidunt,
ut labore et dolore magnam aliquam quaerat voluptatem.
.H 2 "Siegesbeckia orientalis"
.P
Quis autem vel eum iure reprehenderit,
qui inea voluptate velit esse,
quam nihil molestiae consequatur,
vel illum,
qui dolorem eum fugiat,
quo voluptas nulla pariatur?
.H 1 [redacted]
.H 1 [redacted]
.H 1 [redacted]
.H 1 [redacted]
.H 1 [redacted]
.H 1 [redacted]
.H 1 [redacted]
.H 1 [redacted]
.H 1 "Malleus Maleficarum"
.P
Ut enim ad minima veniam,
quis nostrum exercitationem ullam corporis suscipitlaboriosam,
nisi ut aliquid ex ea commodi consequatur?
.'

output=$(printf "%s\n" "$input" | "$groff" -mm -Tascii -P-cbou | cat -s)
echo "$output"

# Expected output:
#                                   ‐ 1 ‐
#
#       1.  De Redrum Unnatura
#
#       1.01  Sed  ut  perspiciatis, unde omnis iste natus error sit
#       voluptatem  accusantium  doloremque  laudantium,  totam  rem
#       aperiam  eaque  ipsa,  quae  ab  illo inventore veritatis et
#       quasi architecto beatae vitae dicta sunt, explicabo.
#
#       1.02  Nemo  enim  ipsam  voluptatem,  quia   voluptas   sit,
#       aspernatur  aut  odit aut fugit, sed quia consequuntur magni
#       dolores eos, qui ratione voluptatem  sequi  nesciunt,  neque
#       porro  quisquam  est, qui dolorem ipsum, quia dolor sit amet
#       consectetur adipiscivelit, sed quia  non‐numquam  eius  modi
#       tempora incidunt, ut labore et dolore magnam aliquam quaerat
#       voluptatem.
#
#       1.1  Siegesbeckia orientalis
#
#       1.03  Quis  autem  vel  eum  iure  reprehenderit,  qui  inea
#       voluptate velit esse, quam nihil molestiae consequatur,  vel
#       illum, qui dolorem eum fugiat, quo voluptas nulla pariatur?
#
#       2.  [redacted]
#
#       3.  [redacted]
#
#       4.  [redacted]
#
#       5.  [redacted]
#
#       6.  [redacted]
#
#       7.  [redacted]
#
#       8.  [redacted]
#
#       9.  [redacted]
#
#       10.  Malleus Maleficarum
#
#       10.01  Ut enim ad minima veniam, quis nostrum exercitationem
#       ullam  corporis  suscipitlaboriosam,  nisi  ut aliquid ex ea
#       commodi consequatur?

echo "checking label of first paragraph" >&2
echo "$output" | grep -Eq '1\.01 {2}Sed *ut *perspiciatis' || wail

echo "checking indentation of first paragraph, second line" >&2
echo "$output" | grep -Eq '^ {7}voluptatem *accusantium' || wail

echo "checking label of second paragraph" >&2
echo "$output" | grep -Eq '1\.02 {2}Nemo *enim *ipsam' || wail

echo "checking indentation of second paragraph, second line" >&2
echo "$output" | grep -Eq '^ {7}aspernatur *aut *odit' || wail

echo "checking label of third paragraph" >&2
echo "$output" | grep -Eq '1\.03 {2}Quis *autem *vel' || wail

echo "checking indentation of third paragraph, second line" >&2
echo "$output" | grep -Eq '^ {7}voluptate *velit *esse' || wail

echo "checking label of fourth paragraph" >&2
echo "$output" | grep -Eq '10\.01 {2}Ut *enim *ad *minima' || wail

echo "checking indentation of fourth paragraph, second line" >&2
echo "$output" | grep -Eq '^ {7}ullam *corporis' || wail

test -z "$fail"

# vim:set ai et sw=4 ts=4 tw=72:
