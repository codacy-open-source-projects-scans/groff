#!/bin/sh
#
# Copyright 2026 G. Branden Robinson
#
# This file is part of mm, a reimplementation of the Documenter's
# Workbench (DWB) troff memorandum macro package for use with GNU troff.
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

# Regression-test Savannah #68201.

input='.
.nr Hy 1
.WA
Louis C. «Wheezy» Gaspé-Fourier
Legrande Schème de Québec S.A.
Boîte 1234
2900, boul. Édouard-Montpetit
Montréal QC H3Z QJ4
CANADA
.WE
.ND "31 Mars 2026"
.IA
Chuck Montesquieu, fils
1789 Rue Condé
New Orleans, LA  70116
UNITED STATES
.IE
.LT
.P
Nous avons une fuite dans la R&D\~!
La prochaine personne que je surprendrai à intégrer
des échantillons d\[cq]ingénierie
de nos processeurs Lightspeed Overdrive à 2048 cœurs
dans des sous-verres en liège distribués
lors de salons professionnels va le regretter.
Aucun d\[cq]entre vous n\[cq]est digne de porter
les bottes du maréchal Pétain.
Tabarnac\~!
.FC
.SG
.NS
des imbéciles divers
.NE
.'

output=$(echo "$input" | "$groff" -K utf8 -m m -m fr -T utf8 -P -cbou \
    | sed '/^$/d')
echo "$output"
echo "$output" | od -c
output=$(echo "$output" | od -t x1)
echo "$output"

echo "checking that escape sequences are handled between WA and WE" >&2
# "Édouard"
echo "$output" | grep -q '^0000360.*c3 89 64 6f 75 61 72 64' || wail

echo "checking that escape sequences are handled between IA and IE" >&2
# "ndé"
echo "$output" | grep -q '^0000720 6e 64 c3 a9' || wail

test -z "$fail"

# vim:set autoindent expandtab shiftwidth=4 tabstop=4 textwidth=72:
