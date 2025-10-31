#!@POSIX_SHELL_PROG@
# Copyright (C) 2004-2015 Free Software Foundation, Inc.
#               2023-2024 G. Branden Robinson
# Written by Mike Bianchi <MBianchi@Foveal.com>
# Changes from May 2015 onward by the groff development team

# Thanks to Peter Bray for debugging.

# This file is part of gdiffmk, which is part of groff, the GNU roff
# typesetting system.

# groff is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# groff is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
# License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
# This file is part of GNU gdiffmk.


CMD=`basename $0`

Diagnose () {
	echo >&2 "${CMD}: $@"
}

Usage () {
	status=0
	if test $# -gt 0
	then
		Diagnose "usage error: $@"
		exec 2>&1
		status=2
	fi
	cat >&2 <<EOF
usage: ${CMD} [-a add-mark] [-c change-mark] [-d delete-mark] \
[-x diff-command] [-D [-B] [-M mark1 mark2]] [--] file1 file2 \
[output-file]
usage: ${CMD} --version
usage: ${CMD} --help
EOF
	if [ -n "$want_help" ]
	then
		cat >&2 <<EOF

Compare roff(7) documents file1 and file2, and write another, derived
from both, to the standard output stream (or output-file), adding margin
character ('mc') requests at places in the output where the input
documents differ.
EOF
	fi
	exit $status
}


Exit () {
	exitcode=$1
	shift
	for arg
	do
		echo >&2 "${CMD}: $1"
		shift
	done
	exit ${exitcode}
}

#	Usage:  FileRead  exit_code  filename
#
#	Check for existence and readability of given file name.
#	If not found or not readable, print message and exit with EXIT_CODE.
FileRead () {
	case "$2" in
	-)
		return
		;;
	esac

	if test ! -f "$2"
	then
		Exit $1 "input file \"$2\" does not exist or is not a file"
	fi
	if test ! -r "$2"
	then
		Exit $1 "input file \"$2\" is not readable"
	fi
}


#	Usage:  FileCreate  exit_code  filename
#
#	Create the given filename if it doesn't exist.
#	If unable to create or write, print message and exit with EXIT_CODE.
FileCreate () {
	case "$2" in
	-)
		return
		;;
	esac

	touch "$2" 2>/dev/null
	if test $? -ne 0
	then
		if test ! -f "$2"
		then
			Exit $1 "File '$2' not created; " \
			  "Cannot write directory '`dirname "$2"`'."
		fi
		Exit $1 "File '$2' not writeable."
	fi
}

WouldClobber () {
	case "$2" in
	-)
		return
		;;
	esac

	cmd='test /dev/null -ef /dev/null > /dev/null 2>&1'
	if ! @POSIX_SHELL_PROG@ -c "$cmd"
	then
		Exit 3 \
		"The system's 'test' command does not support the" \
		" '-ef' option; OUTPUT can be only the standard output."
	else
		if test "$1" -ef "$3"
		then
			Exit 4 \
			"The $2 and OUTPUT arguments both point to the same file," \
			"'$1', and it would be overwritten."
		fi
	fi
}

ADDMARK='+'
CHANGEMARK='|'
DELETEMARK='*'
MARK1='[['
MARK2=']]'

# Given an option with an expected argument, echo the option argument.
# Return 0 if caller should further shift its argument list; 1 if not.
RequiresArgument () {
	case "$1" in
	-??*)
		optarg=${1#-?}
		option=${option%${optarg}}

		if test -z "${optarg}"
		then
			Exit 2 "option '${option}' requires an argument"
		fi

		echo "${optarg}"
		return 1
		;;
	*)
		echo "$2"
		return 0
		;;
	esac
}

BADOPTION=
DIFFCMD=@DIFF_PROG@
SEDCMD=sed
D_option=
br=.br
want_help=
while [ $# -gt 0 ]
do
	OPTION="$1"
	case "${OPTION}" in
	-a*)
		ADDMARK=`RequiresArgument "${OPTION}" "$2"`		&&
			shift
		;;
	-c*)
		CHANGEMARK=`RequiresArgument "${OPTION}" "$2"`		&&
			shift
		;;
	-d*)
		DELETEMARK=`RequiresArgument "${OPTION}" "$2"`		&&
			shift
		;;
	-D )
		D_option=D_option
		;;
	-M* )
		MARK1=`RequiresArgument "${OPTION}" "$2"`		&&
			shift
		if [ $# -lt 2 ]
		then
			Usage "Option '-M' is missing the MARK2 value."
		fi
		MARK2="$2"
		shift
		;;
	-B )
		br=.
		;;
	-s* )
		SEDCMD=`RequiresArgument "${OPTION}" "$2"`		&&
			shift
		;;
	-x* )
		DIFFCMD=`RequiresArgument "${OPTION}" "$2"`		&&
			shift
		;;
	--version)
		echo "${CMD} (groff) version @VERSION@"
		exit 0
		;;
	--help)
		want_help=yes
		Usage
		;;
	--)
		#	What follows  --  are file arguments
		shift
		break
		;;
	-)
		break
		;;
	-*)
		BADOPTION="invalid option '$1'"
		;;
	*)
		break
		;;
	esac
	shift
done

if ! ${DIFFCMD} -Dx /dev/null /dev/null >/dev/null 2>&1
then
	Exit 3 "The '${DIFFCMD}' program does not accept"	\
"the required '-Dname' option.
Use GNU diff instead.  See the '-x DIFFCMD' option.  You can also
install GNU diff as 'gdiff' on your system."
fi

if test -n "${BADOPTION}"
then
	Usage "${BADOPTION}"
fi

if test $# -lt 2 || test $# -gt 3
then
	Usage "expected 2 or 3 operands, got $#"
fi

if test "1$1" = "1-" && test "2$2" = "2-"
then
	Usage "attempting to compare standard input to itself"
fi

FILE1="$1"
FILE2="$2"

FileRead 1 "${FILE1}"
FileRead 2 "${FILE2}"

if test $# = 3
then
	case "$3" in
	-)
		#	output goes to standard output
		;;
	*)
		#	output goes to a file
		WouldClobber "${FILE1}" FILE1 "$3"
		WouldClobber "${FILE2}" FILE2 "$3"

		FileCreate 3 "$3"
		exec >$3
		;;
	esac
fi

#	To make a very unlikely LABEL even more unlikely ...
LABEL=__diffmk_$$__

SED_SCRIPT='
		/^#ifdef '"${LABEL}"'/,/^#endif \/\* '"${LABEL}"'/ {
		  /^#ifdef '"${LABEL}"'/          s/.*/.mc '"${ADDMARK}"'/
		  /^#endif \/\* '"${LABEL}"'/     s/.*/.mc/
		  p
		  d
		}
		/^#ifndef '"${LABEL}"'/,/^#endif \/\* [!not ]*'"${LABEL}"'/ {
		  /^#else \/\* '"${LABEL}"'/,/^#endif \/\* '"${LABEL}"'/ {
		    /^#else \/\* '"${LABEL}"'/    s/.*/.mc '"${CHANGEMARK}"'/
		    /^#endif \/\* '"${LABEL}"'/   s/.*/.mc/
		    p
		    d
		  }
		  /^#endif \/\* [!not ]*'"${LABEL}"'/ {
		   s/.*/.mc '"${DELETEMARK}"'/p
		   a\
.mc
		  }
		  d
		}
		p
	'

if [ ${D_option} ]
then
	SED_SCRIPT='
		/^#ifdef '"${LABEL}"'/,/^#endif \/\* '"${LABEL}"'/ {
		  /^#ifdef '"${LABEL}"'/          s/.*/.mc '"${ADDMARK}"'/
		  /^#endif \/\* '"${LABEL}"'/     s/.*/.mc/
		  p
		  d
		}
		/^#ifndef '"${LABEL}"'/,/^#endif \/\* [!not ]*'"${LABEL}"'/ {
		  /^#ifndef '"${LABEL}"'/ {
		   i\
'"${MARK1}"'
		   d
		  }
		  /^#else \/\* '"${LABEL}"'/ !{
		   /^#endif \/\* [!not ]*'"${LABEL}"'/ !{
		    p
		    d
		   }
		  }
		  /^#else \/\* '"${LABEL}"'/,/^#endif \/\* '"${LABEL}"'/ {
		    /^#else \/\* '"${LABEL}"'/ {
		     i\
'"${MARK2}"'\
'"${br}"'
		     s/.*/.mc '"${CHANGEMARK}"'/
		     a\
.mc '"${CHANGEMARK}"'
		     d
		    }
		    /^#endif \/\* '"${LABEL}"'/   s/.*/.mc/
		    p
		    d
		  }
		  /^#endif \/\* [!not ]*'"${LABEL}"'/ {
		   i\
'"${MARK2}"'\
'"${br}"'
		   s/.*/.mc '"${DELETEMARK}"'/p
		   a\
.mc
		  }
		  d
		}
		p
	'
fi

${DIFFCMD} -D"${LABEL}" -- "${FILE1}" "${FILE2}"  |
	${SEDCMD} -n "${SED_SCRIPT}"

# EOF
