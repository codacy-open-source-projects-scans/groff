#!/usr/bin/env perl

# gperl - preprocess troff(1) input to execute embedded Perl code
#
# Copyright (C) 2014-2020 Free Software Foundation, Inc.
#                    2025 G. Branden Robinson
#
# Written by Bernd Warken <groff-bernd.warken-72@web.de>.
# Enhanced by: G. Branden Robinson <g.branden.robinson@gmail.com>
#
# This file is part of 'gperl'.
#
# 'gperl' is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# 'gperl' is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You can find a copy of the GNU General Public License in the internet
# at <http://www.gnu.org/licenses/gpl-2.0.html>.

########################################################################

use strict;
use warnings;
#use diagnostics;

# temporary dir and files
use File::Temp qw/ tempfile tempdir /;

# needed for temporary dir
use File::Spec;

# for 'copy' and 'move'
use File::Copy;

# for fileparse, dirname and basename
use File::Basename;

# current working directory
use Cwd;

# $Bin is the directory where this script is located
use FindBin;

my $gperl_version = '1.3.0';
my $groff_version = 'DEVELOPMENT';

my $is_in_source_tree;
{
  $is_in_source_tree = 1 if '@VERSION@' eq '@' . 'VERSION' . '@';
}

if (!$is_in_source_tree) {
  $groff_version = '@VERSION@';
}

########################################################################
# system variables and exported variables
########################################################################

$\ = "\n";	# final part for print command

########################################################################
# read-only variables with double-@ construct
########################################################################

our $File_split_env_sh;
our $File_version_sh;
our $Groff_Version;

my $before_make;		# script before run of 'make'
{
  my $at = '@';
  $before_make = 1 if '@VERSION@' eq "${at}VERSION${at}";
}

my %at_at;
my $file_perl_test_pl;
my $groffer_libdir;

if ($before_make) {
  my $gperl_source_dir = $FindBin::Bin;
  $at_at{'BINDIR'} = $gperl_source_dir;
  $at_at{'G'} = '';
} else {
  $at_at{'BINDIR'} = '@BINDIR@';
  $at_at{'G'} = '@g@';
}

(undef, undef, my $program_name) = File::Spec->splitpath($0);

sub usage {
  my $stream = *STDOUT;
  my $had_error = shift;
  $stream = *STDERR if $had_error;
  my $gperl = $program_name;
  print $stream "usage: $gperl [file ...]\n" .
    "usage: $gperl {-v | --version}\n" .
    "usage: $gperl {-h | --help}\n";
  unless ($had_error) {
    # Omit some newlines due to `$\` voodoo.
    print $stream "" .
"Filter troff(1) input, executing Perl code on lines between\n" .
"'.Perl start' and '.Perl end'.  See the gperl(1) manual page.";
  }
  exit $had_error;
}

sub version {
  # Omit newline due to `$\` voodoo.
  print "$program_name (groff $groff_version) $gperl_version";
  exit 0;
}

########################################################################
# options
########################################################################

foreach my $arg (@ARGV) {
  usage(0) if ($arg eq '-h' || $arg eq '--help');
  version() if ($arg eq '-v' || $arg eq '--version');
}


#######################################################################
# temporary file
#######################################################################

my $out_file;
{
  my $template = 'gperl_' . "$$" . '_XXXX';
  my $tmpdir;
  foreach ($ENV{'GROFF_TMPDIR'}, $ENV{'TMPDIR'}, $ENV{'TMP'}, $ENV{'TEMP'},
	   $ENV{'TEMPDIR'}, 'tmp', $ENV{'HOME'},
	   File::Spec->catfile($ENV{'HOME'}, 'tmp')) {
    if ($_ && -d $_ && -w $_) {
      eval { $tmpdir = tempdir( $template,
				CLEANUP => 1, DIR => "$_" ); };
      last if $tmpdir;
    }
  }
  $out_file = File::Spec->catfile($tmpdir, $template);
}


########################################################################
# input
########################################################################

my $perl_mode = 0;

unshift @ARGV, '-' unless @ARGV;
foreach my $filename (@ARGV) {
  my $input;
  if ($filename eq '-') {
    $input = \*STDIN;
  } elsif (not open $input, '<', $filename) {
    warn $!;
    next;
  }
  while (<$input>) {
    chomp;
    s/\s+$//;
    my $line = $_;
    my $is_dot_Perl = $line =~ /^[.']\s*Perl(|\s+.*)$/;

    unless ( $is_dot_Perl ) {	# not a '.Perl' line
      if ( $perl_mode ) {		# is running in Perl mode
        print OUT $line;
      } else {			# normal line, not Perl-related
        print $line;
      }
      next;
    }


    ##########
    # now the line is a '.Perl' line

    my $args = $line;
    $args =~ s/\s+$//;	# remove final spaces
    $args =~ s/^[.']\s*Perl\s*//;	# omit .Perl part, leave the arguments

    my @args = split /\s+/, $args;

    ##########
    # start Perl mode
    if ( @args == 0 || @args == 1 && $args[0] eq 'start' ) {
      # For '.Perl' no args or first arg 'start' means opening 'Perl' mode.
      # Everything else means an ending command.
      if ( $perl_mode ) {
        # '.Perl' was started twice, ignore
        print STDERR q('.Perl' starter was run several times);
        next;
      } else {	# new Perl start
        $perl_mode = 1;
        open OUT, '>', $out_file;
        next;
      }
    }

    ##########
    # now the line must be a Perl ending line (stop)

    unless ( $perl_mode ) {
      print STDERR 'gperl: there was a Perl ending without being in ' .
        'Perl mode:';
      print STDERR '    ' . $line;
      next;
    }

    $perl_mode = 0;	# 'Perl' stop calling is correct
    close OUT;		# close the storing of 'Perl' commands

    ##########
    # run this 'Perl' part, later on about storage of the result
    # array stores prints with \n
    my @print_res = `perl $out_file`;

    # remove 'stop' arg if exists
    shift @args if ( $args[0] eq 'stop' );

    if ( @args == 0 ) {
      # no args for saving, so @print_res doesn't matter
      next;
    }

    my @var_names = ();
    my @mode_names = ();

    my $mode = '.ds';
    for ( @args ) {
      if ( /^\.?ds$/ ) {
        $mode = '.ds';
        next;
      }
      if ( /^\.?nr$/ ) {
        $mode = '.nr';
        next;
      }
      push @mode_names, $mode;
      push @var_names, $_;
    }

    my $n_res = @print_res;
    my $n_vars = @var_names;

    if ( $n_vars < $n_res ) {
      print STDERR 'gperl: not enough variables for Perl part: ' .
        $n_vars . ' variables for ' . $n_res . ' output lines.';
    } elsif ( $n_vars > $n_res ) {
      print STDERR 'gperl: too many variablenames for Perl part: ' .
        $n_vars . ' variables for ' . $n_res . ' output lines.';
    }
    if ( $n_vars < $n_res ) {
      print STDERR 'gperl: not enough variables for Perl part: ' .
        $n_vars . ' variables for ' . $n_res . ' output lines.';
    }

    my $n_min = $n_res;
    $n_min = $n_vars if ( $n_vars < $n_res );
    exit unless ( $n_min );
    $n_min -= 1; # for starting with 0

    for my $i ( 0..$n_min ) {
      my $value = $print_res[$i];
      chomp $value;
      print $mode_names[$i] . ' ' . $var_names[$i] . ' ' . $value;
    }
  }
}


1;
# Local Variables:
# mode: CPerl
# End:
