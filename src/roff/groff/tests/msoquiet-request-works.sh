#!/bin/sh
#
# Copyright (C) 2021-2024 Free Software Foundation, Inc.
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

set -e

# Keep preconv from being run.
#
# The "unset" in Solaris /usr/xpg4/bin/sh can actually fail.
if ! unset GROFF_ENCODING
then
    echo "unable to clear environment; skipping" >&2
    exit 77
fi

DOC='.msoquiet nonexistent'

OUTPUT=$(echo "$DOC" | "$groff" -Tascii 2>&1)
echo "$OUTPUT"

echo "testing that .msoquiet of nonexistent file produces no warning" \
  >&2
test -z "$OUTPUT"
