.ig
	front.t
..
.
.if !'\*[.T]'pdf' .nx
.
.nr PDFOUTLINE.FOLDLEVEL 1
.defcolor pdf:href.colour rgb 0.00 0.25 0.75
.pdfinfo /Title "groff Collected Reference Pages"
.de an*cln
.  ds \\$1
.  als an*cln:res \\$1
.  shift
.  ds an*cln:res \\$*\"
.  ds an*cln:chr \\$*
.  substring an*cln:chr 0 0
.  if '\%'\\*[an*cln:chr]' \{\
.    substring an*cln:res 1
.  \}
..
.
.de END
..
.
.am reload-man END
.de an*bookmark
.  ie (\\\\$1=1) \{\
.     an*cln an*page-ref-nm \\\\$2\"
.     pdfbookmark -T "\\\\*[an*page-ref-nm]" \\\\$1 \\\\$2
.  \}
.  el .pdfbookmark \\\\$1 \\\\$2
..
.
.de1 MR
.  if ((\\\\n[.$] < 2) : (\\\\n[.$] > 3)) \
.    an-style-warn .\\\\$0 expects 2 or 3 arguments, got \\\\n[.$]
.  ds an*url man:\\\\$1(\\\\$2)\" used everywhere but macOS
.  if (\\\\n[an*MR-URL-format] = 2) \
.    ds an*url x-man-page://\\\\$2/\\\\$1\" macOS/Mac OS X since 10.3
.  if (\\\\n[an*MR-URL-format] = 3) \
.    ds an*url man:\\\\$1.\\\\$2\" Bwana (Mac OS X)
.  if (\\\\n[an*MR-URL-format] = 4) \
.    ds an*url x-man-doc://\\\\$2/\\\\$1\" ManOpen (Mac OS X pre-2005)
.  nh
.  ie \\\\n(.$=1 \{\
.    ft \\\\*[MF]
.    nop \\\\$1
.    ft
.  \}
.  el \{\
.    an*cln an*page-ref-nm \\\\$1(\\\\$2)
.    ie d pdf:look(\\\\*[an*page-ref-nm]) \
.      pdfhref L -D \\\\*[an*page-ref-nm] -A "\\\\$3" -- \f[\\\\*[MF]]\\\\$1\f[](\\\\$2)
.    el \{\
.      ds an*saved-stroke-color \\\\n[.m]\"
.      nop \&\m[\\\\*[PDFHREF.TEXT.COLOUR]]\c
.      pdfhref W -D \\\\*[an*url] -- "|"
.      nop \&\\\\*[an-lic]\f[\\\\*[MF]]\\\\$1\\\\*[an-ic]\f[R](\\\\$2)\c
.      nop \X'pdf: markend'\m[\\\\*[an*saved-stroke-color]]\c
.      rm an*saved-stroke-color
.      nop \&\\\\$3
.    \}
.  \}
.  hy \\\\n[an*hyphenation-mode]
..
.END
.
.de Hl
.br
\l'\\n[.l]u-\\n[.i]u\&\\$1'
.br
..
\Z@\D't 8p'@
.pdfbookmark 1 Cover
.pdfpagenumbering
.sp 2i
.Hl
.sp .6i
.ad r
.nr GMP*saved-type-size \n[.ps]
.ps 52
.gcolor maroon
groff
.gcolor
.sp 18p
.ps 16
.ft BMB
Collected Reference Pages
.ft
.ps \n[GMP*saved-type-size]u
.sp .2i
.Hl
.bp 1
.pdfpagenumbering D . 1
