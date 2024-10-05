#!@PERL@
#
#	pdfmom		: Frontend to run groff to produce PDFs
#	Deri James	: Friday 16 Mar 2012
#

# Copyright (C) 2012-2024 Free Software Foundation, Inc.
#      Written by Deri James <deri@chuzzlewit.myzen.co.uk>
#
# This file is part of groff.
#
# groff is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free
# Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# groff is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

use strict;
use warnings;
use File::Spec qw/splitpath/;
use File::Temp qw/tempfile/;

my @cmd;
my $dev='pdf';
my $preconv='';
my $readstdin=1;
my $mom='-mom';
my $zflg='';
if ($0=~m/pdf(\w+)$/)
{
    my $m=$1;
    if ($m=~m/^(mom|mm|ms|me|man|mandoc)$/)
    {
        $mom="-".$m;
    }
    else
    {
        $mom='';
    }
}
my $RT_SEP='@RT_SEP@';

$ENV{PATH}=$ENV{GROFF_BIN_PATH}.$RT_SEP.$ENV{PATH} if exists($ENV{GROFF_BIN_PATH});
$ENV{TMPDIR}=$ENV{GROFF_TMPDIR} if exists($ENV{GROFF_TMPDIR});

(undef,undef,my $prog)=File::Spec->splitpath($0);

sub abort
{
    my $message=shift(@_);
    print STDERR "$prog: fatal error: $message";
    exit 1;
}

sub autopsy
{
    my $waitstatus=shift(@_);
    my $finding;

    if ($waitstatus == -1) {
	$finding = "unable to run groff: $!\n";
    }
    elsif ($? & 127) {
	$finding = sprintf("groff died with signal %d, %s core dump\n",
	    ($? & 127),  ($? & 128) ? 'with' : 'without');
    }
    else {
	$finding = sprintf("groff exited with status %d\n", $? >> 8);
    }

    return $finding;
}

while (my $c=shift)
{
    $c=~s/(?<!\\)"/\\"/g;

    if (substr($c,0,2) eq '-T')
    {
	if (length($c) > 2)
	{
	    $dev=substr($c,2);
	}
	else
	{
	    $dev=shift;
	}
	next;
    }
    elsif (substr($c,0,2) eq '-K')
    {
	if (length($c) > 2)
	{
	    $preconv.=" $c";
	}
	else
	{
	    $preconv.=" $c";
	    $preconv.=shift;
	}
	next;
    }
    elsif (substr($c,0,2) eq '-k')
    {
	$preconv.=" $c";
	next;
    }
    elsif ($c eq '-Z')
    {
	$zflg=$c;
	next;
    }
    elsif ($c eq '-z')
    {
	$zflg="$c -dPDF.EXPORT=1";
	next;
    }
    elsif ($c eq '--roff')
    {
        $mom='';
    }
    elsif ($c eq '--help')
    {
	print "usage: pdfmom [--roff] [-Tpdf] [groff-option ...] [file ...]\n";
	print "usage: pdfmom [--roff] -Tps [pdfroff-option ...] [groff-option ...] [file ...]\n";
	print "usage: pdfmom {-v | --version}\n";
	print "usage: pdfmom --help\n";
	print "\nHandle forward references in groff(1) documents" .
	      " to be formatted as PDF.\n" .
	      "See the pdfmom(1) manual page.\n";
	exit;
    }
    elsif ($c eq '-v' or $c eq '--version')
    {
	print "GNU pdfmom (groff) version @VERSION@\n";
	exit;
    }
    elsif (substr($c,0,1) eq '-')
    {
	if (length($c) > 1)
	{
	    push(@cmd,"\"$c\"");
	    push(@cmd,"'".(shift)."'") if length($c)==2 and index('dDfFIKLmMnoPrwW',substr($c,-1)) >= 0;
	}
	else
	{
	    # Just a '-'

	    push(@cmd,$c);
	    $readstdin=2;
	}
    }
    else
    {
	# Got a filename?

	push(@cmd,"\"$c\"");
	$readstdin=0 if $readstdin == 1;

    }

}

my $cmdstring=' '.join(' ',@cmd).' ';

if ($readstdin)
{
    my ($fh,$tmpfn)=tempfile('pdfmom-XXXXX', UNLINK=>1);

    while (<STDIN>)
    {
	print $fh ($_);
    }

    close($fh);

    $cmdstring=~s/ - / $tmpfn / if $readstdin == 2;
    $cmdstring.=" $tmpfn " if $readstdin == 1;
}

my $waitstatus = 0;

if ($dev eq 'pdf')
{
    if ($mom)
    {
	$waitstatus = system("groff -Tpdf -dLABEL.REFS=1 $mom -z $cmdstring 2>&1 | LC_ALL=C grep '^\\. *ds' | groff -Tpdf $preconv -dPDF.EXPORT=1 -dLABEL.REFS=1 $mom -z - $cmdstring 2>&1 | LC_ALL=C grep '^\\. *ds' | groff -Tpdf $mom $preconv - $cmdstring $zflg");
	abort(autopsy($?)) unless $waitstatus == 0;

    }
    else
    {
	$waitstatus = system("groff -Tpdf $preconv -dPDF.EXPORT=1 -z $cmdstring 2>&1 | LC_ALL=C grep '^\\. *ds' | groff -Tpdf $preconv - $cmdstring $zflg");
	abort(autopsy($?)) unless $waitstatus == 0;
    }
}
elsif ($dev eq 'ps')
{
	$waitstatus = system("groff -Tpdf -dLABEL.REFS=1 $mom -z $cmdstring 2>&1 | LC_ALL=C grep '^\\. *ds' | pdfroff -mpdfmark $mom --no-toc - $preconv $cmdstring");
	abort(autopsy($?)) unless $waitstatus == 0;
}
elsif ($dev eq '-z') # pseudo dev - just compile for warnings
{
    $waitstatus = system("groff -Tpdf $mom -z $cmdstring");
    abort(autopsy($?)) unless $waitstatus == 0;
}
else
{
    abort("groff output device '$dev' not supported");
}

# Local Variables:
# fill-column: 72
# mode: CPerl
# End:
# vim: set cindent noexpandtab shiftwidth=4 softtabstop=4 textwidth=72:
