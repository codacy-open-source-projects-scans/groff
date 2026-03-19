#!/bin/sh
#
# Copyright 2026 G. Branden Robinson
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

eqn="${abs_top_builddir:-.}/eqn"
neqn="${abs_top_builddir:-.}/neqn"

# Test our test harness; the `neqn` script should find an `eqn`
# executable of the same version.

GROFF_BIN_PATH="$abs_top_builddir"
export GROFF_BIN_PATH

eqn_version=$("$eqn" --version | head -n 1)
neqn_version=$("$neqn" --version | head -n 1)

echo "$neqn_version"
echo "$eqn_version"

test "$neqn_version" = "$eqn_version"

# vim:set autoindent expandtab shiftwidth=4 tabstop=4 textwidth=72:
