#!/bin/sh
#
# grap2graph -- compile graph description descriptions to bitmap images
#
# by Eric S. Raymond <esr@thyrsus.com>, May 2003
# based on a recipe by W. Richard Stevens
#
# salves for shell portability agonies by G. Branden Robinson
#
# Take grap description on stdin, emit cropped bitmap on stdout.  The
# grap markup should *not* be wrapped in .G1/.G2, this script will do
# that.  A -U option on the command line enables gpic/groff "unsafe"
# mode.  A -format FOO option changes the image output format to any
# format supported by convert(1).  All other options are passed to
# convert(1).  The default format is PNG.
#
# Requires the groff suite and the ImageMagick tools.  Both are Free
# Software <https://www.gnu.org/philosophy/free-sw.en.html>.  This code
# is released to the public domain.
#
# Here are the assumptions behind the option processing:
#
# 1. None of the options of grap(1) are relevant.
#
# 2. Only the -U option of groff(1) is relevant.
#
# 3. Many options of convert(1) are potentially relevant, (especially
#    -density, -interlace, -transparency, -border, and -comment).
#
# Thus, we pass -U to groff(1), and everything else to convert(1).

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

usage="usage: $prog [-unsafe] [-format output-format] \
[convert-argument ...]
$prog {-v | --version}
$prog --help

Read a grap(1) program from the standard input and write an image file,
by default in Portable Network Graphics (PNG) format, to the standard
output.  See the grap2graph(1) manual page."

groff_opts=""
convert_opts=""
convert_trim_arg="-trim"
format="png"

while [ "$1" ]
do
    case $1 in
    -unsafe)
	groff_opts="-U";;
    -format)
	format=$2
	shift;;
    -v | --version)
	echo "$prog (groff) version @VERSION@"
	exit 0;;
    --help)
	echo "$usage"
	exit 0;;
    *)
	convert_opts="$convert_opts $1";;
    esac
    shift
done

# create temporary directory
tmp=
for d in "$GROFF_TMPDIR" "$TMPDIR" "$TMP" "$TEMP" /tmp
do
    test -n "$d" && break
done

if ! test -d "$d"
then
    echo "$0: error: temporary directory \"$d\" does not exist or is" \
        "not a directory" >&2
    exit 1
fi

if ! tmp=`(umask 077 && mktemp -d -q "$d/grap2graph-XXXXXX") 2> /dev/null`
then
    # mktemp failed--not installed or is a version that doesn't support
    # those flags?  Fall back to older method which uses more
    # predictable naming.
    #
    # $RANDOM is a Bashism.  The fallback of $PPID is not good
    # pseudorandomness, but is supported by the stripped-down dash
    # shell, for instance.
    tmp="$d/grap2graph$$-${RANDOM:-$PPID}"
    (umask 077 && mkdir "$tmp") 2> /dev/null
fi

if ! test -d "$tmp"
then
    echo "$0: error: cannot create temporary directory \"$tmp\"" >&2
    exit 1
fi

# See if the installed version of convert(1) is new enough to support the -trim
# option.  Versions that didn't were described as "old" as early as 2008.
is_convert_recent=`convert -help | grep -e -trim`
if test -z "$is_convert_recent"
then
    echo "$0: warning: falling back to old '-crop 0x0' trim method" >&2
    convert_trim_arg="-crop 0x0"
fi

trap 'exit_status=$?; rm -rf "$tmp" && exit $exit_status' EXIT INT TERM

# Here goes:
# 1. Add .G1/.G2.
# 2. Process through grap(1) to emit pic markup.
# 3. Process through groff(1) with pic preprocessing to emit Postscript.
# 4. Use convert(1) to crop the Postscript and turn it into a bitmap.
(echo ".G1"; cat; echo ".G2") | grap | groff -p $groff_opts -Tps -P-pletter | \
    convert -trim $convert_opts - "$tmp"/grap2graph.$format \
    && cat "$tmp"/grap2graph.$format

# End
