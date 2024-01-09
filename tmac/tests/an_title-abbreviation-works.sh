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

# Regression-test Savannah #43532.
#
# Excessively long man page titles can overrun other parts of the titles
# (headers and footers).  Verify abbreviation of ones that would.

fail=

wail () {
    echo "...FAILED" >&2
    fail=YES
}

input='.TH foo 1 2021-05-31 "groff test suite"
.SH Name
foo \- a command with a very short name'

output=$(echo "$input" | "$groff" -Tascii -P-cbou -man)
echo "$output"

echo "checking that short man page title is set acceptably" >&2
echo "$output" \
    | grep -Eq 'foo\(1\) +General Commands Manual +foo\(1\)' || wail

input='.TH CosNotifyChannelAdmin_StructuredProxyPushSupplier 3erl \
2021-05-31 "groff test suite" "Erlang Module Definition"
.SH Name
CosNotifyChannelAdmin_StructuredProxyPushSupplier \- OMFG'

output=$(echo "$input" | "$groff" -Tascii -P-cbou -man)
echo "$output"

echo "checking that ultra-long man page title is abbreviated" >&2
title_abbv="CosNotif...hSupplier(3erl)"
# 2 spaces each before "Erlang" and after "Definition"
pattern="$title_abbv  Erlang Module Definition  $title_abbv"
echo "$output" | grep -Fq "$pattern" || wail

test -z "$fail"

# vim:set ai et sw=4 ts=4 tw=72:
