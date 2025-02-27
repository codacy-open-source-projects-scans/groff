#!@PERL@
#
#       gropdf          : PDF post processor for groff
#
# Copyright (C) 2011-2024 Free Software Foundation, Inc.
#      Written by Deri James <deri@chuzzlewit.myzen.co.uk>
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

use strict;
use warnings;
require 5.8.0;
use Getopt::Long qw(:config bundling);
use Encode qw(encode);
use POSIX qw(mktime);
use File::Spec qw(splitpath);

use constant
{
    WIDTH    => 0,
    CHRCODE  => 1,
    PSNAME   => 2,
    MINOR    => 3,
    MAJOR    => 4,
    UNICODE  => 5,

    CHR      => 0,
    XPOS     => 1,
    CWID     => 2,
    HWID     => 3,
    NOMV     => 4,
    CHF      => 5,

    MAGIC1   => 52845,
    MAGIC2   => 22719,
    C_DEF    => 4330,
    E_DEF    => 55665,

    LINE     => 0,
    CALLS    => 1,
    NEWNO    => 2,
    CHARCHAR => 3,

    NUMBER   => 0,
    LENGTH   => 1,
    STR      => 2,
    TYPE     => 3,

    SUBSET   => 1,
    USESPACE => 2,
    COMPRESS => 4,
    NOFILE   => 8,
};

my %StdEnc=(
    32 => 'space',
    33 => '!',
    34 => 'dq',
    35 => 'sh',
    36 => 'Do',
    37 => '%',
    38 => '&',
    39 => 'cq',
    40 => '(',
    41 => ')',
    42 => '*',
    43 => '+',
    44 => ',',
    45 => 'hy',
    46 => '.',
    47 => 'sl',
    48 => '0',
    49 => '1',
    50 => '2',
    51 => '3',
    52 => '4',
    53 => '5',
    54 => '6',
    55 => '7',
    56 => '8',
    57 => '9',
    58 => ':',
    59 => ';',
    60 => '<',
    61 => '=',
    62 => '>',
    63 => '?',
    64 => 'at',
    65 => 'A',
    66 => 'B',
    67 => 'C',
    68 => 'D',
    69 => 'E',
    70 => 'F',
    71 => 'G',
    72 => 'H',
    73 => 'I',
    74 => 'J',
    75 => 'K',
    76 => 'L',
    77 => 'M',
    78 => 'N',
    79 => 'O',
    80 => 'P',
    81 => 'Q',
    82 => 'R',
    83 => 'S',
    84 => 'T',
    85 => 'U',
    86 => 'V',
    87 => 'W',
    88 => 'X',
    89 => 'Y',
    90 => 'Z',
    91 => 'lB',
    92 => 'rs',
    93 => 'rB',
    94 => 'ha',
    95 => '_',
    96 => 'oq',
    97 => 'a',
    98 => 'b',
    99 => 'c',
    100 => 'd',
    101 => 'e',
    102 => 'f',
    103 => 'g',
    104 => 'h',
    105 => 'i',
    106 => 'j',
    107 => 'k',
    108 => 'l',
    109 => 'm',
    110 => 'n',
    111 => 'o',
    112 => 'p',
    113 => 'q',
    114 => 'r',
    115 => 's',
    116 => 't',
    117 => 'u',
    118 => 'v',
    119 => 'w',
    120 => 'x',
    121 => 'y',
    122 => 'z',
    123 => 'lC',
    124 => 'ba',
    125 => 'rC',
    126 => 'ti',
    161 => 'r!',
    162 => 'ct',
    163 => 'Po',
    164 => 'f/',
    165 => 'Ye',
    166 => 'Fn',
    167 => 'sc',
    168 => 'Cs',
    169 => 'aq',
    170 => 'lq',
    171 => 'Fo',
    172 => 'fo',
    173 => 'fc',
    174 => 'fi',
    175 => 'fl',
    177 => 'en',
    178 => 'dg',
    179 => 'dd',
    180 => 'pc',
    182 => 'ps',
    183 => 'bu',
    184 => 'bq',
    185 => 'Bq',
    186 => 'rq',
    187 => 'Fc',
    188 => 'u2026',
    189 => '%0',
    191 => 'r?',
    193 => 'ga',
    194 => 'aa',
    195 => 'a^',
    196 => 'a~',
    197 => 'a-',
    198 => 'ab',
    199 => 'a.',
    200 => 'ad',
    202 => 'ao',
    203 => 'ac',
    205 => 'a"',
    206 => 'ho',
    207 => 'ah',
    208 => 'em',
    225 => 'AE',
    227 => 'Of',
    232 => '/L',
    233 => '/O',
    234 => 'OE',
    235 => 'Om',
    241 => 'ae',
    245 => '.i',
    248 => '/l',
    249 => '/o',
    250 => 'oe',
    251 => 'ss',
);

(undef,undef,my $prog)=File::Spec->splitpath($0);

unshift(@ARGV,split(' ',$ENV{GROPDF_OPTIONS})) if exists($ENV{GROPDF_OPTIONS});

my $gotzlib=0;
my $gotinline=0;
my $gotexif=0;

my $rc = eval
{
    require Compress::Zlib;
    Compress::Zlib->import();
    1;
};

if($rc)
{
    $gotzlib=1;
}
else
{
    Warn("Perl module 'Compress::Zlib' not available; cannot compress"
    . " this PDF");
}

mkdir $ENV{HOME}.'/_Inline' if !-e $ENV{HOME}.'/_Inline' and !exists($ENV{PERL_INLINE_DIRECTORY}) and exists($ENV{HOME});

$rc = eval
{
    require Inline;
    Inline->import (C => Config => DIRECTORY => $ENV{HOME}.'/_Inline') if !exists($ENV{PERL_INLINE_DIRECTORY}) and exists($ENV{HOME});
    Inline->import (C =><<'EOC');

    static const uint32_t MAGIC1 = 52845;
    static const uint32_t MAGIC2 = 22719;

    typedef unsigned char byte;

    char* decrypt_exec_C(char *s, int len)
    {
        static uint16_t er=55665;
        byte clr=0;
        int i;
        er=55665;

        for (i=0; i < len; i++)
        {
            byte cypher = s[i];
            clr = (byte)(cypher ^ (er >> 8));
            er = (uint16_t)((cypher + er) * MAGIC1 + MAGIC2);
            s[i] = clr;
        }

        return(s);
    }

EOC
};

if($rc)
{
    $gotinline=1;
}

$rc = eval
{
#     require Image::ExifTool;
#     Image::ExifTool->import();
    require Image::Magick;
    Image::Magick->import();
    1;
};

if($rc)
{
    $gotexif=1;
}

my %cfg;

$cfg{GROFF_VERSION}='@VERSION@';
$cfg{GROFF_FONT_PATH}='@GROFF_FONT_DIR@';
$cfg{RT_SEP}='@RT_SEP@';
binmode(STDOUT);

my @obj;	# Array of PDF objects
my $objct=0;    # Count of Objects
my $fct=0;      # Output count
my %fnt;	# Used fonts
my $lct=0;      # Input Line Count
my $src_name='';
my %env;	# Current environment
my %fontlst;    # Fonts Loaded
my $rot=0;      # Portrait
my %desc;       # Contents of DESC
my %download;   # Contents of downlopad file
my $pages;      # Pointer to /Pages object
my $devnm='devpdf';
my $cpage;      # Pointer to current pages
my $cpageno=0;  # Object no of current page
my $cat;	# Pointer to catalogue
my $dests;      # Pointer to Dests
my @mediabox=(0,0,595,842);
my @defaultmb=(0,0,595,842);
my $stream='';  # Current Text/Graphics stream
my $cftsz=10;   # Current font sz
my $cft;	# Current Font
my $lwidth=1;   # current linewidth
my $linecap=1;
my $linejoin=1;
my $textcol=''; # Current groff text
my $fillcol=''; # Current groff fill
my $curfill=''; # Current PDF fill
my $strkcol='';
my $curstrk='';
my @lin=();     # Array holding current line of text
my @ahead=();   # Buffer used to hol the next line
my $mode='g';   # Graphic (g) or Text (t) mode;
my $xpos=0;     # Current X position
my $ypos=0;     # Current Y position
my $tmxpos=0;
my $kernadjust=0;
my $curkern=0;
my $krntbl;     # Pointer to kern table
my $matrix="1 0 0 1";
my $whtsz;      # Current width of a space
my $wt;
my $poschg=0;   # V/H pending
my $fontchg=0;  # font change pending
my $tnum=2;     # flatness of B-Spline curve
my $tden=3;     # flatness of B-Spline curve
my $linewidth=40;
my $w_flg=0;
my $gotT=0;
my $suppress=0; # Suppress processing?
my %incfil;     # Included Files
my @outlev=([0,undef,0,0]);     # Structure pdfmark /OUT entries
my $curoutlev=\@outlev;
my $curoutlevno=0;      # Growth point for @curoutlev
my $Foundry='';
my $xrev=0;     # Reverse x direction of font
my $inxrev=0;
my $matrixchg=0;
my $thislev=1;
my $mark=undef;
my $suspendmark=undef;
my $boxmax=0;
my %missing;    # fonts in download files which are not found/readable
my @PageLabel;  # PageLabels

my $n_flg=1;
my $pginsert=-1;    # Growth point for kids array
my %pgnames;	    # 'names' of pages for switchtopage
my @outlines=();    # State of Bookmark Outlines at end of each page
my $custompaper=0;  # Has there been an X papersize
my $textenccmap=''; # CMap for groff text.enc encoding
my @XOstream=();
my @PageAnnots={};
my $noslide=0;
my $ND='ND';
my $NP='NP';
my $RD='RD';
my $transition={PAGE => {Type => '/Trans', S => '', D => 1, Dm => '/H', M => '/I', Di => 0, SS => 1.0, B => 0},
		BLOCK => {Type => '/Trans', S => '', D => 1, Dm => '/H', M => '/I', Di => 0, SS => 1.0, B => 0}};
my $firstpause=0;
my $present=0;
my @bgstack;	   # Stack of background boxes
my $bgbox='';	   # Draw commands for boxes on this page

$noslide=1 if exists($ENV{GROPDF_NOSLIDE}) and $ENV{GROPDF_NOSLIDE};

my %ppsz=(
    'ledger'=>[1224,792],
    'legal'=>[612,1008],
    'letter'=>[612,792],
    'a0'=>[2384,3370],
    'a1'=>[1684,2384],
    'a2'=>[1191,1684],
    'a3'=>[842,1191],
    'a4'=>[595,842],
    'a5'=>[420,595],
    'a6'=>[297,420],
    'a7'=>[210,297],
    'a8'=>[148,210],
    'a9'=>[105,148],
    'a10'=>[73,105],
    'b0'=>[2835,4008],
    'b1'=>[2004,2835],
    'b2'=>[1417,2004],
    'b3'=>[1001,1417],
    'b4'=>[709,1001],
    'b5'=>[499,709],
    'b6'=>[354,499],
    'c0'=>[2599,3677],
    'c1'=>[1837,2599],
    'c2'=>[1298,1837],
    'c3'=>[918,1298],
    'c4'=>[649,918],
    'c5'=>[459,649],
    'c6'=>[323,459],
    'com10'=>[297,684],
);

my $ucmap=<<'EOF';
/CIDInit /ProcSet findresource begin
12 dict begin
begincmap
/CIDSystemInfo
<< /Registry (Adobe)
/Ordering (UCS)
/Supplement 0
>> def
/CMapName /Adobe-Identity-UCS def
/CMapType 2 def
1 begincodespacerange
<0000> <FFFF>
endcodespacerange
1 beginbfrange
<001f> <001f> <002d>
endbfrange
endcmap
CMapName currentdict /CMap defineresource pop
end
end
EOF

sub usage
{
    my $stream = *STDOUT;
    my $had_error = shift;
    $stream = *STDERR if $had_error;
    print $stream
"usage: $prog [-dels] [-F font-directory] [-I inclusion-directory]" .
" [-p paper-format] [-u [cmap-file]] [-y foundry] [file ...]\n" .
"usage: $prog {-v | --version}\n" .
"usage: $prog --help\n";
    if (!$had_error)
    {
	print $stream "\n" .
"Translate the output of troff(1) into Portable Document Format.\n" .
"See the gropdf(1) manual page.\n";
    }
    exit($had_error);
}

my $fd;
my $frot;
my $fpsz;
my $embedall=0;
my $debug=0;
my $want_help=0;
my $version=0;
my $stats=0;
my $unicodemap;
my $options=7;
my $PDFver=1.7;
my @idirs;

my $alloc=-1;
my $cftmajor=0;
my $lenIV=4;
my %sec;
my $Glyphs='';
my (@glyphused,@subrused,%glyphseen);
my $newsub=4;
my $term="\n";
my @bl;
my %seac;
my $thisfnt;
my $parcln=qr/\[[^\]]*?\]|(?<term>.)((?!\g{term}).)*\g{term}/;
my $parclntyp=qr/(?:[\d\w]|\([+-]?[\S]{2}|$parcln)/;

if (!GetOptions('F=s' => \$fd, 'I=s' => \@idirs, 'l' => \$frot,
    'p=s' => \$fpsz, 'd!' => \$debug, 'help' => \$want_help, 'pdfver=f' => \$PDFver,
    'v' => \$version, 'version' => \$version, 'opt=s' => \$options,
    'e' => \$embedall, 'y=s' => \$Foundry, 's' => \$stats,
    'u:s' => \$unicodemap))
{
    &usage(1);
}

unshift(@idirs,'.');

&usage(0) if ($want_help);

if ($version)
{
    print "GNU gropdf (groff) version $cfg{GROFF_VERSION}\n";
    exit;
}

if (defined($unicodemap))
{
    if ($unicodemap eq '')
    {
	$ucmap='';
    }
    elsif (-r $unicodemap)
    {
	local $/;
	open(F,"<$unicodemap") or Die("failed to open '$unicodemap'");
	($ucmap)=(<F>);
	close(F);
    }
    else
    {
	Warn("failed to find '$unicodemap'; ignoring");
    }
}

if ($PDFver != 1.4 and $PDFver != 1.7)
{
    Warn("Only pdf versions 1.4 or 1.7 are supported, not '$PDFver'");
    $PDFver=1.7;
}

$PDFver=int($PDFver*10)-10;

# Search for 'font directory': paths in -f opt, shell var
# GROFF_FONT_PATH, default paths

my $fontdir=$cfg{GROFF_FONT_PATH};
$fontdir=$ENV{GROFF_FONT_PATH}.$cfg{RT_SEP}.$fontdir if exists($ENV{GROFF_FONT_PATH});
$fontdir=$fd.$cfg{RT_SEP}.$fontdir if defined($fd);

$rot=90 if $frot;
$matrix="0 1 -1 0" if $frot;

LoadDownload();
LoadDesc();

my $unitwidth=$desc{unitwidth};

$env{FontHT}=0;
$env{FontSlant}=0;
MakeMatrix();

my $possiblesizes = $desc{papersize};
$possiblesizes = $fpsz if $fpsz;
my $papersz;
for $papersz ( split(" ", lc($possiblesizes).' #duff#') )
{
    # No valid papersize found?
    if ($papersz eq '#duff#')
    {
	Warn("ignoring unrecognized paper format(s) '$possiblesizes'");
	last;
    }

    # Check for "/etc/papersize"
    elsif (substr($papersz,0,1) eq '/' and -r $papersz)
    {
	if (open(P,"<$papersz"))
	{
	    while (<P>)
	    {
		chomp;
		s/# .*//;
		next if $_ eq '';
		$papersz=lc($_);
		last;
	    }
	    close(P);
	}
    }

    # Allow height,width specified directly in centimeters, inches, or
    # points.
    if ($papersz=~m/([\d.]+)([cipP]),([\d.]+)([cipP])/)
    {
	@defaultmb=@mediabox=(0,0,ToPoints($3,$4),ToPoints($1,$2));
	last;
    }
    # Look $papersz up as a name such as "a4" or "letter".
    elsif (exists($ppsz{$papersz}))
    {
	@defaultmb=@mediabox=(0,0,$ppsz{$papersz}->[0],$ppsz{$papersz}->[1]);
	last;
    }
    # Check for a landscape version
    elsif (substr($papersz,-1) eq 'l' and exists($ppsz{substr($papersz,0,-1)}))
    {
	# Note 'legal' ends in 'l' but will be caught above
	@defaultmb=@mediabox=(0,0,$ppsz{substr($papersz,0,-1)}->[1],$ppsz{substr($papersz,0,-1)}->[0]);
	last;
    }

    # If we get here, $papersz was invalid, so try the next one.
}

my $dt=PDFDate(time);

my %info=('Creator' => "(groff version $cfg{GROFF_VERSION})",
	  'Producer' => "(gropdf version $cfg{GROFF_VERSION})",
	  'ModDate' => "($dt)",
	  'CreationDate' => "($dt)");
map { $_="< ".$_."\0" } @ARGV;

while (<>)
{
    chomp;
    s/\r$//;
    $lct++;

    do  # The ahead buffer behaves like 'ungetc'
    {{
	if (scalar(@ahead))
	{
	    $_=shift(@ahead);
	}


	my $cmd=substr($_,0,1);
	next if $cmd eq '#';    # just a comment
	my $lin=substr($_,1);

	while ($cmd eq 'w')
	{
	    $cmd=substr($lin,0,1);
	    $lin=substr($lin,1);
	    $w_flg=1 if $gotT;
	}

	$lin=~s/^\s+//;
#	$lin=~s/\s#.*?$//;	# remove comment
	$stream.="\% $_\n" if $debug;

	do_x($lin),next if ($cmd eq 'x');
	next if $suppress;
	do_p($lin),next if ($cmd eq 'p');
	do_f($lin),next if ($cmd eq 'f');
	do_s($lin),next if ($cmd eq 's');
	do_m($lin),next if ($cmd eq 'm');
	do_D($lin),next if ($cmd eq 'D');
	do_V($lin),next if ($cmd eq 'V');
	do_v($lin),next if ($cmd eq 'v');
	do_t($lin),next if ($cmd eq 't');
	do_u($lin),next if ($cmd eq 'u');
	do_C($lin),next if ($cmd eq 'C');
	do_c($lin),next if ($cmd eq 'c');
	do_N($lin),next if ($cmd eq 'N');
	do_h($lin),next if ($cmd eq 'h');
	do_H($lin),next if ($cmd eq 'H');
	do_n($lin),next if ($cmd eq 'n');

	my $tmp=scalar(@ahead);
    }} until scalar(@ahead) == 0;
}

exit 0 if $lct==0;

if ($cpageno > 0)
{
    my $trans='BLOCK';

    $trans='PAGE' if $firstpause;

    if (scalar(@XOstream))
    {
	MakeXO() if $stream;
	$stream=join("\n",@XOstream)."\n";
    }

    my %t=%{$transition->{$trans}};
    $cpage->{MediaBox}=\@mediabox if $custompaper;
    $cpage->{Trans}=FixTrans(\%t) if $t{S};

    if ($#PageAnnots >= 0)
    {
	@{$cpage->{Annots}}=@PageAnnots;
    }

    if ($#bgstack > -1 or $bgbox)
    {
	my $box="q 1 0 0 1 0 0 cm ";

	foreach my $bg (@bgstack)
	{
	    # 0=$bgtype # 1=stroke 2=fill. 4=page
	    # 1=$strkcol
	    # 2=$fillcol
	    # 3=(Left,Top,Right,bottom,LineWeight)
	    # 4=Start ypos
	    # 5=Endypos
	    # 6=Line Weight

	    my $pg=$bg->[3] || \@mediabox;

	    $bg->[5]=$pg->[3];	# box is continuing to next page
	    $box.=DrawBox($bg);
	    $bg->[4]=$pg->[1];	# will continue from page top
	}

	$stream=$box.$bgbox."Q\n".$stream;
	$bgbox='';
    }

    $boxmax=0;
    PutObj($cpageno);
    OutStream($cpageno+1);
}

$cat->{PageMode}='/UseOutlines' if $#outlev > 0;
$cat->{PageMode}='/FullScreen' if $present;

PutOutlines(\@outlev);

my $info=BuildObj(++$objct,\%info);

PutObj($objct);

foreach my $fontno (sort keys %fontlst)
{
    my $f=$fontlst{$fontno};
    my $fnt=$f->{FNT};
    my $nam=$fnt->{NAM};
    my ($head,$body,$tail);
    my $objno=$f->{OBJNO};
    my @fontdesc=();
    my $chars=$fnt->{TRFCHAR};
    my $glyphs='/.notdef';
    $glyphs.='/space' if defined($fnt->{NO}->[32]) and $fnt->{NO}->[32] eq 'space';
    my $fobj;
    @glyphused=@subrused=%seac=();
    push(@subrused,'#0','#1','#2','#3','#4');
    $newsub=4;
    %sec=();
    $thisfnt=$fnt;

    for (my $j=0; $j<=$#{$chars}; $j++)
    {
	$glyphs.=join('',@{$fnt->{CHARSET}->[$j]});
    }

    if (exists($fnt->{fontfile}))
    {
	$fnt->{FONTFILE}=BuildObj(++$objct,
				   {'Length1' => 0,
				    'Length2' => 0,
				    'Length3' => 0
				   }
	), $fobj=$objct if !($options & NOFILE);

	($head,$body,$tail)=GetType1($fnt->{fontfile});
	$head=~s/\/Encoding \d.*?readonly def\b/\/Encoding StandardEncoding def/s;
	$lenIV=4;

	if ($options & SUBSET)
	{
	    $lenIV=$1 if $head=~m'/lenIV\s+(\d+)';
	    my $l=length($body);
	    my $b=($gotinline)?decrypt_exec_C($body,$l):decrypt_exec_P(\$body,$l);
	    $body=substr($body,$lenIV);
	    $body=~m/begin([\r\n]+)/;
	    $term=$1;
	    if (defined($term))
	    {
		(@bl)=split("$term",$body);
		map_subrs(\@bl);
		Subset(\@bl,$glyphs);
	    }
	    else
	    {
		Warn("Unable to parse font '$fnt->{internalname}' for subsetting")
	    }
	}
    }

    for (my $j=0; $j<=$#{$chars}; $j++)
    {
	my @differ;
	my $firstch;
	my $lastch=0;
	my @widths;
	my $miss=-1;
	my $CharSet=join('',@{$fnt->{CHARSET}->[$j]});
	push(@{$chars->[$j]},'space') if $j==0 and $fnt->{NAM}->{space}->[PSNAME];

	foreach my $og (sort { $nam->{$a}->[MINOR] <=> $nam->{$b}->[MINOR] } (@{$chars->[$j]}))
	{
	    my $g=$og;

	    while ($g or $g eq '0')
	    {
		my ($glyph,$trf)=GetNAM($fnt,$g);
		my $chrno=$glyph->[MINOR];
		$firstch=$chrno if !defined($firstch);
		$lastch=$chrno;
		$widths[$chrno-$firstch]=$glyph->[WIDTH];

		push(@differ,$chrno) if $chrno > $miss;
		$miss=$chrno+1;
		my $ps=$glyph->[PSNAME];
		push(@differ,$ps);

		if (exists($seac{$trf}))
		{
		    $g=pop(@{$seac{$ps}});
		    $CharSet.=$g if $g;
		}
		else
		{
		    $g='';
		}
	    }
	}

	foreach my $w (@widths) {$w=0 if !defined($w);}
	my $fontnm=$fontno.(($j)?".$j":'');
	$fnt->{FirstChar}=$firstch;
	$fnt->{LastChar}=$lastch;
	$fnt->{Differences}=\@differ;
	$fnt->{Widths}=\@widths;
	$fnt->{CharSet}=$CharSet;
	$fnt->{'ToUnicode'}=$textenccmap if $j==0 and $CharSet=~m'/minus';

	$objct++;
	push(@fontdesc,EmbedFont($fontnm,$fnt));
	$pages->{'Resources'}->{'Font'}->{'F'.$fontnm}=$fontlst{$fontnm}->{OBJ};
	$obj[$objct-2]->{DATA}->{'ToUnicode'}=$textenccmap if (exists($fnt->{ToUnicode}));
    }

    if (exists($fnt->{fontfile}))
    {
	if ($options & SUBSET and !($options & NOFILE))
	{
	    if (defined($term))
	    {
		$body=encrypt(\@bl);
	    }
	}

	if (defined($fobj))
	{
	    $obj[$fobj]->{STREAM}=$head.$body.$tail;
	    $obj[$fobj]->{DATA}->{Length1}=length($head);
	    $obj[$fobj]->{DATA}->{Length2}=length($body);
	    $obj[$fobj]->{DATA}->{Length3}=length($tail);
	}

	foreach my $o (@fontdesc)
	{
	    $obj[$o]->{DATA}->{FontFile}=$fnt->{FONTFILE} if !($options & NOFILE);
	    if ($options & SUBSET)
	    {
		my $nm='/'.SubTag().$fnt->{internalname};
		$obj[$o]->{DATA}->{FontName}=$nm;
		$obj[$o-2]->{DATA}->{BaseFont}=$nm;
	    }
	}
    }
}

foreach my $j (0..$#{$pages->{Kids}})
{
    my $pg=GetObj($pages->{Kids}->[$j]);

    if (defined($PageLabel[$j]))
    {
	push(@{$cat->{PageLabels}->{Nums}},$j,$PageLabel[$j]);
    }
}

if (exists($cat->{PageLabels}) and $cat->{PageLabels}->{Nums}->[0] != 0)
{
    unshift(@{$cat->{PageLabels}->{Nums}},0,{S => "/D"});
}

PutObj(1);
PutObj(2);

my $objidx=-1;
my @obji;
my $tobjct=$objct;
my $omaj=-1;

foreach my $o (3..$objct)
{
    if (!exists($obj[$o]->{XREF}))
    {
	if ($PDFver!=4 and !exists($obj[$o]->{STREAM}) and ref($obj[$o]->{DATA}) eq 'HASH')
	{
	    # This can be put into an ObjStm
	    my $maj=int(++$objidx/128);
	    my $min=$objidx % 128;

	    if ($maj > $omaj)
	    {
		$omaj=$maj;
		BuildObj(++$tobjct,
		{
		    'Type' => '/ObjStm',
		}
		);

		$obji[$maj]=[$tobjct,0,'',''];
		$obj[$tobjct]->{DATA}->{Extends}=($tobjct-1)." 0 R" if $maj > 0;
	    }

	    $obj[$o]->{INDIRECT}=[$tobjct,$min];
	    $obji[$maj]->[1]++;
	    $obji[$maj]->[2].=' ' if $obji[$maj]->[2];
	    $obji[$maj]->[2].="$o ".length($obji[$maj]->[3]);
	    PutObj($o,\$obji[$maj]->[3]);
	}
	else
	{
	    PutObj($o);
	}
    }
}

foreach my $maj (0..$#obji)
{
    my $obji=$obji[$maj];
    my $objno=$obji->[0];

    $obj[$objno]->{DATA}->{N}=$obji->[1];
    $obj[$objno]->{DATA}->{First}=length($obji->[2]);
    $obj[$objno]->{STREAM}=$obji->[2].$obji->[3];
    PutObj($objno);
}

$objct=$tobjct;

#my $encrypt=BuildObj(++$objct,{'Filter' => '/Standard', 'V' => 1, 'R' => 2, 'P' => 252});
#PutObj($objct);

my $xrefct=$fct;

$objct+=1;

if ($PDFver == 4)
{
    print "xref\n0 $objct\n0000000000 65535 f \n";

    foreach my $j (1..$#obj)
    {
	my $xr=$obj[$j];
	next if !defined($xr);
	printf("%010d 00000 n \n",$xr->{XREF});
    }

    print "trailer\n<<\n/Info $info\n/Root 1 0 R\n/Size $objct\n>>\n";
}
else
{
    BuildObj($objct++,
    {
	'Type' => '/XRef',
	'W' => [1, 4, 1],
	'Info' => $info,
	'Root' => "1 0 R",
	'Size' => $objct,
    });

    $stream=pack('CNC',0,0,0);

    foreach my $j (1..$#obj)
    {
	my $xr=$obj[$j];
	next if !defined($xr);

	if (exists($xr->{INDIRECT}))
	{
	    $stream.=pack('CNC',2,@{$xr->{INDIRECT}});
	}
	else
	{
	    if (exists($xr->{XREF}))
	    {
		$stream.=pack('CNC',1,$xr->{XREF},0);
	    }
	}
    }

    $stream.=pack('CNC',1,$fct,0);
    $obj[$objct-1]->{STREAM}=$stream;
    PutObj($objct-1);
    print "trailer\n<<\n/Root 1 0 R\n/Size $objct\n>>\n";
}

print "startxref\n$xrefct\n\%\%EOF\n";
print "\% Pages=$pages->{Count}\n" if $stats;

sub MakeMatrix
{
    my $fontxrev=shift||0;
    my @mat=($frot)?(0,1,-1,0):(1,0,0,1);

    if (!$frot)
    {
	if ($env{FontHT} != 0)
	{
	    $mat[3]=sprintf('%.3f',$env{FontHT}/$cftsz);
	}

	if ($env{FontSlant} != 0)
	{
	    my $slant=$env{FontSlant};
	    $slant*=$env{FontHT}/$cftsz if $env{FontHT} != 0;
	    my $ang=rad($slant);

	    $mat[2]=sprintf('%.3f',sin($ang)/cos($ang));
	}

	if ($fontxrev)
	{
	    $mat[0]=-$mat[0];
	}
    }

    $matrix=join(' ',@mat);
    $matrixchg=1;
}

sub PutOutlines
{
    my $o=shift;
    my $outlines;

    if ($#{$o} > 0)
    {
	# We've got Outlines to deal with
	my $openct=$curoutlev->[0]->[2];

	while ($thislev-- > 1)
	{
	    my $nxtoutlev=$curoutlev->[0]->[1];
	    $nxtoutlev->[0]->[2]+=$openct if $curoutlev->[0]->[3]==1;
	    $openct=0 if $nxtoutlev->[0]->[3]==-1;
	    $curoutlev=$nxtoutlev;
	}

	$cat->{Outlines}=BuildObj(++$objct,{'Count' => abs($o->[0]->[0])+$o->[0]->[2]});
	$outlines=$obj[$objct]->{DATA};
    }
    else
    {
	return;
    }

    SetOutObj($o);

    $outlines->{First}=$o->[1]->[2];
    $outlines->{Last}=$o->[$#{$o}]->[2];

    LinkOutObj($o,$cat->{Outlines});
}

sub SetOutObj
{
    my $o=shift;

    for my $j (1..$#{$o})
    {
	my $ono=BuildObj(++$objct,$o->[$j]->[0]);
	$o->[$j]->[2]=$ono;

	SetOutObj($o->[$j]->[1]) if $#{$o->[$j]->[1]} > -1;
    }
}

sub LinkOutObj
{
    my $o=shift;
    my $parent=shift;

    for my $j (1..$#{$o})
    {
	my $op=GetObj($o->[$j]->[2]);

	$op->{Next}=$o->[$j+1]->[2] if ($j < $#{$o});
	$op->{Prev}=$o->[$j-1]->[2] if ($j > 1);
	$op->{Parent}=$parent;

	if ($#{$o->[$j]->[1]} > -1)
	{
	    $op->{Count}=$o->[$j]->[1]->[0]->[2]*$o->[$j]->[1]->[0]->[3];# if exists($op->{Count}) and $op->{Count} > 0;
	    $op->{First}=$o->[$j]->[1]->[1]->[2];
	    $op->{Last}=$o->[$j]->[1]->[$#{$o->[$j]->[1]}]->[2];
	    LinkOutObj($o->[$j]->[1],$o->[$j]->[2]);
	}
    }
}

sub GetObj
{
    my $ono=shift;
    ($ono)=split(' ',$ono);
    return($obj[$ono]->{DATA});
}

sub PDFDate
{
    my $ts=shift;
    my @dt;
    my $offset;
    my $rel;
    if ($ENV{SOURCE_DATE_EPOCH}) {
	$offset=0;
	@dt=gmtime($ENV{SOURCE_DATE_EPOCH});
    } else {
	@dt=localtime($ts);
	$offset=mktime(@dt[0..5]) - mktime((gmtime $ts)[0..5]);
    }
    $rel=($offset==0)?'Z':($offset>0)?'+':'-';
    return(sprintf("D:%04d%02d%02d%02d%02d%02d%s%02d'%02d'",$dt[5]+1900,$dt[4]+1,$dt[3],$dt[2],$dt[1],$dt[0],$rel,int(abs($offset)/3600),int((abs($offset)%3600)/60)));
}

sub ToPoints
{
    my $num=shift;
    my $unit=shift;

    if ($unit eq 'i')
    {
	return($num*72);
    }
    elsif ($unit eq 'c')
    {
	return int($num*72/2.54);
    }
    elsif ($unit eq 'm')	# millimetres
    {
	return int($num*72/25.4);
    }
    elsif ($unit eq 'p')
    {
	return($num);
    }
    elsif ($unit eq 'P')
    {
	return($num*6);
    }
    elsif ($unit eq 'z')
    {
	return($num/$unitwidth);
    }
    else
    {
	Die("invalid scaling unit '$unit'");
    }
}

sub LoadDownload
{
    my $f;
    my $found=0;

    my (@dirs)=split($cfg{RT_SEP},$fontdir);

    foreach my $dir (@dirs)
    {
	$f=undef;
	OpenFile(\$f,$dir,"download");
	next if !defined($f);
	$found++;

	while (<$f>)
	{
	    chomp;
	    s/#.*$//;
	    next if $_ eq '';
	    my ($foundry,$name,$file)=split(/\t+/);
	    if (substr($file,0,1) eq '*')
	    {
		next if !$embedall;
		$file=substr($file,1);
	    }

	    my $pth=$file;
	    $pth=$dir."/$devnm/$file" if substr($file,0,1) ne '/';

	    if (!-r $pth)
	    {
		$missing{"$foundry $name"}="$dir/$devnm";
		next;
	    }

	    $download{"$foundry $name"}=$file if !exists($download{"$foundry $name"});
	}

	close($f);
    }

    Die("failed to open 'download' file") if !$found;
}

sub OpenFile
{
    my $f=shift;
    my $dirs=shift;
    my $fnm=shift;

    if (substr($fnm,0,1)  eq '/' or substr($fnm,1,1) eq ':') # dos
    {
	return if -r "$fnm" and open($$f,"<$fnm");
    }

    my (@dirs)=split($cfg{RT_SEP},$dirs);

    foreach my $dir (@dirs)
    {
	last if -r "$dir/$devnm/$fnm" and open($$f,"<$dir/$devnm/$fnm");
    }
}

sub LoadDesc
{
    my $f;

    OpenFile(\$f,$fontdir,"DESC");
    Die("failed to open device description file 'DESC'")
    if !defined($f);

    while (<$f>)
    {
	chomp;
	s/#.*$//;
	next if $_ eq '';
	my ($name,$prms)=split(' ',$_,2);
	$desc{lc($name)}=$prms;
    }

    close($f);

    foreach my $directive ('unitwidth', 'res', 'sizescale')
    {
	Die("device description file 'DESC' missing mandatory directive"
	. " '$directive'") if !exists($desc{$directive});
    }

    foreach my $directive ('unitwidth', 'res', 'sizescale')
    {
	my $val=$desc{$directive};
	Die("device description file 'DESC' directive '$directive'"
	. " value must be positive; got '$val'")
	if ($val !~ m/^\d+$/ or $val <= 0);
    }

    if (exists($desc{'hor'}))
    {
	my $hor=$desc{'hor'};
	Die("device horizontal motion quantum must be 1, got '$hor'")
	if ($hor != 1);
    }

    if (exists($desc{'vert'}))
    {
	my $vert=$desc{'vert'};
	Die("device vertical motion quantum must be 1, got '$vert'")
	if ($vert != 1);
    }

    my ($res,$ss)=($desc{'res'},$desc{'sizescale'});
    Die("device resolution must be a multiple of 72*sizescale, got"
    . " '$res' ('sizescale'=$ss)") if (($res % ($ss * 72)) != 0);
}

sub rad  { $_[0]*3.14159/180 }

my $InPicRotate=0;

sub do_x
{
    my $l=shift;
    my ($xcmd,@xprm)=split(' ',$l);
    $xcmd=substr($xcmd,0,1);

    if ($xcmd eq 'T')
    {
	Warn("expecting a PDF pipe (got $xprm[0])")
	if $xprm[0] ne substr($devnm,3);
    }
    elsif ($xcmd eq 'f')	# Register Font
    {
	$xprm[1]="${Foundry}-$xprm[1]" if $Foundry ne '';
	LoadFont($xprm[0],$xprm[1]);
    }
    elsif ($xcmd eq 'F')	# Source File (for errors)
    {
	$env{SourceFile}=$xprm[0];
    }
    elsif ($xcmd eq 'H')	# FontHT
    {
	$xprm[0]/=$unitwidth;
	$xprm[0]=0 if $xprm[0] == $cftsz;
	$env{FontHT}=$xprm[0];
	MakeMatrix();
    }
    elsif ($xcmd eq 'S')	# FontSlant
    {
	$env{FontSlant}=$xprm[0];
	MakeMatrix();
    }
    elsif ($xcmd eq 'i')	# Initialise
    {
	if ($objct == 0)
	{
	    $objct++;
	    @defaultmb=@mediabox;
	    BuildObj($objct,{'Pages' => BuildObj($objct+1,
		{'Kids' => [],
		    'Count' => 0,
		    'Type' => '/Pages',
		    'Rotate' => $rot,
		    'MediaBox' => \@defaultmb,
		    'Resources' => {'Font' => {},
		    'ProcSet' => ['/PDF', '/Text', '/ImageB', '/ImageC', '/ImageI']}
		}
	    ),
	    'Type' =>  '/Catalog'});

	    $cat=$obj[$objct]->{DATA};
	    $objct++;
	    $pages=$obj[2]->{DATA};
	    Put("%PDF-1.$PDFver\n\x25\xe2\xe3\xcf\xd3\n");
	}
    }
    elsif ($xcmd eq 'X')
    {
	# There could be extended args
	do
	{{
	    LoadAhead(1);
	    if (substr($ahead[0],0,1) eq '+')
	    {
		$l.="\n".substr($ahead[0],1);
		shift(@ahead);
	    }
	}} until $#ahead==0;

	($xcmd,@xprm)=split(' ',$l);
	$xcmd=substr($xcmd,0,1);

	if ($xprm[0]=~m/^(.+:)(.+)/)
	{
	    splice(@xprm,1,0,$2);
	    $xprm[0]=$1;
	}

	my $par=join(' ',@xprm[1..$#xprm]);

	if ($xprm[0] eq 'ps:')
	{
	    if ($xprm[1] eq 'invis')
	    {
		$suppress=1;
	    }
	    elsif ($xprm[1] eq 'endinvis')
	    {
		$suppress=0;
	    }
	    elsif ($par=~m/exec gsave currentpoint 2 copy translate (.+) rotate neg exch neg exch translate/)
	    {
		# This is added by gpic to rotate a single object

		my $theta=-rad($1);

		IsGraphic();
		my ($curangle,$hyp)=RtoP($xpos,GraphY($ypos));
		my ($x,$y)=PtoR($theta+$curangle,$hyp);
		my ($tx, $ty) = ($xpos - $x, GraphY($ypos) - $y);
		if ($frot) {
		    ($tx, $ty) = ($tx *  sin($theta) + $ty * -cos($theta),
				  $tx * -cos($theta) + $ty * -sin($theta));
		}
		$stream.="q\n".sprintf("%.3f %.3f %.3f %.3f %.3f %.3f cm",cos($theta),sin($theta),-sin($theta),cos($theta),$tx,$ty)."\n";
		$InPicRotate=1;
	    }
	    elsif ($par=~m/exec grestore/ and $InPicRotate)
	    {
		IsGraphic();
		$stream.="Q\n";
		$InPicRotate=0;
	    }
	    elsif ($par=~m/exec.*? (\d) setlinejoin/)
	    {
		IsGraphic();
		$linejoin=$1;
		$stream.="$linejoin j\n";
	    }
	    if ($par=~m/exec.*? (\d) setlinecap/)
	    {
		IsGraphic();
		$linecap=$1;
		$stream.="$linecap J\n";
	    }
	    elsif ($par=~m/exec %%%%PAUSE/i and !$noslide)
	    {
		my $trans='BLOCK';

		if ($firstpause)
		{
		    $trans='PAGE';
		    $firstpause=0;
		}
		MakeXO();
		NewPage($trans);
		$present=1;
	    }
	    elsif ($par=~m/exec %%%%BEGINONCE/)
	    {
		if ($noslide)
		{
		    $suppress=1;
		}
		else
		{
		    my $trans='BLOCK';

		    if ($firstpause)
		    {
			$trans='PAGE';
			$firstpause=0;
		    }
		    MakeXO();
		    NewPage($trans);
		    $present=1;
		}
	    }
	    elsif ($par=~m/exec %%%%ENDONCE/)
	    {
		if ($noslide)
		{
		    $suppress=0;
		}
		else
		{
		    MakeXO();
		    NewPage('BLOCK');
		    $present=1;
		    pop(@XOstream);
		}
	    }
	    elsif ($par=~m/\[(.+) pdfmark/)
	    {
		my $pdfmark=$1;
		$pdfmark=~s((\d{4,6}) u)(sprintf("%.1f",$1/$desc{sizescale}))eg;
		$pdfmark=~s(\\\[u00(..)\])(chr(hex($1)))eg;
		$pdfmark=~s/\\n/\n/g;

		if ($pdfmark=~m/\/(\w+) \((.+)\) \/DOCINFO\s*$/s)
		{
		    my $k=$1;
		    $info{$k}='('.utf16($2,1,-1).')' if $k ne 'Producer';
		}
		elsif ($pdfmark=~m/(.+) \/DOCVIEW\s*$/)
		{
		    my @xwds=split(' ',"<< $1 >>");
		    my $docview=ParsePDFValue(\@xwds);

		    foreach my $k (sort keys %{$docview})
		    {
			$cat->{$k}=$docview->{$k} if !exists($cat->{$k});
		    }
		}
		elsif ($pdfmark=~m/\/Dest (\/.+?)( \/View .+) \/DEST\s*$/)
		{
		    my (@d)=($1,$2);
		    my @xwds=split(' ',"<< $d[1] >>");
		    my $dest=ParsePDFValue(\@xwds);
		    $dest->{Dest}=UTFName($d[0]);
		    $dest->{View}->[1]=GraphY($dest->{View}->[1]*-1);
		    unshift(@{$dest->{View}},"$cpageno 0 R");

		    if (!defined($dests))
		    {
			$cat->{Dests}=BuildObj(++$objct,{});
			$dests=$obj[$objct]->{DATA};
		    }

		    my $k=substr($dest->{Dest},1);
		    $dests->{$k}=$dest->{View};
		}
		elsif ($pdfmark=~m/(.+) \/ANN\s*$/)
		{
		    my $l=$1;
		    $l=~s/Color/C/;
		    $l=~s/Action/A/;
		    $l=~s/Title/T/;
		    $l=~s'/Subtype /URI'/S /URI';
		    my @xwds=split(' ',"<< $l >>");
		    my $annotno=BuildObj(++$objct,ParsePDFValue(\@xwds));
		    my $annot=$obj[$objct];
		    $annot->{DATA}->{Type}='/Annot';
		    FixRect($annot->{DATA}->{Rect}); # Y origin to ll
		    FixPDFColour($annot->{DATA});
		    $annot->{DATA}->{Dest}=UTFName($annot->{DATA}->{Dest}) if exists($annot->{DATA}->{Dest});
		    $annot->{DATA}->{A}->{URI}=URIName($annot->{DATA}->{A}->{URI}) if exists($annot->{DATA}->{A}->{URI});
		    push(@PageAnnots,$annotno);
		}
		elsif ($pdfmark=~m/(.+) \/OUT\s*$/)
		{
		    my $t=$1;
		    $t=~s/\\\) /\\\\\) /g;
		    $t=~s/\\e/\\\\/g;
		    $t=~m/^\/Dest (.+?) \/Title \((.*)(\).*)/;
		    my ($d,$title,$post)=($1,$2,$3);
		    $title=utf16($title);

		    $title="\\134" if $title eq "\\";
		    my @xwds=split(' ',"<< \/Title ($title$post >>");
		    my $out=ParsePDFValue(\@xwds);
		    $out->{Dest}=UTFName($d);

		    my $this=[$out,[]];

		    if (exists($out->{Level}))
		    {
			my $lev=abs($out->{Level});
			my $levsgn=sgn($out->{Level});
			delete($out->{Level});

			if ($lev > $thislev)
			{
			    my $thisoutlev=$curoutlev->[$#{$curoutlev}]->[1];
			    $thisoutlev->[0]=[0,$curoutlev,0,$levsgn];
			    $curoutlev=$thisoutlev;
			    $curoutlevno=$#{$curoutlev};
			    $thislev++;
			}
			elsif ($lev < $thislev)
			{
			    my $openct=$curoutlev->[0]->[2];

			    while ($thislev > $lev)
			    {
				my $nxtoutlev=$curoutlev->[0]->[1];
				$nxtoutlev->[0]->[2]+=$openct if $curoutlev->[0]->[3]==1;
				$openct=0 if $nxtoutlev->[0]->[3]==-1;
				$curoutlev=$nxtoutlev;
				$thislev--;
			    }

			    $curoutlevno=$#{$curoutlev};
			}

#			push(@{$curoutlev},$this);
			splice(@{$curoutlev},++$curoutlevno,0,$this);
			$curoutlev->[0]->[2]++;
		    }
		    else
		    {
			# This code supports old pdfmark.tmac, unused by pdf.tmac
			while ($curoutlev->[0]->[0] == 0 and defined($curoutlev->[0]->[1]))
			{
			    $curoutlev=$curoutlev->[0]->[1];
			}

			$curoutlev->[0]->[0]--;
			$curoutlev->[0]->[2]++;
			push(@{$curoutlev},$this);


			if (exists($out->{Count}) and $out->{Count} != 0)
			{
			    push(@{$this->[1]},[abs($out->{Count}),$curoutlev,0,sgn($out->{Count})]);
			    $curoutlev=$this->[1];

			    if ($out->{Count} > 0)
			    {
				my $p=$curoutlev;

				while (defined($p))
				{
				    $p->[0]->[2]+=$out->{Count};
				    $p=$p->[0]->[1];
				}
			    }
			}
		    }
		}
	    }
	}
	elsif (lc($xprm[0]) eq 'pdf:')
	{
	    if (lc($xprm[1]) eq 'import')
	    {
		my $fil=$xprm[2];
		my $llx=$xprm[3];
		my $lly=$xprm[4];
		my $urx=$xprm[5];
		my $ury=$xprm[6];
		my $wid=GetPoints($xprm[7]);
		my $hgt=GetPoints($xprm[8])||-1;
		my $mat=[1,0,0,1,0,0];

		if (!exists($incfil{$fil}))
		{
		    if ($fil=~m/\.pdf$/)
		    {
			$incfil{$fil}=LoadPDF($fil,$mat,$wid,$hgt,"import");
		    }
		    elsif ($fil=~m/\.swf$/)
		    {
			my $xscale=$wid/($urx-$llx+1);
			my $yscale=($hgt<=0)?$xscale:($hgt/($ury-$lly+1));
			$hgt=($ury-$lly+1)*$yscale;

			if ($rot)
			{
			    $mat->[3]=$xscale;
			    $mat->[0]=$yscale;
			}
			else
			{
			    $mat->[0]=$xscale;
			    $mat->[3]=$yscale;
			}

			$incfil{$fil}=LoadSWF($fil,[$llx,$lly,$urx,$ury],$mat);
		    }
		    else
		    {
			Warn("unrecognized 'import' file type '$fil'");
			return undef;
		    }
		}

		if (defined($incfil{$fil}))
		{
		    IsGraphic();
		    if ($fil=~m/\.pdf$/)
		    {
			my $bbox=$incfil{$fil}->[1];
			my $xscale=d3($wid/($bbox->[2]-$bbox->[0]+1));
			my $yscale=d3(($hgt<=0)?$xscale:($hgt/($bbox->[3]-$bbox->[1]+1)));
			$wid=($bbox->[2]-$bbox->[0])*$xscale;
			$hgt=($bbox->[3]-$bbox->[1])*$yscale;
			$ypos+=$hgt;
			$stream.="q $xscale 0 0 $yscale ".PutXY($xpos,$ypos)." cm";
			$stream.=" 0 1 -1 0 0 0 cm" if $rot;
			$stream.=" /$incfil{$fil}->[0] Do Q\n";
		    }
		    elsif ($fil=~m/\.swf$/)
		    {
			$stream.=PutXY($xpos,$ypos)." m /$incfil{$fil} Do\n";
		    }
		}
	    }
	    elsif (lc($xprm[1]) eq 'pdfpic')
	    {
		my $fil=$xprm[2];
		my $flag=uc($xprm[3]||'-L');
		my $wid=GetPoints($xprm[4])||-1;
		my $hgt=GetPoints($xprm[5]||-1);
		my $ll=GetPoints($xprm[6]||0);
		my $mat=[1,0,0,1,0,0];
		my $imgtype='PDF';
		my $info;
		my $image;

		my ($FD,$FDnm)=OpenInc($fil);

		if (!defined($FD))
		{
		    Warn("failed to open image file '$FDnm'");
		    return;
		}

		if (!exists($incfil{$fil}))
		{
		    if ($gotexif and $FDnm!~m/\.pdf$/i)
		    {
			binmode $FD;

			$image = Image::Magick->new;
			my $x = $image->Read(file => $FD);
			Warn("Image '$FDnm': $x"), return if "$x";
			$imgtype=$image->Get('magick');
			$info->{ImageWidth}=$image->Get('width');
			$info->{ImageHeight}=$image->Get('height');
			$info->{ColorComponents}=
			    ($image->Get('colorspace') eq 'Gray')?1:3;
		    }
		    else
		    {
			my $dim=`( identify $FDnm 2>/dev/null || file $FDnm )`;
			$dim=~m/(?:(?:[,=A-Z]|JP2) (?<w>\d+)\s*x\s*(?<h>\d+))|(?:height=(?<h>\d+).+width=(?<w>\d+))/;

			$info->{ImageWidth}=$+{w};
			$info->{ImageHeight}=$+{h};

			if ($dim=~m/JPEG \d+x|JFIF/)
			{
			    $imgtype='JPEG';
			    $info->{ColorComponents}=3;

			    if ($dim=~m/Gray|components 1/)
			    {
				$info->{ColorComponents}=1;
			    }
			}
			elsif ($dim=~m/JP2 \d+x/)
			{
			    $imgtype='JP2';
			}
		    }

		    if ($imgtype eq 'PDF')
		    {
			$incfil{$fil}=LoadPDF($FD,$FDnm,$mat,$wid,$hgt,"pdfpic");
		    }
		    elsif ($imgtype eq 'JPEG')
		    {
			$incfil{$fil}=LoadJPEG($FD,$FDnm,$info);
		    }
		    elsif ($imgtype eq 'JP2')
		    {
			$incfil{$fil}=LoadJP2($FD,$FDnm,$info);
		    }
		    else
		    {
			$incfil{$fil}=LoadMagick($image,$FDnm,$info);
		    }

		    return if !defined($incfil{$fil});
		    $incfil{$fil}->[2]=$imgtype;
		}

		if (defined($incfil{$fil}))
		{
		    IsGraphic();
		    my $bbox=$incfil{$fil}->[1];
		    $imgtype=$incfil{$fil}->[2];
		    Warn("Failed to extract width x height for '$FDnm'"),return if !defined($bbox->[2]) or !defined($bbox->[3]);
		    $wid=($bbox->[2]-$bbox->[0]) if $wid <= 0;
		    my $xscale=d3($wid/($bbox->[2]-$bbox->[0]));
		    my $yscale=d3(($hgt<=0)?$xscale:($hgt/($bbox->[3]-$bbox->[1])));
		    $xscale=($wid<=0)?$yscale:$xscale;
		    $xscale=$yscale if $yscale < $xscale;
		    $yscale=$xscale if $xscale < $yscale;
		    $wid=($bbox->[2]-$bbox->[0])*$xscale;
		    $hgt=($bbox->[3]-$bbox->[1])*$yscale;

		    if ($flag eq '-C' and $ll > $wid)
		    {
			$xpos+=int(($ll-$wid)/2);
		    }
		    elsif ($flag eq '-R' and $ll > $wid)
		    {
			$xpos+=$ll-$wid;
		    }

		    if ($imgtype ne 'PDF')
		    {
			if ($rot)
			{
			    $xscale*=$bbox->[3];
			    $yscale*=$bbox->[2];
			}
			else
			{
			    $xscale*=$bbox->[2];
			    $yscale*=$bbox->[3];
			}

		    }

		    $ypos+=$hgt;
		    $stream.="q $xscale 0 0 $yscale ".PutXY($xpos,$ypos)." cm";
		    $stream.=" 0 1 -1 0 0 0 cm" if $rot;
		    $stream.=" /$incfil{$fil}->[0] Do Q\n";
		}
	    }
	    elsif (lc($xprm[1]) eq 'xrev')
	    {
		PutLine(0);
		$xrev=!$xrev;
	    }
	    elsif (lc($xprm[1]) eq 'markstart')
	    {
		$mark={'rst' => ($xprm[2]+$xprm[4])/$unitwidth, 'rsb' => ($xprm[3]-$xprm[4])/$unitwidth, 'xpos' => $xpos-($xprm[4]/$unitwidth),
		    'ypos' => $ypos, 'lead' => $xprm[4]/$unitwidth, 'pdfmark' => join(' ',@xprm[5..$#xprm])};
	    }
	    elsif (lc($xprm[1]) eq 'markend')
	    {
		PutHotSpot($xpos) if defined($mark);
		$mark=undef;
	    }
	    elsif (lc($xprm[1]) eq 'marksuspend' and $mark)
	    {
		$suspendmark=$mark;
		$mark=undef;
	    }
	    elsif (lc($xprm[1]) eq 'markrestart' and $suspendmark)
	    {
		$mark=$suspendmark;
		$suspendmark=undef;
	    }
	    elsif (lc($xprm[1]) eq 'pagename')
	    {
		if ($pginsert > -1)
		{
		    $pgnames{$xprm[2]}=$pages->{Kids}->[$pginsert];
		}
		else
		{
		    $pgnames{$xprm[2]}='top';
		}
	    }
	    elsif (lc($xprm[1]) eq 'switchtopage')
	    {
		my $ba=$xprm[2];
		my $want=$xprm[3];

		if ($pginsert > -1)
		{
		    if (!defined($want) or $want eq '')
		    {
			# no before/after
			$want=$ba;
			$ba='before';
		    }

		    if (!defined($ba) or $ba eq '' or $want eq 'bottom')
		    {
			$pginsert=$#{$pages->{Kids}};
		    }
		    elsif ($want eq 'top')
		    {
			$pginsert=-1;
		    }
		    else
		    {
			if (exists($pgnames{$want}))
			{
			    my $ref=$pgnames{$want};

			    if ($ref eq 'top')
			    {
				$pginsert=-1;
			    }
			    else
			    {
				FIND: while (1)
				{
				    foreach my $j (0..$#{$pages->{Kids}})
				    {
					if ($ref eq $pages->{Kids}->[$j])
					{
					    if ($ba eq 'before')
					    {
						$pginsert=$j-1;
						last FIND;
					    }
					    elsif ($ba eq 'after')
					    {
						$pginsert=$j;
						last FIND;
					    }
					    else
					    {
						# XXX: indentation wince
						Warn(
						    "expected 'switchtopage' parameter to be one of"
						    . "'top|bottom|before|after', got '$ba'");
						last FIND;
					    }
					}

				    }

				    Warn("cannot find page ref '$ref'");
				    last FIND

				}
			    }
			}
			else
			{
			    Warn("cannot find page named '$want'");
			}
		    }

		    if ($pginsert < 0)
		    {
			($curoutlev,$curoutlevno,$thislev)=(\@outlev,0,1);
		    }
		    else
		    {
			($curoutlev,$curoutlevno,$thislev)=(@{$outlines[$pginsert]});
#			$curoutlevno--;
		    }
		}
	    }
	    elsif (lc($xprm[1]) eq 'transition' and !$noslide)
	    {
		if (uc($xprm[2]) eq 'PAGE' or uc($xprm[2] eq 'SLIDE'))
		{
		    $transition->{PAGE}->{S}='/'.ucfirst($xprm[3]) if $xprm[3] and $xprm[3] ne '.';
		    $transition->{PAGE}->{D}=$xprm[4] if $xprm[4] and $xprm[4] ne '.';
		    $transition->{PAGE}->{Dm}='/'.$xprm[5] if $xprm[5] and $xprm[5] ne '.';
		    $transition->{PAGE}->{M}='/'.$xprm[6] if $xprm[6] and $xprm[6] ne '.';
		    $xprm[7]='/None' if $xprm[7] and uc($xprm[7]) eq 'NONE';
		    $transition->{PAGE}->{Di}=$xprm[7] if $xprm[7] and $xprm[7] ne '.';
		    $transition->{PAGE}->{SS}=$xprm[8] if $xprm[8] and $xprm[8] ne '.';
		    $transition->{PAGE}->{B}=$xprm[9] if $xprm[9] and $xprm[9] ne '.';
		}
		elsif (uc($xprm[2]) eq 'BLOCK')
		{
		    $transition->{BLOCK}->{S}='/'.ucfirst($xprm[3]) if $xprm[3] and $xprm[3] ne '.';
		    $transition->{BLOCK}->{D}=$xprm[4] if $xprm[4] and $xprm[4] ne '.';
		    $transition->{BLOCK}->{Dm}='/'.$xprm[5] if $xprm[5] and $xprm[5] ne '.';
		    $transition->{BLOCK}->{M}='/'.$xprm[6] if $xprm[6] and $xprm[6] ne '.';
		    $xprm[7]='/None' if $xprm[7] and uc($xprm[7]) eq 'NONE';
		    $transition->{BLOCK}->{Di}=$xprm[7] if $xprm[7] and $xprm[7] ne '.';
		    $transition->{BLOCK}->{SS}=$xprm[8] if $xprm[8] and $xprm[8] ne '.';
		    $transition->{BLOCK}->{B}=$xprm[9] if $xprm[9] and $xprm[9] ne '.';
		}

		$present=1;
	    }
	    elsif (lc($xprm[1]) eq 'background')
	    {
		splice(@xprm,0,2);
		my $type=shift(@xprm);
#		print STDERR "ypos=$ypos\n";

		if (lc($type) eq 'off')
		{
		    my $sptr=$#bgstack;
		    if ($sptr > -1)
		    {
			if ($sptr == 0 and $bgstack[0]->[0] & 4)
			{
			    pop(@bgstack);
			}
			else
			{
			    $bgstack[$sptr]->[5]=GraphY($ypos);
			    $bgbox=DrawBox(pop(@bgstack)).$bgbox;
			}
		    }
		}
		elsif (lc($type) eq 'footnote')
		{
		    my $t=GetPoints($xprm[0]);
		    $boxmax=($t<0)?abs($t):GraphY($t);
		}
		else
		{
		    my $bgtype=0;

		    foreach (@xprm)
		    {
			$_=GetPoints($_);
		    }

		    $bgtype|=2 if $type=~m/box/i;
		    $bgtype|=1 if $type=~m/fill/i;
		    $bgtype|=4 if $type=~m/page/i;
		    $bgtype=5 if $bgtype==4;
		    my $bgwt=$xprm[4];
		    $bgwt=$xprm[0] if !defined($bgwt) and $#xprm == 0;
		    my (@bg)=(@xprm);
		    my $bg=\@bg;

		    if (!defined($bg[3]) or $bgtype & 4)
		    {
			$bg=undef;
		    }
		    else
		    {
			FixRect($bg);
		    }

		    if ($bgtype)
		    {
			if ($bgtype & 4)
			{
			    shift(@bgstack) if $#bgstack >= 0 and $bgstack[0]->[0] & 4;
			    unshift(@bgstack,[$bgtype,$strkcol,$fillcol,$bg,GraphY($ypos),GraphY($bg[3]||0),$bgwt || 0.4]);
			}
			else
			{
			    push(@bgstack,[$bgtype,$strkcol,$fillcol,$bg,GraphY($ypos),GraphY($bg[3]||0),$bgwt || 0.4]);
			}
		    }
		}
	    }
	    elsif (lc($xprm[1]) eq 'pagenumbering')
	    {
		# 2=type of [D=decimal,R=Roman,r=roman,A=Alpha (uppercase),a=alpha (lowercase)
		# 3=prefix label
		# 4=start number

		my ($S,$P,$St);

		$xprm[2]='' if !$xprm[2] or $xprm[2] eq '.';
		$xprm[3]='' if defined($xprm[3]) and $xprm[3] eq '.';

		if ($xprm[2] and index('DRrAa',substr($xprm[2],0,1)) == -1)
		{
		    Warn("Page numbering type '$xprm[2]' is not recognised");
		}
		else
		{
		    $S=substr($xprm[2],0,1) if $xprm[2];
		    $P=$xprm[3];
		    $St=$xprm[4] if length($xprm[4]);

		    if (!defined($S) and !length($P))
		    {
			$P=' ';
		    }

		    if ($St and $St!~m/^-?\d+$/)
		    {
			Warn("Page numbering start '$St' must be numeric");
			return;
		    }

		    $cat->{PageLabels}={Nums => []} if !exists($cat->{PageLabels});

		    my $label={};
		    $label->{S} = "/$S" if $S;
		    $label->{P} = "($P)" if length($P);
		    $label->{St} = $St if length($St);

		    $#PageLabel=$pginsert if $pginsert > $#PageLabel;
		    splice(@PageLabel,$pginsert,0,$label);
		}
	    }

	}
	elsif (lc(substr($xprm[0],0,9)) eq 'papersize')
	{
	    if (!($xprm[1] and $xprm[1] eq 'tmac' and $fpsz))
	    {
		my ($px,$py)=split(',',substr($xprm[0],10));
		$px=GetPoints($px);
		$py=GetPoints($py);
		@mediabox=(0,0,$px,$py);
		my @mb=@mediabox;
		$matrixchg=1;
		$custompaper=1;
		$cpage->{MediaBox}=\@mb;
	    }
	}
    }
}

sub URIName
{
    my $s=shift;

    $s=Clean($s);
    $s=~s/\\\[u((?i)D[89AB]\p{AHex}{2})\] # High surrogate in range 0xD800–0xDBFF
	  \\\[u((?i)D[CDEF]\p{AHex}{2})\] #  Low surrogate in range 0xDC00–0xDFFF
	  /chr( ((hex($1) - 0xD800) * 0x400) + (hex($2) - 0xDC00) + 0x10000 )/xge;
    $s=~s/\\\[u(\p{AHex}{4})]/chr hex $1/ge;

    return(join '', map {(m/[-\w.~_]/)?chr($_):'%'.sprintf("%02X", $_)} unpack "C*", encode('utf8',$s));
}

sub Clean
{
    my $p=shift;

    $p=~s/\\c?$//g;
    $p=~s/\\[eE]/\\/g;
    $p=~s/\\[ 0~t]/ /g;
    $p=~s/\\[,!"#\$%&’.0:?{}ˆ_‘|^prud]//g;
    $p=~s/\\'/\\[aa]/g;
    $p=~s/\\`/\\[ga]/g;
    $p=~s/\\_/\\[ul]/g;
    $p=~s/\\-/-/g;

    $p=~s/\\[Oz].//g;
    $p=~s/\\[ABbDHlLoRSvwXZ]$parcln//g;
    $p=~s/\\[FfgkMmnVY]$parclntyp//g;
    $p=~s/\\[hs][-+]?$parclntyp//g;

    $p=~s/\\\((\w\w)/\\\[$1\]/g;	# convert \(xx to \[xx]

    return $p;
}

sub utf16
{
    my $p=Clean(shift);
    my $label=shift;

    $p=~s/\\\(rs|\\\[rs\]/\\E/g;
    $p=~s/\\\[(.*?)\]/FindChr($1,0)/eg;
    $p=~s/\\C($parcln)/FindChr($1,1)/eg;
#    $p=~s/\\\((..)/FindChr($1)/eg;
    $p=~s/\\N($parcln)/FindChr($1,1,1)/eg;

    if ($p =~ /[^[:ascii:]]/)
    {
	$p = join '', map sprintf("\\%o", $_),
	     unpack "C*", encode('utf16', $p);
    }

    return($p) if $label;

    $p=~s/(?<!\\)\(/\\\(/g;
    $p=~s/(?<!\\)\)/\\\)/g;
    $p=~s/\\[eE]/\\\\/g;

    return($p);
}

sub FindChr
{
    my $ch=shift;
    my $subsflg=shift;
    my $cn=shift;

    return('') if !defined($ch);
    $ch=substr($ch,1,-1) if $subsflg;
    $ch=$thisfnt->{NO}->[$ch] if defined($cn);
    return('') if !defined($ch);
    return pack('U',hex($1)) if $ch=~m/^u([0-9A-F]{4,5})$/;

    if (exists($thisfnt->{NAM}->{$ch}))
    {
	if ($thisfnt->{NAM}->{$ch}->[PSNAME]=~m/\\u(?:ni)?([0-9A-F]{4,5})/)
	{
	    return pack('U',hex($1));
	}
	elsif (defined($thisfnt->{NAM}->{$ch}->[UNICODE]))
	{
	    return pack('U',hex($thisfnt->{NAM}->{$ch}->[UNICODE]))
	}
    }
    elsif ($ch=~m/^\w+$/)       # ligature not in font i.e. \(ff
    {
	return $ch;
    }

    Warn("Can't convert '$ch' to unicode");

    return('');
}

sub UTFName
{
    my $s=shift;
    my $r='';

    $s=substr($s,1);
    my $s1=$s;
    my $s2=utf16($s1,1);
#    return "/".MakeLabel((substr($s2,0,1) eq '/')?$s:$s2);
    my $s3='/'.join '', map { MakeLabel($_) } unpack('C*',$s2);
    return $s3;

}

sub MakeLabel
{
    my $c=chr(shift);
    return($c) if ($c=~m/[\w\d:]/);
    return(sprintf("#%02x",ord($c)));
}

sub FixPDFColour
{
    my $o=shift;
    my $a=$o->{C};
    my @r=();
    my $c=$a->[0];

    if ($#{$a}==3)
    {
	if ($c > 1)
	{
	    foreach my $j (0..2)
	    {
		push(@r,sprintf("%1.3f",$a->[$j]/0xffff));
	    }

	    $o->{C}=\@r;
	}
    }
    elsif (substr($c,0,1) eq '#')
    {
	if (length($c) == 7)
	{
	    foreach my $j (0..2)
	    {
		push(@r,sprintf("%1.3f",hex(substr($c,$j*2+1,2))/0xff));
	    }

	    $o->{C}=\@r;
	}
	elsif (length($c) == 14)
	{
	    foreach my $j (0..2)
	    {
		push(@r,sprintf("%1.3f",hex(substr($c,$j*4+2,4))/0xffff));
	    }

	    $o->{C}=\@r;
	}
    }
}

sub PutHotSpot
{
    my $endx=shift;
    my $l=$mark->{pdfmark};
    $l=~s/Color/C/;
    $l=~s/Action/A/;
    $l=~s'/Subtype /URI'/S /URI';
    $l=~s(\\\[u00(..)\])(chr(hex($1)))eg;
    my @xwds=split(' ',"<< $l >>");
    my $annotno=BuildObj(++$objct,ParsePDFValue(\@xwds));
    my $annot=$obj[$objct];
    $annot->{DATA}->{Type}='/Annot';
    $annot->{DATA}->{Rect}=[$mark->{xpos},$mark->{ypos}-$mark->{rsb},$endx+$mark->{lead},$mark->{ypos}-$mark->{rst}];
    FixPDFColour($annot->{DATA});
    FixRect($annot->{DATA}->{Rect}); # Y origin to ll
    $annot->{DATA}->{Dest}=UTFName($annot->{DATA}->{Dest}) if exists($annot->{DATA}->{Dest});
    $annot->{DATA}->{A}->{URI}=URIName($annot->{DATA}->{A}->{URI}) if exists($annot->{DATA}->{A});
    push(@PageAnnots,$annotno);
}

sub sgn
{
    return(1) if $_[0] > 0;
    return(-1) if $_[0] < 0;
    return(0);
}

sub FixRect
{
    my $rect=shift;

    return if !defined($rect);
    $rect->[1]=GraphY($rect->[1]);
    $rect->[3]=GraphY($rect->[3]);

    if ($rot)
    {
	($rect->[0],$rect->[1])=Rotate($rect->[0],$rect->[1]);
	($rect->[2],$rect->[3])=Rotate($rect->[2],$rect->[3]);
    }
}

sub Rotate
{
    my ($tx,$ty)=(@_);
    my $theta=rad($rot);

    ($tx,$ty)=(d3($tx * cos(-$theta) - $ty * sin(-$theta)),
	       d3($tx * sin( $theta) + $ty * cos( $theta)));
    return($tx,$ty);
}

sub GetPoints
{
    my $val=shift;

    $val=ToPoints($1,$2) if ($val and $val=~m/(-?[\d.]+)([cipnz])/);

    return $val;
}

# Although the PDF reference mentions XObject/Form as a way of
# incorporating an external PDF page into the current PDF, it seems not
# to work with any current PDF reader (although I am told (by Leonard
# Rosenthol, who helped author the PDF ISO standard) that Acroread 9
# does support it, empirical observation shows otherwise!!).  So... do
# it the hard way - full PDF parser and merge required objects!!!

# sub BuildRef
# {
#       my $fil=shift;
#       my $bbox=shift;
#       my $mat=shift;
#       my $wid=($bbox->[2]-$bbox->[0])*$mat->[0];
#       my $hgt=($bbox->[3]-$bbox->[1])*$mat->[3];
#
#       if (!open(PDF,"<$fil"))
#       {
#               Warn("failed to open '$fil'");
#               return(undef);
#       }
#
#       my (@f)=(<PDF>);
#
#       close(PDF);
#
#       $objct++;
#       my $xonm="XO$objct";
#
#       $pages->{'Resources'}->{'XObject'}->{$xonm}=BuildObj($objct,{'Type' => '/XObject',
#                                                                   'Subtype' => '/Form',
#                                                                   'BBox' => $bbox,
#                                                                   'Matrix' => $mat,
#                                                                   'Resources' => $pages->{'Resources'},
#                                                                   'Ref' => {'Page' => '1',
#                                                                               'F' => BuildObj($objct+1,{'Type' => '/Filespec',
#                                                                                                         'F' => "($fil)",
#                                                                                                         'EF' => {'F' => BuildObj($objct+2,{'Type' => '/EmbeddedFile'})}
#                                                                               })
#                                                                   }
#                                                               });
#
#       $obj[$objct]->{STREAM}="q 1 0 0 1 0 0 cm
# q BT
# 1 0 0 1 0 0 Tm
# .5 g .5 G
# /F5 20 Tf
# (Proxy) Tj
# ET Q
# 0 0 m 72 0 l s
# Q\n";
#
# #     $obj[$objct]->{STREAM}=PutXY($xpos,$ypos)." m ".PutXY($xpos+$wid,$ypos)." l ".PutXY($xpos+$wid,$ypos+$hgt)." l ".PutXY($xpos,$ypos+$hgt)." l f\n";
#       $obj[$objct+2]->{STREAM}=join('',@f);
#       PutObj($objct);
#       PutObj($objct+1);
#       PutObj($objct+2);
#       $objct+=2;
#       return($xonm);
# }

sub LoadSWF
{
    my $fil=shift;
    my $bbox=shift;
    my $mat=shift;
    my $wid=($bbox->[2]-$bbox->[0])*$mat->[0];
    my $hgt=($bbox->[3]-$bbox->[1])*$mat->[3];
    my (@path)=split('/',$fil);
    my $node=pop(@path);

    if (!open(PDF,"<$fil"))
    {
	Warn("failed to open SWF '$fil'");
	return(undef);
    }

    my (@f)=(<PDF>);

    close(PDF);

    $objct++;
    my $xonm="XO$objct";

    $pages->{'Resources'}->{'XObject'}->{$xonm}=BuildObj($objct,{'Type' => '/XObject', 'BBox' => $bbox, 'Matrix' => $mat, 'FormType' => 1, 'Subtype' => '/Form', 'Length' => 0, 'Type' => "/XObject"});
    $obj[$objct]->{STREAM}='';
    PutObj($objct);
    $objct++;
    my $asset=BuildObj($objct,{'EF' => {'F' => BuildObj($objct+1,{})},
		       'F' => "($node)",
		       'Type' => '/Filespec',
		       'UF' => "($node)"});

    PutObj($objct);
    $objct++;
    $obj[$objct]->{STREAM}=join('',@f);
    PutObj($objct);
    $objct++;
    my $config=BuildObj($objct,{'Instances' => [BuildObj($objct+1,{'Params' => { 'Binding' => '/Background'}, 'Asset' => $asset})],
			'Subtype' => '/Flash'});

    PutObj($objct);
    $objct++;
    PutObj($objct);
    $objct++;

    my ($x,$y)=split(' ',PutXY($xpos,$ypos));

    push(@{$cpage->{Annots}},BuildObj($objct,{'RichMediaContent' => {'Subtype' => '/Flash', 'Configurations' => [$config], 'Assets' => {'Names' => [ "($node)", $asset ] }},
				      'P' => "$cpageno 0 R",
				      'RichMediaSettings' => { 'Deactivation' => { 'Condition' => '/PI',
					  'Type' => '/RichMediaDeactivation'},
				      'Activation' => { 'Condition' => '/PV',
					  'Type' => '/RichMediaActivation'}},
				      'F' => 68,
				      'Subtype' => '/RichMedia',
				      'Type' => '/Annot',
				      'Rect' => "[ $x $y ".($x+$wid)." ".($y+$hgt)." ]",
				      'Border' => [0,0,0]}));

    PutObj($objct);

    return $xonm;
}

sub OpenInc
{
    my $fn=shift;
    my $fnm=$fn;
    my $F;

    if (substr($fnm,0,1)  eq '/' or substr($fnm,1,1) eq ':') # dos
    {
	if (-r $fnm and open($F,"<$fnm"))
	{
	    return($F,$fnm);
	}
    }
    else
    {
	foreach my $dir (@idirs)
	{
	    $fnm="$dir/$fn";

	    if (-r "$fnm" and open($F,"<$fnm"))
	    {
		return($F,$fnm);
	    }
	}
    }

    return(undef,$fn);
}

sub LoadPDF
{
    my $PD=shift;
    my $PDnm=shift;
    my $mat=shift;
    my $wid=shift;
    my $hgt=shift;
    my $type=shift;
    my $pdf;
    my $pdftxt='';
    my $strmlen=0;
    my $curobj=-1;
    my $instream=0;
    my $cont;
    my $adj=0;
    my $keepsep=$/;

    seek($PD,0,0);
    my $hdr=<$PD>;

    if ($hdr!~m/^%PDF/)
    {
	Warn("'$PDnm' does not appear to be a pdf file");
	return undef;
    }

    $/="\r",$adj=1 if (length($hdr) > 10);

    while (<$PD>)
    {
	chomp;

	s/\n//;

	if (m/endstream(\s+.*)?$/)
	{
	    $instream=0;
	    $_="endstream";
	    $_.=$1 if defined($1)
	}

	next if $instream;

	if (m'/Length\s+(\d+)(\s+\d+\s+R)?')
	{
	    if (!defined($2))
	    {
		$strmlen=$1;
	    }
	    else
	    {
		$strmlen=0;
	    }
	}

	if (m'^(\d+) \d+ obj')
	{
	    $curobj=$1;
	    $pdf->[$curobj]->{OBJ}=undef;
	}

	if (m'stream\s*$' and ! m/^endstream/)
	{
	    if ($curobj > -1)
	    {
		$pdf->[$curobj]->{STREAMPOS}=[tell($PD)+$adj,$strmlen];
		seek($PD,$strmlen,1);
		$instream=1;
	    }
	    else
	    {
		Warn("parsing PDF '$PDnm' failed");
		return undef;
	    }
	}

	s/%.*?$//;
	$pdftxt.=$_.' ';
    }

    close($PD);

    open(PD,"<$PDnm");
#    $pdftxt=~s/\]/ \]/g;
    my (@pdfwds)=split(' ',$pdftxt);
    my $wd;
    my $root;
    my @ObjStm;

    while ($wd=nextwd(\@pdfwds),length($wd))
    {
	if ($wd=~m/\d+/ and defined($pdfwds[1]) and $pdfwds[1]=~m/^obj(.*)/)
	{
	    $curobj=$wd;
	    shift(@pdfwds); shift(@pdfwds);
	    unshift(@pdfwds,$1) if defined($1) and length($1);
	    $pdf->[$curobj]->{OBJ}=ParsePDFObj(\@pdfwds);
	    my $o=$pdf->[$curobj];

            push(@ObjStm,$curobj) if (ref($o->{OBJ}) eq 'HASH' and exists($o->{OBJ}->{Type}) and $o->{OBJ}->{Type} eq '/ObjStm');
	    $root=$curobj if ref($pdf->[$curobj]->{OBJ}) eq 'HASH' and exists($pdf->[$curobj]->{OBJ}->{Type}) and $pdf->[$curobj]->{OBJ}->{Type} eq '/XRef';
	}
	elsif ($wd eq 'trailer' and !exists($pdf->[0]->{OBJ}))
	{
	    $pdf->[0]->{OBJ}=ParsePDFObj(\@pdfwds);
	}
	else
	{
#		   print "Skip '$wd'\n";
	}
    }

    foreach my $ObjStm (@ObjStm)
    {
        LoadStream($pdf->[$ObjStm],$pdf);
        my $pos=$pdf->[$ObjStm]->{OBJ}->{First};
        my $s=$pdf->[$ObjStm]->{STREAM};
        $s=~s/\%.*?$//m;
        my @o=split(' ',substr($s,0,$pos));
        substr($s,0,$pos)='';
        push(@o,-1,length($s));

        for (my $j=0; $j<=$#o-2; $j+=2)
        {
            my @w=split(' ',substr($s,$o[$j+1],$o[$j+3]-$o[$j+1]));
            $pdf->[$o[$j]]->{OBJ}=ObjMerge($pdf->[$o[$j]]->{OBJ},ParsePDFObj(\@w));
        }

        $pdf->[$ObjStm]=undef;
    }

    $pdf->[0]=$pdf->[$root] if !defined($pdf->[0]);
    my $catalog=${$pdf->[0]->{OBJ}->{Root}};
    my $page=FindPage(1,$pdf);
    my $xobj=++$objct;

    # Load the streamas

    foreach my $o (@{$pdf})
    {
	if (exists($o->{STREAMPOS}) and !exists($o->{STREAM}))
	{
	    LoadStream($o,$pdf);
	}
    }

    close(PD);

    # Find BBox
    my $BBox;
    my $insmap={};

    foreach my $k (qw( ArtBox TrimBox BleedBox CropBox MediaBox ))
    {
	$BBox=FindKey($pdf,$page,$k);
	last if $BBox;
    }

    $BBox=[0,0,595,842] if !defined($BBox);

    $wid=($BBox->[2]-$BBox->[0]+1) if $wid==0;
    my $xscale=d3(abs($wid)/($BBox->[2]-$BBox->[0]+1));
    my $yscale=d3(($hgt<=0)?$xscale:(abs($hgt)/($BBox->[3]-$BBox->[1]+1)));
    $hgt=($BBox->[3]-$BBox->[1]+1)*$yscale;

    if ($type eq "import")
    {
	$mat->[0]=$xscale;
	$mat->[3]=$yscale;
    }

    # Find Resource

    my $res=FindKey($pdf,$page,'Resources');
    my $xonm="XO$xobj";

    # Map inserted objects to current PDF

    MapInsValue($pdf,$page,'',$insmap,$xobj,$pdf->[$page]->{OBJ});
    #
    #   Many PDFs include 'Resources' at the 'Page' level but if
    #   'Resources' is held at a higher level (i.e 'Pages') then we need
    #   to include its objects as well.
    #
    MapInsValue($pdf,$page,'',$insmap,$xobj,$res) if !exists($pdf->[$page]->{OBJ}->{Resources});

    # Copy Resources

    my %incres=%{$res};

    $incres{ProcSet}=['/PDF', '/Text', '/ImageB', '/ImageC', '/ImageI'];

    ($mat->[4],$mat->[5])=split(' ',PutXY($xpos,$ypos));
    $pages->{'Resources'}->{'XObject'}->{$xonm}=BuildObj($xobj,{'Type' => '/XObject', 'BBox' => $BBox, 'Name' => "/$xonm", 'FormType' => 1, 'Subtype' => '/Form', 'Length' => 0, 'Type' => "/XObject", 'Resources' => \%incres});

    if ($BBox->[0] != 0 or $BBox->[1] != 0)
    {
	my (@matrix)=(1,0,0,1,-$BBox->[0],-$BBox->[1]);
	$obj[$xobj]->{DATA}->{Matrix}=\@matrix;
    }

    BuildStream($xobj,$pdf,$pdf->[$page]->{OBJ}->{Contents});

    $/=$keepsep;
    return([$xonm,$BBox] );
}

sub LoadJPEG
{
    my $JP=shift;
    my $JPnm=shift;
    my $info=shift;
    my $BBox=[0,0,$info->{ImageWidth},$info->{ImageHeight}];

    local $/=undef;

    seek($JP,0,0);

    my $strm=<$JP>;
    close($JP);

    my $xobj=++$objct;
    my $xonm="XO$xobj";
    my $cs=($info->{ColorComponents}==1)?'/DeviceGray':'/DeviceRGB';
    $pages->{'Resources'}->{'XObject'}->{$xonm}=BuildObj($xobj,{'Type' => '/XObject', 'Width' => $BBox->[2], 'Height' => $BBox->[3], 'ColorSpace' => $cs, 'BitsPerComponent' => $info->{BitsPerSample}||8, 'Subtype' => '/Image', 'Length' => length($strm), 'Filter' => '/DCTDecode'});
    $obj[$xobj]->{STREAM}=$strm;
    return([$xonm,$BBox]);
}

sub LoadJP2
{
    my $JP=shift;
    my $JPnm=shift;
    my $info=shift;
    my $BBox=[0,0,$info->{ImageWidth},$info->{ImageHeight}];

    local $/=undef;

    seek($JP,0,0);

    my $strm=<$JP>;
    close($JP);

    my $xobj=++$objct;
    my $xonm="XO$xobj";
    $pages->{'Resources'}->{'XObject'}->{$xonm}=BuildObj($xobj,{'Type' => '/XObject', 'Width' => $BBox->[2], 'Height' => $BBox->[3],  'Subtype' => '/Image', 'Length' => length($strm), 'SMaskInData' => 1, 'Filter' => '/JPXDecode'});
    $obj[$xobj]->{STREAM}=$strm;
    return([$xonm,$BBox]);
}

sub LoadMagick
{
    my $image=shift;
    my $JPnm=shift;
    my $info=shift;

#     my $e=$image->Get('endian');
#     print STDERR "En: '$JPnm' $e\n";
#     $image->Set(endian => 'MSB') if $e eq 'Undefined';

    my $BPC;
    $BPC=$image->Get('depth');
    $BPC=$info->{BitDepth} if ($info and exists($info->{BitDepth}) and $info->{BitDepth} != $BPC);
#    $image->Set(depth => 8), $BPC=8 if $BPC==16;

    my $BBox=[0,0,$image->Get('width'),$image->Get('height')];
    my $alpha;

    if ($image->Get('matte'))
    {
	$alpha=$image->Clone();
	$alpha->Separate(channel => 'Alpha');
	my $v=$alpha->Get('version');
	$v=$1 if $v=~m/^ImageMagick (\d+\.\d+)/;
	$alpha->Negate(channel => 'All') if $v < 7;
	$alpha->Set(magick => 'gray');
    }

    my $cs=$image->Get('colorspace');
    $cs='RGB' if $cs eq 'sRGB';
    my $x = $image->Set(alpha => 'off', magick => $cs);
    Warn("Image '$JPnm': $x"), return if "$x";
    my @blobs = $image->ImageToBlob();
    Warn("Image '$JPnm': More than 1 image") if $#blobs > 0;
    $blobs[0]=pack('v*', unpack('n*', $blobs[0])) if $BPC==16;
    $blobs[0]=pack('V*', unpack('N*', $blobs[0])) if $BPC==32;

    my $xobj=++$objct;
    my $xonm="XO$xobj";

    if ($cs=~m/^(Gray|RGB|CMYK)$/)
    {
	$cs="/Device$cs";
    }
    else
    {
	Warn("Image '$JPnm' unknown ColourSpace '$cs'");
	return;
    }

    $pages->{'Resources'}->{'XObject'}->{$xonm}=BuildObj($xobj,{'Type' => '/XObject', 'Width' => $BBox->[2], 'Height' => $BBox->[3], 'ColorSpace' => $cs, 'BitsPerComponent' => $BPC, 'Subtype' => '/Image', 'Interpolate' => 'false', 'Length' => length($blobs[0])});
    $obj[$xobj]->{STREAM}=$blobs[0];

    if ($alpha)
    {
	$#blobs=-1;
	$alpha->Set(depth => 8);
	$BPC=8;
	@blobs = $alpha->ImageToBlob();
	$obj[$xobj]->{DATA}->{SMask}=BuildObj(++$objct,{'Type' => '/XObject', 'Width' => $BBox->[2], 'Height' => $BBox->[3], 'ColorSpace' => '/DeviceGray', 'BitsPerComponent' => $BPC, 'Subtype' => '/Image', 'Length' => length($blobs[0])});
	$obj[$objct]->{STREAM}=$blobs[0];
    }

    return([$xonm,$BBox]);
}

sub ObjMerge
{
    my $o1=shift;
    my $o2=shift;

    return $o1 if !defined($o2);
    return $o2 if !defined($o1);

    foreach my $k (keys %{$o2})
    {
        $o1->{$k}=$o2->{$k};
    }

    return $o1;
}

sub LoadStream
{
    my $o=shift;
    my $pdf=shift;
    my $l;

    $l=$o->{OBJ}->{Length} if exists($o->{OBJ}->{Length});

    $l=$pdf->[$$l]->{OBJ} if (defined($l) && ref($l) eq 'OBJREF');

    Die("unable to determine length of stream \@$o->{STREAMPOS}->[0]")
    if !defined($l);

    sysseek(PD,$o->{STREAMPOS}->[0],0);
    Warn("failed to read all of the stream")
    if $l != sysread(PD,$o->{STREAM},$l);

    if ($gotzlib and exists($o->{OBJ}->{'Filter'}) and $o->{OBJ}->{'Filter'} eq '/FlateDecode' and !exists($o->{OBJ}->{'DecodeParms'}))
    {
	$o->{STREAM}=Compress::Zlib::uncompress($o->{STREAM});
	delete($o->{OBJ }->{'Filter'});
    }
}

sub BuildStream
{
    my $xobj=shift;
    my $pdf=shift;
    my $val=shift;
    my $strm='';
    my $objs;
    my $refval=ref($val);

    if ($refval eq 'OBJREF')
    {
	push(@{$objs}, $val);
    }
    elsif ($refval eq 'ARRAY')
    {
	$objs=$val;
    }
    else
    {
	Warn("unexpected 'Contents'");
    }

    foreach my $o (@{$objs})
    {
	$strm.="\n" if $strm;
	$strm.=$pdf->[$$o]->{STREAM} if exists($pdf->[$$o]->{STREAM});
    }

    $obj[$xobj]->{STREAM}=$strm;
}

sub MapInsHash
{
    my $pdf=shift;
    my $o=shift;
    my $insmap=shift;
    my $parent=shift;
    my $val=shift;

    foreach my $k (sort keys(%{$val}))
    {
	MapInsValue($pdf,$o,$k,$insmap,$parent,$val->{$k}) if $k ne 'Contents';
    }
}

sub MapInsValue
{
    my $pdf=shift;
    my $o=shift;
    my $k=shift;
    my $insmap=shift;
    my $parent=shift;
    my $val=shift;
    my $refval=ref($val);

    if ($refval eq 'OBJREF')
    {
	if ($k ne 'Parent')
	{
	    if (!exists($insmap->{IMP}->{$$val}))
	    {
		$objct++;
		$insmap->{CUR}->{$objct}=$$val;
		$insmap->{IMP}->{$$val}=$objct;
		$obj[$objct]->{DATA}=$pdf->[$$val]->{OBJ};
		$obj[$objct]->{STREAM}=$pdf->[$$val]->{STREAM} if exists($pdf->[$$val]->{STREAM});
		MapInsValue($pdf,$$val,'',$insmap,$o,$pdf->[$$val]->{OBJ});
	    }

	    $$val=$insmap->{IMP}->{$$val};
	}
	else
	{
	    $$val=$parent;
	}
    }
    elsif ($refval eq 'ARRAY')
    {
	foreach my $v (@{$val})
	{
	    MapInsValue($pdf,$o,'',$insmap,$parent,$v)
	}
    }
    elsif ($refval eq 'HASH')
    {
	MapInsHash($pdf,$o,$insmap,$parent,$val);
    }
}

sub FindKey
{
    my $pdf=shift;
    my $page=shift;
    my $k=shift;

    if (exists($pdf->[$page]->{OBJ}->{$k}))
    {
	my $val=$pdf->[$page]->{OBJ}->{$k};
	$val=$pdf->[$$val]->{OBJ} if ref($val) eq 'OBJREF';
	return($val);
    }
    else
    {
	if (exists($pdf->[$page]->{OBJ}->{Parent}))
	{
	    return(FindKey($pdf,${$pdf->[$page]->{OBJ}->{Parent}},$k));
	}
    }

    return(undef);
}

sub FindPage
{
    my $wantpg=shift;
    my $pdf=shift;
    my $catalog=${$pdf->[0]->{OBJ}->{Root}};
    my $pages=${$pdf->[$catalog]->{OBJ}->{Pages}};

    return(NextPage($pdf,$pages,\$wantpg));
}

sub NextPage
{
    my $pdf=shift;
    my $pages=shift;
    my $wantpg=shift;
    my $ret;

    if ($pdf->[$pages]->{OBJ}->{Type} eq '/Pages')
    {
	foreach my $kid (@{$pdf->[$pages]->{OBJ}->{Kids}})
	{
	    $ret=NextPage($pdf,$$kid,$wantpg);
	    last if $$wantpg<=0;
	}
    }
    elsif ($pdf->[$pages]->{OBJ}->{Type} eq '/Page')
    {
	$$wantpg--;
	$ret=$pages;
    }

    return($ret);
}

sub nextwd
{
    my $pdfwds=shift;
    my $instring=shift || 0;

    my $wd=shift(@{$pdfwds});

    return('') if !defined($wd);
    return($wd) if $instring;

    if ($wd=~m/^(.*?)(<<|>>|(?:(?<!\\)\[|\]))(.*)/)
    {
	my ($p1,$p2,$p3)=($1,$2,$3);

	if (defined($p1) and length($p1))
	{
	    if (!($p2 eq ']' and $p1=~m/\[/))
	    {
		unshift(@{$pdfwds},$p3) if defined($p3) and length($p3);
		unshift(@{$pdfwds},$p2);
		$wd=$p1;
	    }
	}
	else
	{
	    unshift(@{$pdfwds},$p3) if defined($p3) and length($p3);
	    $wd=$p2;
	}
    }

    return($wd);
}

sub ParsePDFObj
{
    my $pdfwds=shift;
    my $rtn;
    my $wd;

    while ($wd=nextwd($pdfwds),length($wd))
    {
	if ($wd eq 'stream' or $wd eq 'endstream')
	{
	    next;
	}
	elsif ($wd eq 'endobj' or $wd eq 'startxref')
	{
	    last;
	}
	else
	{
	    unshift(@{$pdfwds},$wd);
	    $rtn=ParsePDFValue($pdfwds);
	}
    }

    return($rtn);
}

sub ParsePDFHash
{
    my $pdfwds=shift;
    my $rtn={};
    my $wd;

    while ($wd=nextwd($pdfwds),length($wd))
    {
	if ($wd eq '>>')
	{
	    last;
	}

	my (@w)=split('/',$wd,3);

	if ($w[0])
	{
	    Warn("PDF Dict Key '$wd' does not start with '/'");
	    exit 1;
	}
	else
	{
	    unshift(@{$pdfwds},"/$w[2]") if $w[2];
	    $wd=$w[1];
	    (@w)=split('\(',$wd,2);
	    $wd=$w[0];
	    unshift(@{$pdfwds},"($w[1]") if defined($w[1]);
	    (@w)=split('\<',$wd,2);
	    $wd=$w[0];
	    unshift(@{$pdfwds},"<$w[1]") if defined($w[1]);

	    $rtn->{$wd}=ParsePDFValue($pdfwds);
	}
    }

    return($rtn);
}

sub ParsePDFValue
{
    my $pdfwds=shift;
    my $rtn;
    my $wd=nextwd($pdfwds);

    if ($wd=~m/^\d+$/ and $pdfwds->[0]=~m/^\d+$/ and $pdfwds->[1]=~m/^R(\]|\>|\/)?/)
    {
	shift(@{$pdfwds});
	if (defined($1) and length($1))
	{
	    $pdfwds->[0]=substr($pdfwds->[0],1);
	}
	else
	{
	    shift(@{$pdfwds});
	}
	return(bless(\$wd,'OBJREF'));
    }

    if ($wd eq '<<')
    {
	return(ParsePDFHash($pdfwds));
    }

    if ($wd eq '[')
    {
	return(ParsePDFArray($pdfwds));
    }

    if ($wd=~m/(.*?)(\(.*)$/ and substr($wd,0,1) ne '/')
    {
	if (defined($1) and length($1))
	{
	    unshift(@{$pdfwds},$2);
	    $wd=$1;
	}
	else
	{
	    return(ParsePDFString($wd,$pdfwds));
	}
    }

    if ($wd=~m/(.*?)(\<.*)$/)
    {
	if (defined($1) and length($1))
	{
	    unshift(@{$pdfwds},$2);
	    $wd=$1;
	}
	else
	{
	    return(ParsePDFHexString($wd,$pdfwds));
	}
    }

    if ($wd=~m/(.+?)(\/.*)$/ and substr($wd,0,1) ne '/')
    {
	if (defined($2) and length($2))
	{
	    unshift(@{$pdfwds},$2);
	    $wd=$1;
	}
    }

    return($wd);
}

sub ParsePDFString
{
    my $wd=shift;
    my $rtn='';
    my $pdfwds=shift;
    my $lev=0;

    while (length($wd))
    {
	$rtn.=' ' if length($rtn);

	while ($wd=~m/(?<!\\)\(/g) {$lev++;}
	while ($wd=~m/(?<!\\)\)/g) {$lev--;}


	if ($lev<=0 and $wd=~m/^(.*?\))([^)]+)$/)
	    {
		unshift(@{$pdfwds},$2) if defined($2) and length($2);
		$wd=$1;
	    }

	    $rtn.=$wd;

	last if $lev <= 0;

	$wd=nextwd($pdfwds,1);
    }

    return($rtn);
}

sub ParsePDFHexString
{
    my $wd=shift;
    my $rtn='';
    my $pdfwds=shift;
    my $lev=0;

    if ($wd=~m/^(<.+?>)(.*)/)
    {
	unshift(@{$pdfwds},$2) if defined($2) and length($2);
	$rtn=$1;
    }

    return($rtn);
}

sub ParsePDFArray
{
    my $pdfwds=shift;
    my $rtn=[];
    my $wd;

    while (1)
    {
	$wd=ParsePDFValue($pdfwds);
	last if $wd eq ']' or length($wd)==0;
	push(@{$rtn},$wd);
    }

    return($rtn);
}

sub Notice
{
    if ($debug)
    {
	unshift(@_, "debug: ");
	my $msg=join('',@_);
	Msg(0,$msg);
    }
}

sub Warn
{
    unshift(@_, "warning: ");
    my $msg=join('',@_);
    Msg(0,$msg);
}

sub Die
{
    Msg(1,(@_));
}

sub Msg
{
    my ($fatal,$msg)=@_;

    print STDERR "$prog:";
    print STDERR "$env{SourceFile}:" if exists($env{SourceFile});
    print STDERR " ";

    if ($fatal)
    {
	print STDERR "fatal error: ";
    }

    print STDERR "$msg\n";
    exit 1 if $fatal;
}

sub PutXY
{
    my ($x,$y)=(@_);

    if ($frot)
    {
	return(d3($y)." ".d3($x));
    }
    else
    {
	$y=$mediabox[3]-$y;
	return(d3($x)." ".d3($y));
    }
}

sub GraphY
{
    my $y=shift;

    if ($frot)
    {
	return($y);
    }
    else
    {
	return($mediabox[3]-$y);
    }
}

sub Put
{
    my $msg=shift;

    print $msg;
    $fct+=length($msg);
}

sub PutObj
{
    my $ono=shift;
    my $inmem=shift;

    if ($inmem)
    {
	PutField($inmem,$obj[$ono]->{DATA});
	return;
    }

    my $msg="$ono 0 obj ";
    $obj[$ono]->{XREF}=$fct;
    if (exists($obj[$ono]->{STREAM}))
    {
	if ($gotzlib && ($options & COMPRESS) && !$debug && !exists($obj[$ono]->{DATA}->{'Filter'}))
	{
	    $obj[$ono]->{STREAM}=Compress::Zlib::compress($obj[$ono]->{STREAM});
	    $obj[$ono]->{DATA}->{'Filter'}='/FlateDecode';
	}

	$obj[$ono]->{DATA}->{'Length'}=length($obj[$ono]->{STREAM});
    }
    PutField(\$msg,$obj[$ono]->{DATA});
    PutStream(\$msg,$ono) if exists($obj[$ono]->{STREAM});
    Put($msg."endobj\n");
}

sub PutStream
{
    my $msg=shift;
    my $ono=shift;

    # We could 'flate' here
    $$msg.="stream\n$obj[$ono]->{STREAM}endstream\n";
}

sub PutField
{
    my $pmsg=shift;
    my $fld=shift;
    my $term=shift||"\n";
    my $typ=ref($fld);

    if ($typ eq '')
    {
	$$pmsg.="$fld$term";
    }
    elsif ($typ eq 'ARRAY')
    {
	$$pmsg.='[';
	    foreach my $cell (@{$fld})
	    {
		PutField($pmsg,$cell,' ');
	    }
	    $$pmsg.="]$term";
    }
    elsif ($typ eq 'HASH')
    {
	$$pmsg.='<< ';
	    foreach my $key (sort keys %{$fld})
	    {
		$$pmsg.="/$key ";
		PutField($pmsg,$fld->{$key});
	    }
	    $$pmsg.=">>$term";
    }
    elsif ($typ eq 'OBJREF')
    {
	$$pmsg.="$$fld 0 R$term";
    }
}

sub BuildObj
{
    my $ono=shift;
    my $val=shift;

    $obj[$ono]->{DATA}=$val;

    return("$ono 0 R ");
}

sub EmbedFont
{
    my $fontno=shift;
    my $fnt=shift;
    my $st=$objct;

    $fontlst{$fontno}->{OBJ}=BuildObj($objct,
	    {
		'Type' => '/Font',
		'Subtype' => '/Type1',
		'BaseFont' => '/'.$fnt->{internalname},
		'Widths' => $fnt->{Widths},
		'FirstChar' => $fnt->{FirstChar},
		'LastChar' => $fnt->{LastChar},
		'Encoding' => BuildObj($objct+1,
		{
		    'Type' => '/Encoding',
		    'Differences' => $fnt->{Differences}
		}),
		'FontDescriptor' => BuildObj($objct+2,
		{
		    'Type' => '/FontDescriptor',
		    'FontName' => '/'.$fnt->{internalname},
		    'Flags' => $fnt->{t1flags},
		    'FontBBox' => $fnt->{fntbbox},
		    'ItalicAngle' => $fnt->{slant},
		    'Ascent' => $fnt->{ascent},
		    'Descent' => $fnt->{fntbbox}->[1],
		    'CapHeight' => $fnt->{capheight},
		    'StemV' => 0,
		    'CharSet' => "($fnt->{CharSet})",
		} )
	    }
    );

    $fontlst{$fontno}->{OBJNO}=$objct;

    $objct+=2;
    $fontlst{$fontno}->{NM}='/F'.$fontno;
    $pages->{'Resources'}->{'Font'}->{'F'.$fontno}=$fontlst{$fontno}->{OBJ};
#    $fontlst{$fontno}->{FNT}=$fnt;
#    $obj[$objct]->{STREAM}=$t1stream;

    return($st+2);
}

sub LoadFont
{
    my $fontno=shift;
    my $fontnm=shift;
    my $ofontnm=$fontnm;

    return $fontlst{$fontno}->{OBJ} if (exists($fontlst{$fontno}) and $fontnm eq $fontlst{$fontno}->{FNT}->{name}) ;

    my $f;
    OpenFile(\$f,$fontdir,"$fontnm");

    if (!defined($f) and $Foundry)
    {
	# Try with no foundry
	$fontnm=~s/.*?-//;
	OpenFile(\$f,$fontdir,$fontnm);
    }

    Die("unable to open font '$ofontnm' for mounting") if !defined($f);

    my $foundry='';
    $foundry=$1 if $fontnm=~m/^(.)-/;
    my $stg=1;
    my %fnt;
    my @fntbbox=(0,0,0,0);
    my $capheight=0;
    my $lastchr=0;
    my $lastnm;
    my $t1flags=0;
    my $fixwid=-1;
    my $ascent=0;
    my $charset='';

    while (<$f>)
    {
	chomp;

	s/^ +//;
	s/^#.*// if $stg == 1;
	next if $_ eq '';

	if ($stg == 1)
	{
	    my ($key,$val)=split(' ',$_,2);

	    $key=lc($key);
	    $stg=2,next if $key eq 'kernpairs';
	    $stg=3,next if lc($_) eq 'charset';

	    $fnt{$key}=$val
	}
	elsif ($stg == 2)
	{
	    $stg=3,next if lc($_) eq 'charset';

	    my ($ch1,$ch2,$k)=split;
	}
	else
	{
	    my (@r)=split;
	    my (@p)=split(',',$r[1]);

	    if ($r[1] eq '"')
	    {
		$fnt{NAM}->{$r[0]}=$fnt{NAM}->{$lastnm};
		next;
	    }

	    $r[3]=oct($r[3]) if substr($r[3],0,1) eq '0';
	    $r[4]=$r[0] if !defined($r[4]);
	    $fnt{NAM}->{$r[0]}=[$p[0],$r[3],'/'.$r[4],undef,undef,$r[6]];
	    $fnt{NO}->[$r[3]]=$r[0];
	    $lastnm=$r[0];
	    $lastchr=$r[3] if $r[3] > $lastchr;
	    $fixwid=$p[0] if $fixwid == -1;
	    $fixwid=-2 if $fixwid > 0 and $p[0] != $fixwid;

	    $fntbbox[1]=-$p[2] if defined($p[2]) and -$p[2] < $fntbbox[1];
	    $fntbbox[2]=$p[0] if $p[0] > $fntbbox[2];
	    $fntbbox[3]=$p[1] if defined($p[1]) and $p[1] > $fntbbox[3];
	    $ascent=$p[1] if defined($p[1]) and $p[1] > $ascent and $r[3] >= 32 and $r[3] < 128;
	    $charset.='/'.$r[4] if defined($r[4]);
	    $capheight=$p[1] if length($r[4]) == 1 and $r[4] ge 'A' and $r[4] le 'Z' and $p[1] > $capheight;
	}
    }

    close($f);

    $fnt{NAM}->{space}->[MINOR]=32;
    $fnt{NAM}->{space}->[MAJOR]=0;
    my $fno=0;
    my $slant=0;
    $fnt{DIFF}=[];
    $fnt{WIDTH}=[];
    $fnt{fntbbox}=\@fntbbox;
    $fnt{ascent}=$ascent;
    $fnt{capheight}=$capheight;
    $fnt{lastchr}=$lastchr;
    $fnt{NAM}->{''}=[0,-1,'/.notdef',-1,0];
    $slant=-$fnt{'slant'} if exists($fnt{'slant'});
    $fnt{slant}=$slant;
    $fnt{nospace}=(!defined($fnt{NAM}->{space}->[PSNAME]) or $fnt{NAM}->{space}->[PSNAME] ne '/space' or !exists($fnt{'spacewidth'}))?1:0;
    $fnt{'spacewidth'}=270 if !exists($fnt{'spacewidth'});
    Notice("Using nospace mode for font '$ofontnm'") if $fnt{nospace} == 1 and $options & USESPACE;

    $t1flags|=2**0 if $fixwid > -1;
    $t1flags|=(exists($fnt{'special'}))?2**2:2**5;
    $t1flags|=2**6 if $fnt{slant} != 0;
    $fnt{t1flags}=$t1flags;
    my $fontkey="$foundry $fnt{internalname}";

    Warn("\nFont '$fnt{internalname} ($ofontnm)' has $lastchr glyphs\n"
	."You would see a noticeable speedup if you install the perl module Inline::C\n") if !$gotinline and $lastchr > 1000;

    if (exists($download{$fontkey}))
    {
	# Real font needs subsetting
	$fnt{fontfile}=$download{$fontkey};
#	my ($head,$body,$tail)=GetType1($download{$fontkey});
#	$head=~s/\/Encoding .*?readonly def\b/\/Encoding StandardEncoding def/s;
#	$fontlst{$fontno}->{HEAD}=$head;
#	$fontlst{$fontno}->{BODY}=$body;
#	$fontlst{$fontno}->{TAIL}=$tail;
#	$fno=++$objct;
#	EmbedFont($fontno,\%fnt);
    }
    else
    {
	if (exists($missing{$fontkey}))
	{
	    Warn("The download file in '$missing{$fontkey}' "
	    . " has erroneous entry for '$fnt{internalname} ($ofontnm)'");
	}
	else
	{
	    Warn("unable to embed font file for '$fnt{internalname}'"
	    . " ($ofontnm) (missing entry in 'download' file?)")
	    if $embedall;
	}
    }

    $fontlst{$fontno}->{NM}='/F'.$fontno;
    $fontlst{$fontno}->{FNT}=\%fnt;

    if (defined($fnt{encoding}) and $fnt{encoding} eq 'text.enc' and $ucmap ne '')
    {
	if ($textenccmap eq '')
	{
	    $textenccmap = BuildObj($objct+1,{});
	    $objct++;
	    $obj[$objct]->{STREAM}=$ucmap;
	}
    }

#    PutObj($fno);
#    PutObj($fno+1);
#    PutObj($fno+2) if defined($obj[$fno+2]);
#    PutObj($fno+3) if defined($obj[$fno+3]);
}

sub GetType1
{
    my $file=shift;
    my ($l1,$l2,$l3);		# Return lengths
    my ($head,$body,$tail);	# Font contents
    my $f;

    OpenFile(\$f,$fontdir,"$file");
    Die("unable to open font '$file' for embedding") if !defined($f);

    $head=GetChunk($f,1,"currentfile eexec");
    $body=GetChunk($f,2,"00000000") if !eof($f);
    $tail=GetChunk($f,3,"cleartomark") if !eof($f);

    return($head,$body,$tail);
}

sub GetChunk
{
    my $F=shift;
    my $segno=shift;
    my $ascterm=shift;
    my ($type,$hdr,$chunk,@msg);
    binmode($F);
    my $enc="ascii";

    while (1)
    {
	# There may be multiple chunks of the same type

	my $ct=read($F,$hdr,2);

	if ($ct==2)
	{
	    if (substr($hdr,0,1) eq "\x80")
	    {
		# binary chunk

		my $chunktype=ord(substr($hdr,1,1));
		$enc="binary";

		if (defined($type) and $type != $chunktype)
		{
		    seek($F,-2,1);
		    last;
		}

		$type=$chunktype;
		return if $chunktype == 3;

		$ct=read($F,$hdr,4);
		Die("failed to read binary segment length") if $ct != 4;
		my $sl=unpack('V',$hdr);
		my $data;
		my $chk=read($F,$data,$sl);
		Die("failed to read binary segment") if $chk != $sl;
		$chunk.=$data;
	    }
	    else
	    {
		# ascii chunk

		my $hex=0;
		seek($F,-2,1);
		my $ct=0;

		while (1)
		{
		    my $lin=<$F>;

		    last if !$lin;

		    $hex=1,$enc.=" hex" if $segno == 2 and !$ct and $lin=~m/^[A-F0-9a-f]{4,4}/;

		    if ($segno !=2 and $lin=~m/^(.*$ascterm[\n\r]?)(.*)/)
		    {
			$chunk.=$1;
			seek($F,-length($2)-1,1) if $2;
			last;
		    }
		    elsif ($segno == 2 and $lin=~m/^(.*?)($ascterm.*)/)
		    {
			$chunk.=$1;
			seek($F,-length($2)-1,1) if $2;
			last;
		    }

		    chomp($lin), $lin=pack('H*',$lin) if $hex;
		    $chunk.=$lin; $ct++;
		}

		last;
	    }
	}
	else
	{
	    push(@msg,"Failed to read 2 header bytes");
	}
    }

    return $chunk;
}

sub OutStream
{
    my $ono=shift;

    IsGraphic();
    $stream.="Q\n";
    $obj[$ono]->{STREAM}=$stream;
    $obj[$ono]->{DATA}->{Length}=length($stream);
    $stream='';
    PutObj($ono);
}

sub do_p
{
    my $trans='BLOCK';

    $trans='PAGE' if $firstpause;
    NewPage($trans);
    @XOstream=();
    @PageAnnots=();
    $firstpause=1;
}

sub FixTrans
{
    my $t=shift;
    my $style=$t->{S};

    if ($style)
    {
	delete($t->{Dm}) if $style ne '/Split' and $style ne '/Blinds';
	delete($t->{M})  if !($style eq '/Split' or $style eq '/Box' or $style eq '/Fly');
	delete($t->{Di}) if !($style eq '/Wipe' or $style eq '/Glitter' or $style eq '/Fly' or $style eq '/Cover' or $style eq '/Uncover' or $style eq '/Push') or ($style eq '/Fly' and $t->{Di} eq '/None' and $t->{SS} != 1);
	delete($t->{SS}) if !($style eq '/Fly');
	delete($t->{B})  if !($style eq '/Fly');
    }

    return($t);
}

sub NewPage
{
    my $trans=shift;
    # Start of pages

    if ($cpageno > 0)
    {
	if ($#XOstream>=0)
	{
	    MakeXO() if $stream;
	    $stream=join("\n",@XOstream,'');
	}

	my %t=%{$transition->{$trans}};
	$cpage->{MediaBox}=\@mediabox if $custompaper;
	$cpage->{Trans}=FixTrans(\%t) if $t{S};

	if ($#PageAnnots >= 0)
	{
	    @{$cpage->{Annots}}=@PageAnnots;
	}

	if ($#bgstack > -1 or $bgbox)
	{
	    my $box="q 1 0 0 1 0 0 cm ";

	    foreach my $bg (@bgstack)
	    {
		# 0=$bgtype # 1=stroke 2=fill. 4=page
		# 1=$strkcol
		# 2=$fillcol
		# 3=(Left,Top,Right,bottom,LineWeight)
		# 4=Start ypos
		# 5=Endypos
		# 6=Line Weight

		my $pg=$bg->[3] || \@defaultmb;

		$bg->[5]=$pg->[3];      # box is continuing to next page
		$box.=DrawBox($bg);
		$bg->[4]=$pg->[1];      # will continue from page top
	    }

	    $stream=$box.$bgbox."Q\n".$stream;
	    $bgbox='';
	    $boxmax=0;
	}

	PutObj($cpageno);
	OutStream($cpageno+1);
    }

    $cpageno=++$objct;

    my $thispg=BuildObj($objct,
	    {
		'Type' => '/Page',
		'Group' =>
		{
		    'CS' => '/DeviceRGB',
		    'S' => '/Transparency'
		},
		'Parent' => '2 0 R',
		'Contents' =>
		[
		    BuildObj($objct+1,
		    {
			'Length' => 0
		    } )
		],
	    }
    );

    splice(@{$pages->{Kids}},++$pginsert,0,$thispg);
    splice(@outlines,$pginsert,0,[$curoutlev,$#{$curoutlev}+1,$thislev]);

    $objct+=1;
    $cpage=$obj[$cpageno]->{DATA};
    $pages->{'Count'}++;
    $stream="q 1 0 0 1 0 0 cm\n$linejoin J\n$linecap j\n0.4 w\n";
    $stream.=$strkcol."\n", $curstrk=$strkcol if $strkcol ne '';
	    $mode='g';
	    $curfill='';
#	    @mediabox=@defaultmb;
}

sub DrawBox
{
    my $bg=shift;
    my $res='';
    my $pg=$bg->[3] || \@mediabox;
    $bg->[4]=$pg->[1], $bg->[5]=$pg->[3] if $bg->[0] & 4;
    my $bot=$bg->[5];
    $bot=$boxmax if $boxmax > $bot;
    my $wid=$pg->[2]-$pg->[0];
    my $dep=$bot-$bg->[4];

    $res="$bg->[1] $bg->[2] $bg->[6] w\n";
    $res.="$pg->[0] $bg->[4] $wid $dep re f " if $bg->[0] & 1;
    $res.="$pg->[0] $bg->[4] $wid $dep re s " if $bg->[0] & 2;

    return("$res\n");
}

sub MakeXO
{
    $stream.="%mode=$mode\n";
    IsGraphic();
    $stream.="Q\n";
    my $xobj=++$objct;
    my $xonm="XO$xobj";
    $pages->{'Resources'}->{'XObject'}->{$xonm}=BuildObj($xobj,{'Type' => '/XObject', 'BBox' => \@mediabox, 'Name' => "/$xonm", 'FormType' => 1, 'Subtype' => '/Form', 'Length' => 0, 'Type' => "/XObject"});
    $obj[$xobj]->{STREAM}=$stream;
    $stream='';
    push(@XOstream,"q") if $#XOstream==-1;
    push(@XOstream,"/$xonm Do");
}

sub do_f
{
    my $par=shift;
    my $fnt=$fontlst{$par}->{FNT};
    $thisfnt=$fnt;

#    IsText();
    $cft="$par";
    $fontchg=1;
    PutLine();
}

sub IsText
{
    if ($mode eq 'g')
    {
	$stream.="q BT\n$matrix ".PutXY($xpos,$ypos)." Tm\n";
	$poschg=0;
	$matrixchg=0;
	$tmxpos=$xpos;
	$stream.=$textcol."\n", $curfill=$textcol if $textcol ne $curfill;

	if (defined($cft))
	{
	    $fontchg=1;
#	   $stream.="/F$cft $cftsz Tf\n";
	}

	$stream.="$curkern Tc\n";
    }

    if ($poschg or $matrixchg)
    {
	PutLine(0) if $matrixchg;
	shift(@lin) if $#lin==0 and !defined($lin[0]->[CHR]);
	$stream.="$matrix ".PutXY($xpos,$ypos)." Tm\n", $poschg=0;
	$tmxpos=$xpos;
	$matrixchg=0;
	$stream.="$curkern Tc\n";
    }

    $mode='t';
}

sub IsGraphic
{
    if ($mode eq 't')
    {
	PutLine();
	$stream.="ET Q\n";
	$stream.=$strkcol."\n", $curstrk=$strkcol if $strkcol ne $curstrk;
	$curfill=$fillcol;
    }
    $mode='g';
}

sub do_s
{
    my $par=shift;
    $par/=$unitwidth;

    $whtsz=$fontlst{$cft}->{FNT}->{spacewidth}*$par if !defined($whtsz) and defined($cft);

    if ($par != $cftsz and defined($cft))
    {
	PutLine();
	$cftsz=$par;
	Set_LWidth() if $lwidth < 1;
	$fontchg=1;
    }
    else
    {
	$cftsz=$par;
	Set_LWidth() if $lwidth < 1;
    }
}

sub Set_LWidth
{
    IsGraphic();
    $stream.=((($desc{res}/(72*$desc{sizescale}))*$linewidth*$cftsz)/1000)." w\n";
    return;
}

sub do_m
{
    # Groff uses /m[] for text & graphic stroke, and /M[] (DF?) for
    # graphic fill.  PDF uses G/RG/K for graphic stroke, and g/rg/k for
    # text & graphic fill.
    #
    # This means that we must maintain g/rg/k state separately for text
    # colour & graphic fill (this is probably why 'gs' maintains
    # separate graphic states for text & graphics when distilling PS ->
    # PDF).
    #
    # To facilitate this:-
    #
    #   $textcol	= current groff stroke colour
    #   $fillcol	= current groff fill colour
    #   $curfill	= current PDF fill colour

    my $par=shift;
    my $mcmd=substr($par,0,1);

    $par=substr($par,1);
    $par=~s/^ +//;

#    IsGraphic();

    $textcol=set_col($mcmd,$par,0);
    $strkcol=set_col($mcmd,$par,1);

    if ($mode eq 't')
    {
	PutLine();
	$stream.=$textcol."\n";
	$curfill=$textcol;
    }
    else
    {
	$stream.="$strkcol\n";
	$curstrk=$strkcol;
    }
}

sub set_col
{
    my $mcmd=shift;
    my $par=shift;
    my $upper=shift;
    my @oper=('g','k','rg');

    @oper=('G','K','RG') if $upper;

    if ($mcmd eq 'd')
    {
	# default colour
	return("0 $oper[0]");
    }

    my (@c)=split(' ',$par);

    if ($mcmd eq 'c')
    {
	# Text CMY
	return(d3($c[0]/65535).' '.d3($c[1]/65535).' '.d3($c[2]/65535)." 0 $oper[1]");
    }
    elsif ($mcmd eq 'k')
    {
	# Text CMYK
	return(d3($c[0]/65535).' '.d3($c[1]/65535).' '.d3($c[2]/65535).' '.d3($c[3]/65535)." $oper[1]");
    }
    elsif ($mcmd eq 'g')
    {
	# Text Grey
	return(d3($c[0]/65535)." $oper[0]");
    }
    elsif ($mcmd eq 'r')
    {
	# Text RGB0
	return(d3($c[0]/65535).' '.d3($c[1]/65535).' '.d3($c[2]/65535)." $oper[2]");
    }
}

sub do_D
{
    my $par=shift;
    my $Dcmd=substr($par,0,1);

    $par=substr($par,1);

    IsGraphic();

    if ($Dcmd eq 'F')
    {
	my $mcmd=substr($par,0,1);

	$par=substr($par,1);
	$par=~s/^ +//;

	$fillcol=set_col($mcmd,$par,0);
	$stream.="$fillcol\n";
	$curfill=$fillcol;
    }
    elsif ($Dcmd eq 'f')
    {
	my $mcmd=substr($par,0,1);

	$par=substr($par,1);
	$par=~s/^ +//;
	($par)=split(' ',$par);

	if ($par >= 0 and $par <= 1000)
	{
	    $fillcol=set_col('g',int((1000-$par)*65535/1000),0);
	}
	else
	{
	    $fillcol=lc($textcol);
	}

	$stream.="$fillcol\n";
	$curfill=$fillcol;
    }
    elsif ($Dcmd eq '~')
    {
	# B-Spline
	my (@p)=split(' ',$par);
	my ($nxpos,$nypos);

	foreach my $p (@p) { $p/=$unitwidth; }
	$stream.=PutXY($xpos,$ypos)." m\n";
	$xpos+=($p[0]/2);
	$ypos+=($p[1]/2);
	$stream.=PutXY($xpos,$ypos)." l\n";

	for (my $i=0; $i < $#p-1; $i+=2)
	{
	    $nxpos=(($p[$i]*$tnum)/(2*$tden));
	    $nypos=(($p[$i+1]*$tnum)/(2*$tden));
	    $stream.=PutXY(($xpos+$nxpos),($ypos+$nypos))." ";
	    $nxpos=($p[$i]/2 + ($p[$i+2]*($tden-$tnum))/(2*$tden));
	    $nypos=($p[$i+1]/2 + ($p[$i+3]*($tden-$tnum))/(2*$tden));
	    $stream.=PutXY(($xpos+$nxpos),($ypos+$nypos))." ";
	    $nxpos=(($p[$i]-$p[$i]/2) + $p[$i+2]/2);
	    $nypos=(($p[$i+1]-$p[$i+1]/2) + $p[$i+3]/2);
	    $stream.=PutXY(($xpos+$nxpos),($ypos+$nypos))." c\n";
	    $xpos+=$nxpos;
	    $ypos+=$nypos;
	}

	$xpos+=($p[$#p-1]-$p[$#p-1]/2);
	$ypos+=($p[$#p]-$p[$#p]/2);
	$stream.=PutXY($xpos,$ypos)." l\nS\n";
	$poschg=1;
    }
    elsif ($Dcmd eq 'p' or $Dcmd eq 'P')
    {
	# Polygon
	my (@p)=split(' ',$par);
	my ($nxpos,$nypos);

	foreach my $p (@p) { $p/=$unitwidth; }
	$stream.=PutXY($xpos,$ypos)." m\n";

	for (my $i=0; $i < $#p; $i+=2)
	{
	    $xpos+=($p[$i]);
	    $ypos+=($p[$i+1]);
	    $stream.=PutXY($xpos,$ypos)." l\n";
	}

	if ($Dcmd eq 'p')
	{
	    $stream.="s\n";
	}
	else
	{
	    $stream.="f\n";
	}

	$poschg=1;
    }
    elsif ($Dcmd eq 'c')
    {
	# Stroke circle
	$par=substr($par,1);
	my (@p)=split(' ',$par);

	DrawCircle($p[0],$p[0]);
	$stream.="s\n";
	$poschg=1;
    }
    elsif ($Dcmd eq 'C')
    {
	# Fill circle
	$par=substr($par,1);
	my (@p)=split(' ',$par);

	DrawCircle($p[0],$p[0]);
	$stream.="f\n";
	$poschg=1;
    }
    elsif ($Dcmd eq 'e')
    {
	# Stroke ellipse
	$par=substr($par,1);
	my (@p)=split(' ',$par);

	DrawCircle($p[0],$p[1]);
	$stream.="s\n";
	$poschg=1;
    }
    elsif ($Dcmd eq 'E')
    {
	# Fill ellipse
	$par=substr($par,1);
	my (@p)=split(' ',$par);

	DrawCircle($p[0],$p[1]);
	$stream.="f\n";
	$poschg=1;
    }
    elsif ($Dcmd eq 'l')
    {
	# Line To
	$par=substr($par,1);
	my (@p)=split(' ',$par);

	foreach my $p (@p) { $p/=$unitwidth; }
	$stream.=PutXY($xpos,$ypos)." m\n";
	$xpos+=$p[0];
	$ypos+=$p[1];
	$stream.=PutXY($xpos,$ypos)." l\n";

	$stream.="S\n";
	$poschg=1;
    }
    elsif ($Dcmd eq 't')
    {
	# Line Thickness
	$par=substr($par,1);
	my (@p)=split(' ',$par);

	foreach my $p (@p) { $p/=$unitwidth; }
#	$xpos+=$p[0]*100;	       # WTF!!!
#	int lw = ((font::res/(72*font::sizescale))*linewidth*env->size)/1000;
	$p[0]=(($desc{res}/(72*$desc{sizescale}))*$linewidth*$cftsz)/1000 if $p[0] < 0;
	$lwidth=$p[0];
	$stream.="$p[0] w\n";
	$poschg=1;
	$xpos+=$lwidth;
    }
    elsif ($Dcmd eq 'a')
    {
	# Arc : h1 v1 h2 v2
	$par=substr($par,1);
	my (@p)=split(' ',$par);
	my $rad180=3.14159;
	my $rad360=$rad180*2;
	my $rad90=$rad180/2;

	foreach my $p (@p) { $p/=$unitwidth; }

	# Documentation is wrong. Groff does not use Dh1,Dv1 as centre
	# of the circle!

	my $centre=adjust_arc_centre(\@p);

	# Using formula here : http://www.tinaja.com/glib/bezcirc2.pdf
	# First calculate angle between start and end point

	my ($startang,$r)=RtoP(-$centre->[0],$centre->[1]);
	my ($endang,$r2)=RtoP(($p[0]+$p[2])-$centre->[0],-($p[1]+$p[3]-$centre->[1]));

	if (abs($endang-$startang) < 0.004)
	{
	    if ($frot)
	    {
		$stream.="q $ypos ".GraphY($xpos)." m ".($ypos+$p[1]+$p[3])." ".GraphY($xpos+$p[0]+$p[2])." l S Q\n";
	    }
	    else
	    {
		$stream.="q $xpos ".GraphY($ypos)." m ".($xpos+$p[0]+$p[2])." ".GraphY($ypos+$p[1]+$p[3])." l S Q\n";
	    }
	}
	else
	{
	    $endang+=$rad360 if $endang < $startang;
	    my $pieces=int(($endang-$startang) / $rad90)+1;
	    my $totang=($endang-$startang)/$pieces;       # do it in pieces

	    # Now 1 piece

	    my $x0=cos($totang/2);
	    my $y0=sin($totang/2);
	    return if !$y0;
	    my $x3=$x0;
	    my $y3=-$y0;
	    my $x1=(4-$x0)/3;
	    my $y1=((1-$x0)*(3-$x0))/(3*$y0);
	    my $x2=$x1;
	    my $y2=-$y1;

	    # Rotate to start position and draw pieces

	    foreach my $j (0..$pieces-1)
	    {
		PlotArcSegment($totang/2+$startang+$j*$totang,$r,d3($xpos+$centre->[0]),d3(GraphY($ypos+$centre->[1])),d3($x0),d3($y0),d3($x1),d3($y1),d3($x2),d3($y2),d3($x3),d3($y3));
	    }
	}

	$xpos+=$p[0]+$p[2];
	$ypos+=$p[1]+$p[3];

	$poschg=1;
    }
}

sub deg
{
    return int($_[0]*180/3.14159);
}

sub adjust_arc_centre
{
    # Taken from geometry.cpp

    # We move the center along a line parallel to the line between
    # the specified start point and end point so that the center
    # is equidistant between the start and end point.
    # It can be proved (using Lagrange multipliers) that this will
    # give the point nearest to the specified center that is equidistant
    # between the start and end point.

    my $p=shift;
    my @c;
    my $x = $p->[0] + $p->[2];  # (x, y) is the end point
    my $y = $p->[1] + $p->[3];
    my $n = $x*$x + $y*$y;
    if ($n != 0)
    {
	$c[0]= $p->[0];
	$c[1] = $p->[1];
	my $k = .5 - ($c[0]*$x + $c[1]*$y)/$n;
	$c[0] += $k*$x;
	$c[1] += $k*$y;
	return(\@c);
    }
    else
    {
	return([0,0]);
    }
}


sub PlotArcSegment
{
    my ($ang,$r,$transx,$transy,$x0,$y0,$x1,$y1,$x2,$y2,$x3,$y3)=@_;
    my $cos=sprintf("%0.5f",cos($ang));
    my $sin=sprintf("%0.5f",sin($ang));
    my @mat=($cos,$sin,-$sin,$cos,0,0);
    my $lw=$lwidth/$r;

    if ($frot)
    {
       $stream.="q $r 0 0 $r $transy $transx cm ".join(' ',@mat)." cm $lw w $y0 $x0 m $y1 $x1 $y2 $x2 $y3 $x3 c S Q\n";
    }
    else
    {
       $stream.="q $r 0 0 $r $transx $transy cm ".join(' ',@mat)." cm $lw w $x0 $y0 m $x1 $y1 $x2 $y2 $x3 $y3 c S Q\n";
    }
}

sub DrawCircle
{
    my $hd=shift;
    my $vd=shift;
    my $hr=$hd/2/$unitwidth;
    my $vr=$vd/2/$unitwidth;
    my $kappa=0.5522847498;
    $hd/=$unitwidth;
    $vd/=$unitwidth;
    $stream.=PutXY(($xpos+$hd),$ypos)." m\n";
    $stream.=PutXY(($xpos+$hd),($ypos+$vr*$kappa))." ".PutXY(($xpos+$hr+$hr*$kappa),($ypos+$vr))." ".PutXY(($xpos+$hr),($ypos+$vr))." c\n";
    $stream.=PutXY(($xpos+$hr-$hr*$kappa),($ypos+$vr))." ".PutXY(($xpos),($ypos+$vr*$kappa))." ".PutXY(($xpos),($ypos))." c\n";
    $stream.=PutXY(($xpos),($ypos-$vr*$kappa))." ".PutXY(($xpos+$hr-$hr*$kappa),($ypos-$vr))." ".PutXY(($xpos+$hr),($ypos-$vr))." c\n";
    $stream.=PutXY(($xpos+$hr+$hr*$kappa),($ypos-$vr))." ".PutXY(($xpos+$hd),($ypos-$vr*$kappa))." ".PutXY(($xpos+$hd),($ypos))." c\n";
    $xpos+=$hd;

    $poschg=1;
}

sub FindCircle
{
    my ($x1,$y1,$x2,$y2,$x3,$y3)=@_;
    my ($Xo, $Yo);

    my $x=$x2+$x3;
    my $y=$y2+$y3;
    my $n=$x**2+$y**2;

    if ($n)
    {
	my $k=.5-($x2*$x + $y2*$y)/$n;
	return(sqrt($n),$x2+$k*$x,$y2+$k*$y);
    }
    else
    {
	return(-1);
    }
}

sub PtoR
{
    my ($theta,$r)=@_;

    return($r*cos($theta),$r*sin($theta));
}

sub RtoP
{
    my ($x,$y)=@_;

    return(atan2($y,$x),sqrt($x**2+$y**2));
}

sub PutLine
{
    my $f=shift;

    IsText() if !defined($f);

    return if (scalar(@lin) == 0 or ($#lin == 0 and !defined($lin[0]->[CHR])));

    my $s='[ ';
    my $n=1;
    my $len=0;
    my $rev=0;

    if ($xrev)
    {
	$len=($lin[$#lin]->[XPOS]-$lin[0]->[XPOS]+$lin[$#lin]->[HWID])*1000/$cftsz;
	$s.=d3($len).' ' if $len;
	$rev=1;
    }

    $stream.="%! wht0sz=".d3($whtsz/$unitwidth).", wt=".((defined($wt))?d3($wt/$unitwidth):'--')."\n" if $debug;

    foreach my $c (@lin)
    {
	my $chr=$c->[CHR];
	my $char;
	my $chrc=defined($chr)?$c->[CHF]->[MAJOR].'/'.$chr:'';
	$chrc.="(".chr(abs($chr)).")" if defined($chr) and $cftmajor==0 and $chr<128;
	$chrc.="[$c->[CHF]->[PSNAME]]" if defined($chr);

	if (defined($chr))
	{
	    $chr=abs($chr);
	    $char=chr($chr);
	    $char="\\\\" if $char eq "\\";
	    $char="\\(" if $char eq "(";
	    $char="\\)" if $char eq ")";
	}

	$stream.="%! PutLine: XPOS=$c->[XPOS], CHR=$chrc, CWID=$c->[CWID], HWID=$c->[HWID], NOMV=$c->[NOMV]\n" if $debug;

	if (!defined($chr) and defined($wt))
	{
	    # white space

	    my $gap = $c->[HWID]*$unitwidth;

	    if ($options & USESPACE and $thisfnt->{nospace}==0)
	    {
		$stream.="%!! GAP=".($gap)."\n" if $debug;

#		while ($gap >= $whtsz+$wt)
#		while (abs($gap - ($whtsz+$wt)) > 1)
		if ($wt >= 0)
		{
		    my $i=int(($gap+1) / ($whtsz+$wt));

		    if ($i < 6)
		    {
			$s.="(",$n=0 if $n;
			$s.=' ' x $i;
			$gap-=($whtsz+$wt) * $i;
		    }
		}
		else
		{
		    $wt=0;
		}
	    }

	    if (abs($gap) > 1)
	    {
		$s.=') ' if !$n;
		$s.=d3(-$gap/$cftsz).' (';
		$n=0;
	    }
	}
	elsif ($c->[CWID] != $c->[HWID])
	{
	    if ($rev)
	    {
		$s.=') ' if !$n;
		$s.=d3(($c->[CWID]-$c->[HWID])*1000/$cftsz).' (';
		$n=0;
	    }

	    if (defined($chr))
	    {
		$s.=' (',$n=0 if $n;
		$s.=$char;
	    }

	    if (!$rev)
	    {
		$s.=') ' if !$n;
		$s.=d3((($c->[CWID]-$c->[HWID])*1000)/$cftsz).' (';
		$n=0;
	    }

	}
	else
	{
	    $s.="(",$n=0 if $n;
	    $s.=$char;
	}
    }

    $s=substr($s,0,-1),$n=1 if substr($s,-1) eq "(" and substr($s,-2,1) ne "\\";
    $s.=")" if !$n;
    $s.=d3(-$len) if $len;
    $wt=0 if !defined($wt);
    $stream.=d3($wt/$unitwidth)." Tw ";
    $stream.="$s] TJ\n";
    @lin=();
    $wt=undef;
    $whtsz=$fontlst{$cft}->{FNT}->{spacewidth}*$cftsz;
}

sub d3
{
    return(sprintf("%.3f",shift || 0));
}

sub LoadAhead
{
    my $no=shift;

    foreach my $j (1..$no)
    {
	my $lin=<>;
	chomp($lin);
	$lin=~s/\r$//;
	$lct++;

	push(@ahead,$lin);
	$stream.="%% $lin\n" if $debug;
    }
}

sub do_V
{
    my $par=shift;

    if ($mode eq 't')
    {
	PutLine();
    }

    $ypos=$par/$unitwidth;

    $poschg=1;
}

sub do_v
{
    my $par=shift;

    PutLine() if $mode eq 't';

    $ypos+=$par/$unitwidth;

    $poschg=1;
}

sub GetNAM
{
    my ($f,$c)=(@_);

    my $r=$f->{NAM}->{$c};
    return($r,$c) if ref($r) eq 'ARRAY';
    return($f->{NAM}->{$r},$r);
}

sub AssignGlyph
{
    my ($fnt,$chf,$ch)=(@_);

    if ($chf->[CHRCODE] > 32 and $chf->[CHRCODE] < 128)
    {
	($chf->[MINOR],$chf->[MAJOR])=($chf->[CHRCODE],0);
    }
    elsif ($chf->[CHRCODE] == 173)
    {
	($chf->[MINOR],$chf->[MAJOR])=(31,0);
    }
    else
    {
	($chf->[MINOR],$chf->[MAJOR])=NextAlloc($fnt);
    }

#   $fnt->{SUB}->[$chf->[MAJOR]]->{CHARSET}.=$chf->[PSNAME];

    my $uc;

    # Add ToUnicode CMap entry - requires change to afmtodit

    push(@{$fnt->{CHARSET}->[$chf->[MAJOR]]},$chf->[PSNAME]);
    push(@{$fnt->{TRFCHAR}->[$chf->[MAJOR]]},$ch);
    $stream.="% Assign: $chf->[PSNAME] to $chf->[MAJOR]/$chf->[MINOR]\n" if $debug;
}

sub PutGlyph
{
    my ($fnt,$ch,$nowidth)=@_;
    my $chf;
    ($chf,$ch)=GetNAM($fnt,$ch);

    IsText();

    if ($n_flg and defined($mark))
    {
	$mark->{ypos}=$ypos;
	$mark->{xpos}=$xpos;
    }

    $n_flg=0;

    if (!defined($chf->[MINOR]))
    {
	AssignGlyph($fnt,$chf,$ch);
    }

    if ($fontchg or $chf->[MAJOR] != $cftmajor)
    {
	PutLine();
	$cftmajor=$chf->[MAJOR];
#	$whtsz=$fontlst{$cft}->{FNT}->{spacewidth}*$cftsz;
	my $c=$cft;
	$c.=".".$cftmajor if $cftmajor;
	$stream.="/F$c $cftsz Tf\n";
	$fontchg=0;
    }

    my $cn=$chf->[MINOR];
    my $chr=chr($cn);
    my $cwid=($chf->[WIDTH]*$cftsz)/$unitwidth+$curkern;
    my $hwid=($nowidth)?0:$cwid;

    $gotT=1;

    if ($xrev)
    {
	PutLine(0) if $#lin > -1 and ($lin[$#lin]->[CHR]||0) > 0;
	$cn=-$cn;
    }
    else
    {
	PutLine(0) if $#lin > -1 and ($lin[$#lin]->[CHR]||0) < 0;
    }

    if ($#lin < 1)
    {
	if (!$inxrev and $cn < 0) # in xrev
	{
	    MakeMatrix(1);
	    $inxrev=1;
	    $#lin=-1;
	}
	elsif ($inxrev and $cn > 0)
	{
	    MakeMatrix(0);
	    $inxrev=0;
	    $#lin=-1;
	}

	if ($matrixchg or $poschg)
	{
	    $stream.="$matrix ".PutXY($xpos,$ypos)." Tm\n", $poschg=0;
	    $tmxpos=$xpos;
	    $matrixchg=0;
	    $stream.="$curkern Tc\n";
	}
    }

    $whtsz=$fontlst{$cft}->{FNT}->{spacewidth}*$cftsz if $#lin==-1;
#     $stream.="%!!! Put: font=$cft, char=$chf->[PSNAME]\n" if $debug;

    push(@lin,[$cn,$xpos,$cwid,$hwid,$nowidth,$chf]);

    $xpos+=$hwid;
}

sub do_t
{
    my $par=shift;
    my $fnt=$fontlst{$cft}->{FNT};

    if ($kernadjust != $curkern)
    {
	PutLine();
	$stream.="$kernadjust Tc\n";
	$curkern=$kernadjust;
    }

    IsText();

    foreach my $j (0..length($par)-1)
    {
	my $ch=substr($par,$j,1);

	PutGlyph($fnt,$ch,0);
    }
}

sub do_u
{
    my $par=shift;

    $par=m/([+-]?\d+) (.*)/;
    $kernadjust=$1/$unitwidth;
    do_t($2);
    $kernadjust=0;
}

sub do_h
{
    my $v=shift;

    $v/=$unitwidth;

    if ($mode eq 't')
    {
	if ($w_flg)
	{
	    if ($#lin > -1 and $lin[$#lin]->[NOMV]==1)
	    {
		$lin[$#lin]->[HWID]=$v;
	    }
	    else
	    {
		push(@lin,[undef,$xpos,$v,$v,0]);
	    }

	    if (!defined($wt))
	    {
		$whtsz=$fontlst{$cft}->{FNT}->{spacewidth}*$cftsz;
		$wt=($v * $unitwidth) - $whtsz;
		$stream.="%!! wt=$wt, whtsz=$whtsz\n" if $debug;
	    }

	    $w_flg=0;
	}
	else
	{
	    if ($#lin > -1 and $lin[$#lin]->[NOMV]==1)
	    {
		$lin[$#lin]->[HWID]=$v;
	    }
	    else
	    {
		push(@lin,[undef,$xpos,0,$v,0]);
	    }
	}
    }

    $xpos+=$v;
}

sub do_H
{
    my $par=shift;
    $xpos=($par/$unitwidth);

    if ($mode eq 't')
    {
#	PutLine();
	if ($#lin > -1)
	{
	    $lin[$#lin]->[HWID]=d3($xpos-$lin[$#lin]->[XPOS]);
	}
	else
	{
	    $stream.=d3($xpos-$tmxpos)." 0 Td\n" if $mode eq 't';
	    $tmxpos=$xpos;
	}
    }
}

sub do_C
{
    my $par=shift;
    my $fnt=$fontlst{$cft}->{FNT};

    PutGlyph($fnt,$par,1) if $par ne 'space';
}

sub do_c
{
    my $par=shift;

    push(@ahead,substr($par,1));
    $par=substr($par,0,1);
    do_C($par);
}

sub do_N
{
    my $par=shift;
    my $fnt=$fontlst{$cft}->{FNT};

    if (!defined($fnt->{NO}->[$par]))
    {
	Warn("no chr($par) in font $fnt->{internalname}");
	return;
    }

    my $chnm=$fnt->{NO}->[$par];
    PutGlyph($fnt,$chnm,1) if $chnm ne 'space';
}

sub do_n
{
    $gotT=0;
    PutLine(0);
    $n_flg=1;
    @lin=();
    PutHotSpot($xpos) if defined($mark);
}

sub NextAlloc
{
    my $fnt=shift;

    $alloc=++$fnt->{ALLOC};

    my $maj=$alloc >> 8;
    my $min=$alloc & 0xff;

    my $start=($maj == 0)?128:33;
    $min=$start if $min < $start;
    $min++ if $min == ord('(');
    $min++ if $min == ord(')');
    $maj++,$min=$start if $min > 255;

    $fnt->{ALLOC}=($maj << 8) + $min;

    return($min,$maj);
}

sub decrypt_char
{
    my $l=shift;
    my (@la)=unpack('C*',$l);
    my @res;

    if ($lenIV >= 0)
    {
	my $clr;
	my $cr=C_DEF;
	my $skip=$lenIV;

	foreach my $cypher (@la)
	{
	    $clr=($cypher ^ ($cr >> 8)) & 0xFF;
	    $cr=(($cypher + $cr) * MAGIC1 + MAGIC2) & 0xFFFF;
	    push(@res,$clr) if --$skip < 0;
	}

	return(\@res);
    }
    else
    {
	return(\@la);
    }
}

sub decrypt_exec_P
{
    my $e=shift;
    my $l=shift;
    $l--;
    my $clr;
    my $er=E_DEF;

    foreach my $j (0..$l)
    {
	my $cypher=ord(substr($$e,$j,1));
	$clr=($cypher ^ ($er >> 8)) & 0xFF;
	$er=(($cypher + $er) * MAGIC1 + MAGIC2) & 0xFFFF;
	substr($$e,$j,1)=chr($clr);
    }

    return($e);
}

sub encrypt_exec
{
    my $la=shift;
    unshift(@{$la},0x44,0x65,0x72,0x69);
    my $res;
    my $cypher;
    my $er=E_DEF;

    foreach my $clr (@{$la})
    {
	$cypher=($clr ^ ($er >> 8)) & 0xFF;
	$er=(($cypher + $er) * MAGIC1 + MAGIC2) & 0xFFFF;
	$res.=pack('C',$cypher);
    }

    return($res);
}

sub encrypt_char
{
    my $la=shift;
    unshift(@{$la},0x44,0x65,0x72,0x69) if $lenIV;
    my $res;
    my $cypher;
    my $cr=C_DEF;

    foreach my $clr (@{$la})
    {
	$cypher=($clr ^ ($cr >> 8)) & 0xFF;
	$cr=(($cypher + $cr) * MAGIC1 + MAGIC2) & 0xFFFF;
	$res.=pack('C',$cypher);
    }

    return($res);
}

sub map_subrs
{
    my $lines=shift;
    my $stage=0;
    my $lin=$lines->[0];
    my $i=0;
    my ($RDre,$NDre);

    for (my $j=0; $j<=$#{$lines}; $lin=$lines->[++$j] )
    {
#	next if !defined($lines->[$j]);

	if ($stage == 0)
	{
	    if ($lin=~m/^\s*\/Subrs \d+/)
	    {
		$sec{'#Subrs'}=$j;
		$stage=1;
		$RDre=qr/\Q$RD\E/;
		$NDre=qr/\Q$ND\E/;
	    }
	    elsif ($lin=~m/^\/(.+?)\s*\{string currentfile exch readstring pop\}\s*executeonly def/)
	    {
		$RD=$1;
	    }
	    elsif ($lin=~m/^\/(.+?)\s*\{noaccess def\}\s*executeonly def/)
	    {
		$ND=$1;
	    }
	    elsif ($lin=~m/^\/(.+?)\s*\{noaccess put\}\s*executeonly def/)
	    {
		$NP=$1;
	    }
	    elsif ($lin=~m'^/lenIV\s+(\d+)')
	    {
		$lenIV=$1;
	    }
	}
	elsif ($stage == 1)
	{
	    if ($lin=~m/^\s*\d index \/CharStrings \d+/)
	    {
		$sec{'#CharStrings'}=$j;
		$stage=2;
		$i=0;
	    }
	    elsif ($lin=~m/^\s*dup\s+(\d+)\s+(\d+)\s+$RDre (.*)/s)
	    {
		my $n=$1;
		my $l=$2;
		my $s=$3;

		if (!exists($sec{"#$n"}))
		{
		    $sec{"#$n"}=[$j,{}];
		    $i=$j;
		    $sec{"#$n"}->[NEWNO]=$n if $n<=$newsub;
		}

		if (length($s) > $l)
		{
		    $s=substr($s,0,$l);
		}
		else
		{
		    $lin.=$term.$lines->[++$j];
		    $lines->[$j]=undef;
		    redo;
		}

#		$s=decrypt_char($s);
#		subs_call($s,"#$n");
		$lines->[$i]=["#$n",$l,$s,$NP];
	    }
	    elsif ($lin=~m/^$NDre/)
	    {}
	    else
	    {
		Warn("Don't understand '$lin'");
	    }
	}
	elsif ($stage == 2)
	{
	    if ($lin=~m/^0{64}/)
	    {
		$sec{'#Pad'}=$j;
		$stage=3;
	    }
	    elsif ($lin=~m/^\s*\/([-.\w]*)\s+(\d+)\s+$RDre (.*)/s)
	    {
		my $n=$1;
		my $l=$2;
		my $s=$3;

		$sec{"/$n"}=[$j,{}] if !exists($sec{"/$n"});

		if (length($s) > $l)
		{
		    $s=substr($s,0,$l);
		}
		else
		{
		    $lin.=$term.$lines->[++$j];
		    $lines->[$j]=undef;
		    $i--;
		    redo;
		}

		$i+=$j;

		if ($sec{"/$n"}->[0] != $i)
		{
		    # duplicate glyph name !!! discard ???
		    $lines->[$i]=undef;
		}
		else
		{
		    $lines->[$i]=["/$n",$l,$s,$ND];
		}

		$i=0;
	    }
#	    else
#	    {
#		Warn("Don't understand '$lin'");
#	    }
	}
	elsif ($stage == 3)
	{
	    if ($lin=~m/cleartomark/)
	    {
		$sec{'#cleartomark'}=[$j];
		$stage=4;
	    }
	    elsif ($lin!~m/^0+$/)
	    {
		Warn("Expecting padding - got '$lin'");
	    }
	}
    }
}

sub subs_call
{
    my $charstr=shift;
    my $key=shift;
    my $lines=shift;
    my @c;

    for (my $j=0; $j<=$#{$charstr}; $j++)
    {
	my $n=$charstr->[$j];

	if ($n >= 32 and $n <= 246)
	{
	    push(@c,[$n-139,1]);
	}
	elsif ($n >= 247 and $n <= 250)
	{
	    push(@c,[(($n-247) << 8)+$charstr->[++$j]+108,1]);
	}
	elsif ($n >= 251 and $n <= 254)
	{
	    push(@c,[-(($n-251) << 8)-$charstr->[++$j]-108,1]);
	}
	elsif ($n == 255)
	{
	    $n=($charstr->[++$j] << 24)+($charstr->[++$j] << 16)+($charstr->[++$j] << 8)+$charstr->[++$j];
	    $n=~$n if $n & 0x8000;
	    push(@c,[$n,1]);
	}
	elsif ($n == 10)
	{
	    if ($c[$#c]->[1])
	    {
		$c[$#c]->[0]=MarkSub("#$c[$#c]->[0]");
		$c[$#c-1]->[0]=MarkSub("#$c[$#c-1]->[0]") if ($c[$#c]->[0] == 4 and $c[$#c-1]->[1]);
	    }
	    push(@c,[10,0]);
	}
	elsif ($n == 12)
	{
	    push(@c,[12,0]);
	    my $n2=$charstr->[++$j];
	    push(@c,[$n2,0]);

	    if ($n2==16)	 # callothersub
	    {
		$c[$#c-4]->[0]=MarkSub("#$c[$#c-4]->[0]") if ($c[$#c-4]->[1]);
	    }
	    elsif ($n2==6)	 # seac
	    {
		my $ch=$StdEnc{$c[$#c-2]->[0]};
		my $chf;

		#	       if ($ch ne 'space')
		{
		    ($chf)=GetNAM($thisfnt,$ch);

		    if (!defined($chf->[MINOR]))
		    {
			AssignGlyph($thisfnt,$chf,$ch);
			Subset($lines,"$chf->[PSNAME]");
			push(@{$seac{$key}},"$ch");
		    }
		}

		$ch=$StdEnc{$c[$#c-3]->[0]};

		if ($ch ne 'space')
		{
		    ($chf)=GetNAM($thisfnt,$ch);

		    if (!defined($chf->[MINOR]))
		    {
			AssignGlyph($thisfnt,$chf,$ch);
			Subset($lines,"$chf->[PSNAME]");
			push(@{$seac{$key}},"$ch");
		    }
		}
	    }
	}
	else
	{
	    push(@c,[$n,0]);
	}
    }

    $sec{$key}->[CHARCHAR]=\@c;

#    foreach my $j (@c) {Warn("Undefined op in $key") if !defined($j);}
}

sub Subset
{
    my $lines=shift;
    my $glyphs=shift;
    my $extra=shift;

    foreach my $g ($glyphs=~m/(\/[.\w]+)/g)
    {
	if (exists($sec{$g}))
	{
	    $glyphseen{$g}=1;
	    $g='/space' if $g eq '/ ';

	    my $ln=$lines->[$sec{$g}->[LINE]];
	    subs_call($sec{$g}->[CHARCHAR]=decrypt_char($ln->[STR]),$g,$lines);

	    push(@glyphused,$g);
	}
	else
	{
	    Warn("Can't locate glyph '$g' in font") if $g ne '/space';
	}
    }
}

sub MarkSub
{
    my $k=shift;

    if (exists($sec{$k}))
    {
	if (!defined($sec{$k}->[NEWNO]))
	{
	    $sec{$k}->[NEWNO]=++$newsub;
	    push(@subrused,$k);

	    my $ln=$bl[$sec{$k}->[LINE]];
	    subs_call($sec{$k}->[CHARCHAR]=decrypt_char($ln->[STR]),$k,\@bl);
	}

	return($sec{$k}->[NEWNO]);
    }
    else
    {
	Warn("Missing Subrs '$k'");
    }
}

sub encrypt
{
    my $lines=shift;

    if (exists($sec{'#Subrs'}))
    {
	$newsub++;
	$lines->[$sec{'#Subrs'}]=~s/\d+\s+array/$newsub array/;
    }
    else
    {
	Warn("Unable to locate /Subrs");
    }

    if (exists($sec{'#CharStrings'}))
    {
	my $n=$#glyphused+1;
	$lines->[$sec{'#CharStrings'}]=~s/\d+\s+dict /$n dict /;
    }
    else
    {
	Warn("Unable to locate /CharStrings");
    }

    my $bdy;

    for (my $j=0; $j<=$#{$lines}; $j++)
    {
	my $lin=$lines->[$j];

	next if !defined($lin);

	if (ref($lin) eq 'ARRAY' and $lin->[TYPE] eq $NP)
	{
	    foreach my $sub (@subrused)
	    {
		if (exists($sec{$sub}))
		{
		    subs_call($sec{$sub}->[CHARCHAR]=decrypt_char($lines->[$sec{$sub}->[LINE]]->[STR]),$sub,$lines) if (!defined($sec{$sub}->[CHARCHAR]));
		    my $cs=encode_charstr($sec{$sub}->[CHARCHAR],$sub);
		    $bdy.="dup ".$sec{$sub}->[NEWNO].' '.length($cs)." $RD $cs $NP\n";
		}
		else
		{
		    Warn("Failed to locate Subr '$sub'");
		}
	    }

	    while (!defined($lines->[$j+1]) or ref($lines->[$j+1]) eq 'ARRAY') {$j++;};
	}
	elsif (ref($lin) eq 'ARRAY' and $lin->[TYPE] eq $ND)
	{
	    foreach my $chr (@glyphused)
	    {
		if (exists($sec{$chr}))
		{
		    my $cs=encode_charstr($sec{$chr}->[CHARCHAR],$chr);
		    $bdy.="$chr ".length($cs)." $RD $cs $ND\n";
		}
		else
		{
		    Warn("Failed to locate glyph '$chr'");
		}
	    }

	    while (!defined($lines->[$j+1]) or ref($lines->[$j+1]) eq 'ARRAY') {$j++;};
	}
	else
	{
	    $bdy.="$lin\n";
	}
    }

    my @bdy=unpack('C*',$bdy);
    return(encrypt_exec(\@bdy));
}

sub encode_charstr
{
    my $ops=shift;
    my $key=shift;
    my @c;

    foreach my $c (@{$ops})
    {
	my $n=$c->[0];
	my $num=$c->[1];

	if ($num)
	{
	    if ($n >= -107 and $n <= 107)
	    {
		push(@c,$n+139);
	    }
	    elsif ($n >= 108 and $n <= 1131)
	    {
		my $hi=($n - 108)>>8;
		my $lo=($n - 108) & 0xff;
		push(@c,$hi+247,$lo);
	    }
	    elsif ($n <= -108 and $n >= -1131)
	    {
		my $hi=abs($n + 108)>>8;
		my $lo=abs($n + 108) & 0xff;
		push(@c,$hi+251,$lo);
	    }
#	    elsif ($n >= -32768 and $n <= 32767)
#	    {
#		push(@c,28,($n>>8) & 0xff,$n & 0xff);
#	    }
	    else
	    {
		push(@c,255,($n >> 24) & 0xff, ($n >> 16) & 0xff,
		     ($n >> 8) & 0xff, $n & 0xff );
	    }
	}
	else
	{
	    push(@c, $n);
	}
    }

    return(encrypt_char(\@c));
}

sub SubTag
{
    my $res;

    foreach (1..6)
    {
	$res.=chr(int((rand(26)))+65);
    }

    return($res.'+');
}
1;

# Local Variables:
# fill-column: 72
# mode: CPerl
# End:
# vim: set cindent noexpandtab shiftwidth=4 softtabstop=4 textwidth=72:
