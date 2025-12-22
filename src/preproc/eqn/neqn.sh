#!/bin/sh
#
# Copyright 2014-2024 Free Software Foundation, Inc.
#
# This file is part of groff, the GNU roff typesetting system.
#
# groff is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free
# Software Foundation, either version 2 of the License (GPL2).
#
# groff is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# The GPL2 license text is available in the internet at
# <http://www.gnu.org/licenses/gpl-2.0.txt>.

# Provision of this shell script should not be taken to imply that use
# of GNU eqn with groff -Tascii|-Tlatin1|-Tutf8 is supported.

# Screen for shells non-conforming with POSIX Issue 4 (1994).
badshell=yes
# Solaris 10 /bin/sh is so wretched that it not only doesn't support
# standard parameter expansion, but it also writes diagnostic messages
# to the standard output instead of standard error.
if [ -n "$SHELL" ]
then
  "$SHELL" -c 'prog=${0##*/}' >/dev/null 2>&1 && badshell=
fi

if [ -n "$badshell" ]
then
  prog=`basename $0`
else
  prog=${0##*/}
fi

@GROFF_BIN_PATH_SETUP@
PATH="$GROFF_RUNTIME$PATH"
export PATH

for arg
do
  case "$arg" in
    -h|--help)
      cat <<EOF
usage: $prog [eqn-option ...] [file ...]
usage: $prog {-h | --help}

$prog invokes eqn(1) with the '-T ascii' option and any other arguments
specified.  See the neqn(1) manual page.
EOF
      exit 0
      ;;
  esac
done

# Note: The construction '${1+"@$"}' preserves the absence of arguments
# in old shells; see "Shell Substitutions" in the GNU Autoconf manual.
# We don't want 'neqn' to become 'neqn ... ""' if $# equals zero.
exec @g@eqn -T ascii ${1+"$@"}

# Local Variables:
# fill-column: 72
# End:
# vim: set autoindent expandtab shiftwidth=2 softtabstop=2 textwidth=72:
