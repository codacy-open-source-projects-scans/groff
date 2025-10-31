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

# Regression-test Savannah #64484.
#
# The `device` request should not alter any escape sequences except
# those encoding special characters.

output=$(printf \
  '.device ps:exec A B\\~C\\ D\\0E\\|F\\^G\\h".5m"H\\v".5m"I\002\n' \
  | "$groff" -T ps -Zww | grep '^x X ')
# Use printf instead of echo; the latter might interpret `\v`.
printf '%s\n' "$output"
printf '%s\n' "$output" \
  | grep -Fqx 'x X ps:exec A B\~C\ D\0E\|F\^G\h".5m"H\v".5m"I'

# vim:set autoindent expandtab shiftwidth=2 tabstop=2 textwidth=72:
