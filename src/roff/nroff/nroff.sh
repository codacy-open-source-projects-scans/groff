#!/bin/sh
#
# Emulate nroff with groff.
#
# Copyright (C) 1992-2024 Free Software Foundation, Inc.
#
# Written by James Clark, Werner Lemberg, and G. Branden Robinson.
#
# This file is part of 'groff'.
#
# 'groff' is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License (GPL) as published
# by the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# 'groff' is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

prog=${0##*/}

T=
Topt=
opts=
dry_run=
is_option_argument_pending=

usage="usage: $prog [-abcCDEhiIkpRStUVzZ] [-d ctext] [-d string=text] \
[-K fallback-encoding] [-m macro-package] [-M macro-directory] \
[-n page-number] [-o page-list] [-P postprocessor-argument] \
[-r cnumeric-expression] [-r register=numeric-expression] \
[-T output-device] [-w warning-category] [-W warning-category] \
[file ...]
usage: $prog {-v | --version}
usage: $prog --help"

summary="
Format documents for terminal devices with groff(1).  See the nroff(1)
manual page."

# Break up option clusters into separate arguments.
newargs=
for arg
do
  thisarg=$arg
  while :
  do
    case $thisarg in
      -[abCEikpRStUvzZ])
        newargs="$newargs $thisarg"
        break
        ;;
      -[abCEikpRStUvzZ]*)
        remainder=${thisarg#-?}
        thisarg=${thisarg%%$remainder}
        newargs="$newargs $thisarg"
        thisarg=-$remainder
        ;;
      *)
        newargs="$newargs $thisarg"
        break
        ;;
    esac
  done
done

set -- $newargs

for arg
do
  if [ -n "$is_option_argument_pending" ]
  then
    is_option_argument_pending=
    opts="$opts $arg"
    shift
    continue
  fi

  # groff(1) options we don't support:
  #
  # -e because of historical clash in meaning.
  # -s "
  # -f because terminal devices don't support font families.
  # -g because terminals don't do graphics.  (Some do, but grotty(1)
  #    does not produce ReGIS or Sixel output.)
  # -G "
  # -j "
  # -p "
  # -l because terminal output is not suitable for a print spooler.
  # -L "
  # -N because we don't support -e.
  # -X because gxditview(1) doesn't support terminal documents (why?).
  case $arg in
    -c)
      opts="$opts $arg -P-c" ;;
    -h)
      opts="$opts -P-h" ;;
    -[eq] | -s*)
      # ignore these options
      ;;
    -[dDIKmMnoPrTwW])
      is_option_argument_pending=yes
      opts="$opts $arg" ;;
    -[abCEikpRStUzZ] | -[dDIKMmrnoPwW]*)
      opts="$opts $arg" ;;
    -T*)
      Topt=$arg ;;
    -u*)
      # -u is for Solaris compatibility and not otherwise documented.
      #
      # Solaris 2.2 through at least Solaris 9 'man' invokes
      # 'nroff -u0 ... | col -x'.  Ignore the -u0, since 'less' and
      # 'more' can use the emboldening info.  But disable SGR, since
      # Solaris 'col' mishandles it.
      opts="$opts -P-c" ;;
    -V)
      dry_run=yes ;;
    -v | --version)
      echo "GNU nroff (groff) version @VERSION@"
      opts="$opts $arg" ;;
    --help)
      echo "$usage"
      echo "$summary"
      exit 0 ;;
    --)
      shift
      break ;;
    -)
      break ;;
    -*)
      echo "$prog: usage error: invalid option '$arg'" >&2
      echo "$usage" >&2
      exit 2 ;;
    *)
      break ;;
  esac
  shift
done

if [ -n "$is_option_argument_pending" ]
then
    echo "$prog: usage error: option '$arg' requires an argument" >&2
    exit 2
fi

# Determine the -T option.  Was a valid one specified?
case "$Topt" in
  -Tascii | -Tlatin1 | -Tutf8)
    T=$Topt ;;
esac

# -T option absent or invalid; try environment.
if [ -z "$T" ]
then
  Tenv=-T$GROFF_TYPESETTER
  case "$Tenv" in
    -Tascii | -Tlatin1 | -Tutf8)
      T=$Tenv ;;
  esac
fi

# Finally, infer a -T option from the locale.  Try 'locale charmap'
# first because it is the most reliable, then look at environment
# variables.
if [ -z "$T" ]
then
  # The separate `exec` is to work around a ~2004 bug in Cygwin sh.exe.
  case "`exec 2>/dev/null ; locale charmap`" in
    UTF-8)
      Tloc=utf8 ;;
    ISO-8859-1 | ISO-8859-15)
      Tloc=latin1 ;;
    *)
      # Some old shells don't support ${FOO:-bar} expansion syntax.  We
      # should switch to it when it is safe to abandon support for them.
      case "${LC_ALL-${LC_CTYPE-${LANG}}}" in
        *.UTF-8)
          Tloc=utf8 ;;
        iso_8859_1 | *.ISO-8859-1 | *.ISO8859-1 | \
        iso_8859_15 | *.ISO-8859-15 | *.ISO8859-15)
          Tloc=latin1 ;;
        *)
          case "$LESSCHARSET" in
            utf-8)
              Tloc=utf8 ;;
            latin1)
              Tloc=latin1 ;;
            *)
              Tloc=ascii ;;
          esac ;;
      esac ;;
  esac
  T=-T$Tloc
fi

# Load nroff-style character definitions too.
opts="-mtty-char$opts"

# Set up the 'GROFF_BIN_PATH' variable to be exported in the current
# 'GROFF_RUNTIME' environment.
@GROFF_BIN_PATH_SETUP@
export GROFF_BIN_PATH

# Let our test harness redirect us.  See LC_ALL comment above.
groff=${GROFF_TEST_GROFF-groff}

# Note 1: It would be nice to apply the DRY ("Don't Repeat Yourself")
# principle here and store the entire command string to be executed into
# a variable, and then either display it or execute it.  For example:
#
#   cmd="PATH=... groff ... $@"
#   ...
#   printf "%s\n" "$cmd"
#   ...
#   eval $cmd
#
# Unfortunately, the shell is a nightmarish hellscape of quoting issues.
# Naïve attempts to solve the problem fail when arguments to nroff
# contain embedded whitespace or shell metacharacters.  The solution
# below works with those, but there is insufficient quoting in -V (dry
# run) mode, such that you can't copy-and-paste the output of 'nroff -V'
# if you pass it a filename like foo"bar (with the embedded quotation
# mark) and expect it to run without further quoting.
#
# If POSIX adopts Bash's ${var@Q} or an equivalent, this issue can be
# revisited.
#
# Note 2: The construction '${1+"@$"}' preserves the absence of
# arguments in old shells; see "Shell Substitutions" in the GNU Autoconf
# manual.  We don't want 'nroff' to become 'groff ... ""' if $# equals
# zero.
if [ -n "$dry_run" ]
then
  echo PATH="$GROFF_RUNTIME$PATH" $groff $T $opts ${1+"$@"}
else
  PATH="$GROFF_RUNTIME$PATH" $groff $T $opts ${1+"$@"}
fi

# Local Variables:
# fill-column: 72
# End:
# vim: set autoindent expandtab shiftwidth=2 softtabstop=2 textwidth=72:
