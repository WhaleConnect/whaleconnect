#
# Makefile to build a version of GNU Unifont with the
# unifoundry.com GNU Unifont utilities.  This Makefile
# converts unifont-$(VERSION).hex and related files into a final
# GNU Unifont font file.
#
# This software is released under version 2.0 of the GNU Public License,
# or (at your option) a later version of the GPL.
#
# Paul Hardy, 7 July 2008
#
# commands will be interpreted with the Bourne Shell
#
SHELL = /bin/sh
INSTALL = install
GZFLAGS = -f -9 -n
FONTFORGE = fontforge
PERL = 
#
VERSION = 13.0.06

COPYRIGHT = "Copyright (C) 1998-2021 Roman Czyborra, Paul Hardy, \
Qianqian Fang, Andrew Miller, Johnnie Weaver, David Corbett, \
Nils Moskopp, Rebecca Bettencourt, et al. \
License: SIL Open Font License version 1.1 and GPLv2+: \
GNU GPL version 2 or later <http://gnu.org/licenses/gpl.html> \
with the GNU Font Embedding Exception."

#
# Path to local unifont-bin utilities.  If your system doesn't
# use GNU's version of "make", use the first BINDIR definition.
# If using GNU's "make", use the second BINDIR definition.
#
BINDIR = ../bin
# BINDIR = $(CURDIR)/../bin
#
# The top-level directory for installing fonts on the system,
# and the installation directories for PCF and TrueType fonts.
#
CONSOLEDEST = $(DESTDIR)/usr/share/consolefonts
FONTDEST    = $(DESTDIR)/usr/share/fonts
PCFDEST     = $(FONTDEST)/X11/misc
TTFDEST     = $(FONTDEST)/truetype/unifont
#
# destination directory for compiled fonts
#
COMPILED_DIR = compiled
#
# destination directory for .bmp or .png representation of font
#
BMPDIR = $(COMPILED_DIR)/bmp
PNGDIR = $(COMPILED_DIR)/png

# Choose one graphics format or neither.
GRAPHICS=png
# GRAPHICS=bmp
#
# Directory with original unifont-$(VERSION).hex files
#
HEXDIR = plane00
#
# These are the files for building GNU Unifont with the Qianqian Fang's
# Wen Quan Yi CJK ideographs.  This version provides complete coverage
# of the Unicode Basic Multilingual Plane.
#
# If you don't want to include blank glyphs in unassigned code points,
# uncomment the UNASSIGNED definition or override from the make command line.
#
# Likewise, uncomment the PUA definition if you want to use Private
# Use Area glyphs.
#
UNASSIGNED = $(HEXDIR)/plane00-unassigned.hex
# UNASSIGNED =

#
# Non-printing glyphs.  There are only about 100 of these, and many end
# users do want to print representations of these glyphs, so they are
# included as an optional assignment.
#
NONPRINTING = $(HEXDIR)/plane00-nonprinting.hex
# NONPRINTING =

#
# Private Use Area glyphs.  Uncomment to include four-digit hexadecimal glyphs
# or override from the make command line.
#
# PUA = $(HEXDIR)/pua.hex
PUA = 

# The remaining .hex files will be constant unless a customized font
# is being built.  Note that COPYLEFT contains the encoding for U+01F12F,
# which is in Unicode Plane 1 rather than Plane 0.  It is included in the
# base Plane 0 Unifont TTF font as an exception because of its significance.
UNIFONTBASE = $(HEXDIR)/unifont-base.hex
CJK         = $(HEXDIR)/wqy.hex
HANGUL      = $(HEXDIR)/hangul-syllables.hex
SPACES      = $(HEXDIR)/spaces.hex
CUSTOM00    = $(HEXDIR)/custom00.hex
COPYLEFT    = $(HEXDIR)/copyleft.hex

UNIFILES = $(UNIFONTBASE) $(CJK) $(HANGUL) $(SPACES) $(UNASSIGNED) \
	   $(NONPRINTING) $(PUA) $(CUSTOM00)

#
# All Plane 0 glyphs except for the JIS X 0213 glyph file, for
# creating the unifont_sample file.
#
PLANE0 = copyleft.hex custom00.hex hangul-syllables.hex \
	omit.hex plane00-nonprinting.hex plane00-unassigned.hex \
	pua.hex spaces.hex unifont-base.hex wqy.hex

#
# JIS X 0213 hex files for Japanese.  The plane00 file contains glyphs
# that will replace those in the default Chinese version of Unifont.
#
JISHEX = plane00/izmg16-plane00.hex plane02/izmg16-plane02.hex
PLANE_2_HEX = plane02/izmg16-plane02.hex

#
# Planes 1 through 14 (0x0E) are ordinary;
# Planes 15 (0x0F) and 16 (0x10) are Private Use Area
#
UPPER_FILES = plane0[1-E]/*.hex

#
# Location of the file containing a list of Unicode combining characters.
#
COMBINING = plane00/plane00-combining.txt

UPPER_COMBINING = plane0[1-E]/*combining.txt

VPATH = plane00 ttfsrc

#
# Location of  TTF source directory, where TTF font is built.
#
TTFSRC = ttfsrc


all: compiled

compiled: $(UNIFILES) $(COMBINING) $(UPPER_FILES) $(UPPER_COMBINING)
	$(MAKE) compiled-files

#
# Only use PSF target if on GNU/Linux, not on BSD, because
# a BSD console font only allows 256 code points, not 512.
#
PSF = psf
# PSF =
compiled-files: pcf $(PSF) $(GRAPHICS) ttf csurttf upperttf bigpic coverage
	install -p index.html $(COMPILED_DIR)/index.html

#
# Build the aggregate .hex font files
#
hex:
	if [ ! -d $(COMPILED_DIR) ] ; then \
	   mkdir -p $(COMPILED_DIR) ; \
	fi
	sort $(UNIFILES) > $(COMPILED_DIR)/unifont-$(VERSION).hex
	(cd $(HEXDIR) && sort $(PLANE0)) | \
	  egrep -v "^FFF[EF]" | grep -v "^01F12F:" | \
	   $(BINDIR)/unigencircles $(COMBINING) plane00/plane00-nonprinting.hex \
	   > $(COMPILED_DIR)/unifont_sample-$(VERSION).hex
	sort $(UPPER_FILES) \
	   > $(COMPILED_DIR)/unifont_upper-$(VERSION).hex
	sort $(UPPER_COMBINING) > $(COMPILED_DIR)/upper_combining.txt
	sort plane0[1-E]/plane*-nonprinting.hex \
	   > $(COMPILED_DIR)/upper_nonprinting.hex
	# Generate files in plane01/all directory to create font sample book.
	(cd plane01 && make)
	sort plane01/all/plane01-all.hex plane0[2-E]/*.hex | egrep -v "^..FFF[EF]" | \
	   $(BINDIR)/unigencircles $(COMPILED_DIR)/upper_combining.txt \
	   $(COMPILED_DIR)/upper_nonprinting.hex \
	   > $(COMPILED_DIR)/unifont_upper_sample-$(VERSION).hex
	# Create a .hex file with CSUR glyphs, without combining circles
	(cd plane00csur && make)
	sort plane00csur/all/plane00csur-all.hex \
	     $(COMPILED_DIR)/unifont-$(VERSION).hex \
	   > $(COMPILED_DIR)/unifont_csur_lower-$(VERSION).hex
	# Create a .hex file with only CSUR glyphs from all planes
	sort plane00csur/all/plane00csur-all.hex | sed -e 's/^/00/' \
	   > $(COMPILED_DIR)/unifont_csur-$(VERSION).hex
	sort plane0[1-F]csur/*.hex \
	   >> $(COMPILED_DIR)/unifont_csur-$(VERSION).hex
	# Create a .hex file with all CSUR glyphs, with combining circles
	sort plane00csur/plane00csur-combining.txt \
	   >  $(COMPILED_DIR)/csur_combining.txt
	sort plane0[1-F]csur/plane0[1-F]csur-combining.txt \
	   >> $(COMPILED_DIR)/csur_combining.txt
	$(BINDIR)/unigencircles \
	     $(COMPILED_DIR)/csur_combining.txt \
	     plane00/plane00-nonprinting.hex \
	   < $(COMPILED_DIR)/unifont_csur-$(VERSION).hex \
	   > $(COMPILED_DIR)/unifont_csur_sample-$(VERSION).hex
	# Build Japanese version with glyphs from JIS X 0213:2004
	awk -F: -f unipatch-hex.awk $(COMPILED_DIR)/unifont-$(VERSION).hex \
	   plane00/izmg16-plane00.hex | sort |\
	   tee $(COMPILED_DIR)/unifont_jp-$(VERSION).hex | \
	   $(BINDIR)/unigencircles $(COMBINING) plane00/plane00-nonprinting.hex \
              > $(COMPILED_DIR)/unifont_jp_sample-$(VERSION).hex
	cat $(PLANE_2_HEX) \
	   >> $(COMPILED_DIR)/unifont_jp-$(VERSION).hex

#
# Build a BDF font file from the final .hex file.
#
bdf: hex
	# First make the default BDF font.  The font name will be "unifont".
	$(BINDIR)/hex2bdf --version "$(VERSION)" --copyright $(COPYRIGHT) \
	   $(COMPILED_DIR)/unifont-$(VERSION).hex \
	   >$(COMPILED_DIR)/unifont-$(VERSION).bdf
	gzip $(GZFLAGS) <$(COMPILED_DIR)/unifont-$(VERSION).bdf \
	           >$(COMPILED_DIR)/unifont-$(VERSION).bdf.gz
	# Now make a version with combining circles.  The font name
	# will be "unifont_sample" instead of "unifont" to distinguish
	# it from the default font.
	$(BINDIR)/hex2bdf --font "Unifont Sample" \
	   --version "$(VERSION)" --copyright $(COPYRIGHT) \
	     $(COMPILED_DIR)/unifont_sample-$(VERSION).hex \
	   > $(COMPILED_DIR)/unifont_sample-$(VERSION).bdf
	gzip $(GZFLAGS) <$(COMPILED_DIR)/unifont_sample-$(VERSION).bdf \
	           >$(COMPILED_DIR)/unifont_sample-$(VERSION).bdf.gz
	# Font with ConScript Unicode Registry (CSUR) glyphs added
	$(BINDIR)/hex2bdf --font "Unifont CSUR" \
	   --version "$(VERSION)" --copyright $(COPYRIGHT) \
	     $(COMPILED_DIR)/unifont_csur_lower-$(VERSION).hex \
	   > $(COMPILED_DIR)/unifont_csur-$(VERSION).bdf
	gzip $(GZFLAGS) <$(COMPILED_DIR)/unifont_csur-$(VERSION).bdf \
	           >$(COMPILED_DIR)/unifont_csur-$(VERSION).bdf.gz
	# Font with glyphs above Plane 0
	$(BINDIR)/hex2bdf --font "Unifont Upper" \
	   --version "$(VERSION)" --copyright $(COPYRIGHT) \
	     $(COMPILED_DIR)/unifont_upper-$(VERSION).hex \
	   > $(COMPILED_DIR)/unifont_upper-$(VERSION).bdf
	gzip $(GZFLAGS) <$(COMPILED_DIR)/unifont_upper-$(VERSION).bdf \
	           >$(COMPILED_DIR)/unifont_upper-$(VERSION).bdf.gz
	# Font with glyphs above Plane 0 with combining circles added
	$(BINDIR)/hex2bdf --font "Unifont Upper Sample" \
	   --version "$(VERSION)" --copyright $(COPYRIGHT) \
	     $(COMPILED_DIR)/unifont_upper_sample-$(VERSION).hex \
	   > $(COMPILED_DIR)/unifont_upper_sample-$(VERSION).bdf
	gzip $(GZFLAGS) <$(COMPILED_DIR)/unifont_upper_sample-$(VERSION).bdf \
	           >$(COMPILED_DIR)/unifont_upper_sample-$(VERSION).bdf.gz
	# Font with Japanese glyphs from JIS X 0213 standard
	$(BINDIR)/hex2bdf --font "Unifont Japanese" \
	   --version "$(VERSION)" --copyright $(COPYRIGHT) \
	     $(COMPILED_DIR)/unifont_jp-$(VERSION).hex \
	   > $(COMPILED_DIR)/unifont_jp-$(VERSION).bdf
	gzip $(GZFLAGS) <$(COMPILED_DIR)/unifont_jp-$(VERSION).bdf \
	           >$(COMPILED_DIR)/unifont_jp-$(VERSION).bdf.gz
	# Same Japanese version, but only with Plane 0 glyphs for bdftopcf
	grep "^....:" $(COMPILED_DIR)/unifont_jp-$(VERSION).hex |\
	   $(BINDIR)/hex2bdf --font "Unifont Japanese" \
	      --version "$(VERSION)" --copyright $(COPYRIGHT) \
	      > $(COMPILED_DIR)/unifont_jp0-$(VERSION).bdf
	gzip $(GZFLAGS) <$(COMPILED_DIR)/unifont_jp0-$(VERSION).bdf \
	           >$(COMPILED_DIR)/unifont_jp0-$(VERSION).bdf.gz

#
# Build a PCF font file from the final .hex file.
#
pcf: bdf
	bdftopcf <$(COMPILED_DIR)/unifont-$(VERSION).bdf \
	         >$(COMPILED_DIR)/unifont-$(VERSION).pcf
	gzip $(GZFLAGS) $(COMPILED_DIR)/unifont-$(VERSION).pcf
	bdftopcf <$(COMPILED_DIR)/unifont_sample-$(VERSION).bdf \
	         >$(COMPILED_DIR)/unifont_sample-$(VERSION).pcf
	gzip $(GZFLAGS) $(COMPILED_DIR)/unifont_sample-$(VERSION).pcf
	bdftopcf <$(COMPILED_DIR)/unifont_csur-$(VERSION).bdf \
	         >$(COMPILED_DIR)/unifont_csur-$(VERSION).pcf
	gzip $(GZFLAGS) $(COMPILED_DIR)/unifont_csur-$(VERSION).pcf
	bdftopcf <$(COMPILED_DIR)/unifont_jp0-$(VERSION).bdf \
	         >$(COMPILED_DIR)/unifont_jp-$(VERSION).pcf
	gzip $(GZFLAGS) $(COMPILED_DIR)/unifont_jp-$(VERSION).pcf

#
# Make the PSF (console) font for APL (A Programming Language).
#
psf: bdf
	bdf2psf --fb \
	   $(COMPILED_DIR)/unifont-$(VERSION).bdf \
	   psf/apl-equivalents.txt \
	   psf/unifont-apl.txt \
	   512 \
	   $(COMPILED_DIR)/Unifont-APL8x16-$(VERSION).psf
	gzip $(GZFLAGS) $(COMPILED_DIR)/Unifont-APL8x16-$(VERSION).psf

#
# Print coverage of all scripts in Unifont.  Combine all Plane 0
# directories first to keep everything ordered properly.  Only keep
# the space glyph from the Plane 0 Unifont source hex file; it is
# duplicated in other files used to create separate fonts.  Don't
# use plane00/pua.hex (U+E000..U+F8FF) because we are including
# Plane 0 CSUR glyphs.  The built unifont_csur_lower-$(VERSION).hex
# includes all non-Private Use Area Plane 0 glyphs, so start with it
# as a baseline.  Remove the Plane 1 Copyleft glyph from the Plane 0
# set of hex files.
#
coverage: bdf
	grep -v "^01F12F" $(COMPILED_DIR)/unifont_csur_lower-$(VERSION).hex \
	   > $(COMPILED_DIR)/unifont_all-$(VERSION).hex
	sort plane0[1-E]/*.hex plane0Fcsur/*.hex | grep -v "^0020:" \
	   >> $(COMPILED_DIR)/unifont_all-$(VERSION).hex
	$(BINDIR)/unicoverage \
	   -i$(COMPILED_DIR)/unifont_all-$(VERSION).hex \
	   -o$(COMPILED_DIR)/coverage.txt
	\rm -f $(COMPILED_DIR)/unifont_all-$(VERSION).hex
#
# Print HTML page coverage in Basic Multilingual Plane in .txt file.
#
pagecount: $(COMPILED_DIR)/unifont-$(VERSION).hex $(BINDIR)/unipagecount
	$(BINDIR)/unipagecount -l < $(COMPILED_DIR)/unifont-$(VERSION).hex \
		                  > $(COMPILED_DIR)/pagecount.html

#
# Create the .png (Portable Network Graphics) versions of the glyphs.
#
png: hex $(BINDIR)/unihex2png
	if [ ! -d $(PNGDIR) ] ; then \
	   mkdir -p $(PNGDIR) ; \
	fi
	if [ ! -d $(PNGDIR)/plane00 ] ; then \
	   mkdir -p $(PNGDIR)/plane00 ; \
	fi
	for i in 0 1 2 3 4 5 6 7 8 9 A B C D E F; do \
	   for j in 0 1 2 3 4 5 6 7 8 9 A B C D E F; do \
	      $(PERL) $(BINDIR)/unihex2png -p 00$$i$$j \
	         -i $(COMPILED_DIR)/unifont_sample-$(VERSION).hex \
	         -o $(PNGDIR)/plane00/uni00$$i$$j.png ; \
	   done ; \
	done
	if [ ! -d $(PNGDIR)/plane00-jp ] ; then \
	   mkdir -p $(PNGDIR)/plane00-jp ; \
	fi
	for i in 0 1 2 3 4 5 6 7 8 9 A B C D E F; do \
	   for j in 0 1 2 3 4 5 6 7 8 9 A B C D E F; do \
	      $(PERL) $(BINDIR)/unihex2png -p 00$$i$$j \
	         -i $(COMPILED_DIR)/unifont_jp_sample-$(VERSION).hex \
	         -o $(PNGDIR)/plane00-jp/uni00$$i$$j-jp.png ; \
	   done ; \
	done
	if [ ! -d $(PNGDIR)/plane01 ] ; then \
	   mkdir -p $(PNGDIR)/plane01 ; \
	fi
	for i in 0 1 2 3 4 5 6 7 8 9 A B C D E F; do \
	   for j in 0 1 2 3 4 5 6 7 8 9 A B C D E F; do \
	      $(PERL) $(BINDIR)/unihex2png -p 01$$i$$j \
	         -i $(COMPILED_DIR)/unifont_upper_sample-$(VERSION).hex \
	         -o $(PNGDIR)/plane01/uni01$$i$$j.png ; \
	   done ; \
	done
	if [ ! -d $(PNGDIR)/plane02-jp ] ; then \
	   mkdir -p $(PNGDIR)/plane02-jp ; \
	fi
	for i in 0 1 2 3 4 5 6 7 8 9 A B C D E F; do \
	   for j in 0 1 2 3 4 5 6 7 8 9 A B C D E F; do \
	      $(PERL) $(BINDIR)/unihex2png -p 02$$i$$j \
	         -i $(PLANE_2_HEX) \
	         -o $(PNGDIR)/plane02-jp/uni02$$i$$j-jp.png ; \
	   done ; \
	done
	if [ ! -d $(PNGDIR)/plane0E ] ; then \
	   mkdir -p $(PNGDIR)/plane0E ; \
	fi
	for i in 0 1 2 3 4 5 6 7 8 9 A B C D E F; do \
	   for j in 0 1 2 3 4 5 6 7 8 9 A B C D E F; do \
	      $(PERL) $(BINDIR)/unihex2png -p 0E$$i$$j \
	         -i $(COMPILED_DIR)/unifont_upper_sample-$(VERSION).hex \
	         -o $(PNGDIR)/plane0E/uni0E$$i$$j.png ; \
	   done ; \
	done
	if [ ! -d $(PNGDIR)/csur ] ; then \
	   mkdir -p $(PNGDIR)/csur ; \
	fi
	for i in E0 E1 E2 E3 E4 E5 E6 E7 E8 E9 EA EB EC ED EE EF \
	         F0 F1 F2 F3 F4 F5 F6 F7 F8 ; do \
	      $(PERL) $(BINDIR)/unihex2png -p 00$$i \
	         -i $(COMPILED_DIR)/unifont_csur_sample-$(VERSION).hex \
	         -o $(PNGDIR)/csur/uni00$$i.png ; \
	done
	for i in 0 1 2 3 4 5 6 7 8 9 A B C D E F; do \
	   for j in 0 1 2 3 4 5 6 7 8 9 A B C D E F; do \
	      $(PERL) $(BINDIR)/unihex2png -p 0F$$i$$j \
	         -i $(COMPILED_DIR)/unifont_csur_sample-$(VERSION).hex \
	         -o $(PNGDIR)/csur/uni0F$$i$$j.png ; \
	   done ; \
	done
	# Create HTML table showing color-coded percent coverage in Plane 0.
	$(BINDIR)/unipagecount -h < $(COMPILED_DIR)/unifont-$(VERSION).hex | \
	   sed -e 's/<a /<a target="blank" /g' \
	                          > $(COMPILED_DIR)/plane00.html
	# Create HTML table showing color-coded percent coverage in Japanese Plane 0.
	$(BINDIR)/unipagecount -h < $(COMPILED_DIR)/unifont_jp_sample-$(VERSION).hex | \
	   sed -e 's/Glyphs/Glyphs \&mdash; Japanese Version/' | \
	   sed -e 's/<a /<a target="blank" /g' | \
	   sed -e 's/plane00/plane00-jp/g' | \
	   sed -e 's/\.png/-jp.png/g' \
	                          > $(COMPILED_DIR)/plane00-jp.html
	# Create HTML table showing color-coded percent coverage in Plane 1.
	../bin/unipagecount -P1 -h < plane01/all/plane01-all.hex | \
	   sed -e 's/<a /<a target="blank" /g' > compiled/plane01.html
	# Copy Plane 2 HTML table, specially formatted to show empty
	# ranges as gray because there are so many empty ranges.
	cp plane02-jp.html $(COMPILED_DIR)
	@echo "Done creating $(PNGDIR)"

#
# Create the .bmp (Bitmap Graphics) versions of the glyphs.
#
bmp: hex $(BINDIR)/unihex2bmp
	if [ ! -d $(BMPDIR) ] ; then \
	   mkdir -p $(BMPDIR) ; \
	fi
	if [ ! -d $(BMPDIR)/plane00 ] ; then \
	   mkdir -p $(BMPDIR)/plane00 ; \
	fi
	for i in 0 1 2 3 4 5 6 7 8 9 A B C D E F; do \
	   for j in 0 1 2 3 4 5 6 7 8 9 A B C D E F; do \
	      $(BINDIR)/unihex2bmp -p00$$i$$j \
	         < $(COMPILED_DIR)/unifont_sample-$(VERSION).hex \
	         > $(BMPDIR)/plane00/uni00$$i$$j.bmp ; \
	   done ; \
	done
	if [ ! -d $(BMPDIR)/plane00-jp ] ; then \
	   mkdir -p $(BMPDIR)/plane00-jp ; \
	fi
	for i in 0 1 2 3 4 5 6 7 8 9 A B C D E F; do \
	   for j in 0 1 2 3 4 5 6 7 8 9 A B C D E F; do \
	      $(BINDIR)/unihex2bmp -p00$$i$$j \
	         < $(COMPILED_DIR)/unifont_jp_sample-$(VERSION).hex \
	         > $(BMPDIR)/plane00-jp/uni00$$i$$j-jp.bmp ; \
	   done ; \
	done
	if [ ! -d $(BMPDIR)/plane01 ] ; then \
	   mkdir -p $(BMPDIR)/plane01 ; \
	fi
	for i in 0 1 2 3 4 5 6 7 8 9 A B C D E F; do \
	   for j in 0 1 2 3 4 5 6 7 8 9 A B C D E F; do \
	      $(BINDIR)/unihex2bmp -p01$$i$$j \
	         < $(COMPILED_DIR)/unifont_upper_sample-$(VERSION).hex \
	         > $(BMPDIR)/plane01/uni01$$i$$j.bmp ; \
	   done ; \
	done
	if [ ! -d $(BMPDIR)/plane02-jp ] ; then \
	   mkdir -p $(BMPDIR)/plane02-jp ; \
	fi
	for i in 0 1 2 3 4 5 6 7 8 9 A B C D E F; do \
	   for j in 0 1 2 3 4 5 6 7 8 9 A B C D E F; do \
	      $(BINDIR)/unihex2bmp -p02$$i$$j \
	         < $(PLANE_2_HEX) \
	         > $(BMPDIR)/plane02-jp/uni02$$i$$j-jp.bmp ; \
	   done ; \
	done
	if [ ! -d $(BMPDIR)/plane0E ] ; then \
	   mkdir -p $(BMPDIR)/plane0E ; \
	fi
	for i in 0 1 2 3 4 5 6 7 8 9 A B C D E F; do \
	   for j in 0 1 2 3 4 5 6 7 8 9 A B C D E F; do \
	      $(BINDIR)/unihex2bmp -p0E$$i$$j \
	         < $(COMPILED_DIR)/unifont_upper_sample-$(VERSION).hex \
	         > $(BMPDIR)/plane0E/uni0E$$i$$j.bmp ; \
	   done ; \
	done
	if [ ! -d $(BMPDIR)/csur ] ; then \
	   mkdir -p $(BMPDIR)/csur ; \
	fi
	for i in E0 E1 E2 E3 E4 E5 E6 E7 E8 E9 EA EB EC ED EE EF \
	         F0 F1 F2 F3 F4 F5 F6 F7 F8 ; do \
	      $(BINDIR)/unihex2bmp -p00$$i \
	         < $(COMPILED_DIR)/unifont_csur_sample-$(VERSION).hex \
	         > $(BMPDIR)/csur/uni00$$i.bmp ; \
	done
	for i in 0 1 2 3 4 5 6 7 8 9 A B C D E F; do \
	   for j in 0 1 2 3 4 5 6 7 8 9 A B C D E F; do \
	      $(BINDIR)/unihex2bmp -p0F$$i$$j \
	         < $(COMPILED_DIR)/unifont_csur_sample-$(VERSION).hex \
	         > $(BMPDIR)/csur/uni0F$$i$$j.bmp ; \
	   done ; \
	done
	# Create HTML table showing color-coded percent coverage in Plane 0.
	$(BINDIR)/unipagecount -h < $(COMPILED_DIR)/unifont-$(VERSION).hex | \
	   sed -e 's/<a /<a target="blank" /g' | \
	   sed -e 's/png/bmp/g'   > $(COMPILED_DIR)/plane00.html
	# Create HTML table showing color-coded percent coverage in Japanese Plane 0.
	$(BINDIR)/unipagecount -h < $(COMPILED_DIR)/unifont_jp_sample-$(VERSION).hex | \
	   sed -e 's/Glyphs/Glyphs \&mdash; Japanese Version/' | \
	   sed -e 's/<a /<a target="blank" /g' | \
	   sed -e 's/plane00/plane00-jp/g' | \
	   sed -e 's/png/bmp/g' | \
	   sed -e 's/\.bmp/-jp.bmp/g' \
	                          > $(COMPILED_DIR)/plane00-jp.html
	# Create HTML table showing color-coded percent coverage in Plane 1.
	$(BINDIR)/unipagecount -P1 -h < plane01/all/plane01-all.hex | \
	   sed -e 's/<a /<a target="blank" /g' | \
	   sed -e 's/png/bmp/g'   > $(COMPILED_DIR)/plane01.html
	# Copy Plane 2 HTML table, specially color coded to show empty
	# ranges as gray because there are so many empty ranges in Plane 2..
	sed -e 's/png/bmp/g' < plane02-jp.html > $(COMPILED_DIR)/plane02-jp.html
	echo "Done creating $(BMPDIR)"

#
# Build one bitmap of the entire Unifont Plane 0 set of glyphs.
# Use all the .hex files in $(HEXDIR) so we add the contents of
# omit.hex (FFFE & FFFF).  Those two glyphs aren't included in the
# unifont_sample font because they cause problems with MS Windows,
# but we can put them in the picture.  Also add the Plane 0
# ConScript Unicode Registry (CSUR) glyphs in the Private Use Area.
# The CSUR glyphs are optional, appearing in the unifont_csur font
# files rather than the main unifont font files.
#
bigpic: hex
	cat $(COMPILED_DIR)/unifont_sample-$(VERSION).hex \
	    $(COMPILED_DIR)/unifont_csur_sample-$(VERSION).hex | \
	    grep -v "^01F12F" | \
	    $(BINDIR)/unifontpic -d120 \
	       > $(COMPILED_DIR)/unifont-$(VERSION).bmp
	$(BINDIR)/unifontpic -d120 -P1 \
	   < $(COMPILED_DIR)/unifont_upper_sample-$(VERSION).hex \
	   > $(COMPILED_DIR)/unifont_plane1-$(VERSION).bmp
	$(BINDIR)/unifontpic -d120 -P2 \
	   < $(PLANE_2_HEX) \
	   > $(COMPILED_DIR)/unifont_plane2-$(VERSION).bmp

#
# Note that $(TTFSRC) must exist, because it contains some source files
# Perform a "make && make clean" because ALL of the files would consume
# over 200 Megabytes if left around.  The .sfd file is about 100 Megabytes,
# and it is created from merging "[0-F].sfd", which take up another 100 MB.
#

ttf: hex
	# First, copy the ordinary version, to make a TrueType font.
	install -p \
		$(COMPILED_DIR)/unifont-$(VERSION).hex \
		$(TTFSRC)/unifont.hex
	# Add the Copyleft symbol to the Plane 0 Unifont TrueType font
	cat $(COPYLEFT) >> $(TTFSRC)/unifont.hex
	install -p \
		$(HEXDIR)/plane00-combining.txt \
		$(TTFSRC)/combining.txt
	cd $(TTFSRC) && \
	   $(MAKE) FONTFILE="unifont" COMBINING="combining" \
		FONTNAME="Unifont" PSNAME="Unifont"
	mv $(TTFSRC)/unifont.sfd $(COMPILED_DIR)/unifont-$(VERSION).sfd 
	mv $(TTFSRC)/unifont.ttf \
		$(COMPILED_DIR)/unifont-$(VERSION).ttf 
	# Second, copy unifont_sample.bdf, to make an SBIT font.
	install -p \
		$(COMPILED_DIR)/unifont_sample-$(VERSION).bdf \
		$(TTFSRC)/unifont_sample.bdf
	cd $(TTFSRC) && \
	   $(MAKE) sbit FONTFILE="unifont_sample" COMBINING="" \
		FONTNAME="Unifont Sample" PSNAME="UnifontSample"
	mv $(TTFSRC)/unifont_sample.ttf \
		$(COMPILED_DIR)/unifont_sample-$(VERSION).ttf 
	# Third, make a version with Japanese glyphs as drawn in
	# JIS X 0213:2004, adding Copyleft symbol.
	sort $(COPYLEFT) $(COMPILED_DIR)/unifont_jp-$(VERSION).hex \
		> $(TTFSRC)/unifont_jp.hex
	install -p \
		$(HEXDIR)/plane00-combining.txt \
		$(TTFSRC)/combining.txt
	cd $(TTFSRC) && \
	   $(MAKE) FONTFILE="unifont_jp" COMBINING="combining" \
		FONTNAME="Unifont-JP" PSNAME="Unifont-JP"
	mv $(TTFSRC)/unifont_jp.sfd $(COMPILED_DIR)/unifont_jp-$(VERSION).sfd 
	mv $(TTFSRC)/unifont_jp.ttf \
		$(COMPILED_DIR)/unifont_jp-$(VERSION).ttf 

#
# Now build the ConScript Unicode Registry PUA font.
#
csurttf: hex
	install -p \
		$(COMPILED_DIR)/unifont_csur-$(VERSION).hex \
		$(TTFSRC)/unifont_csur.hex
	sort -u plane00csur/plane00csur-combining.txt \
		>  $(TTFSRC)/combining_csur.txt
	sort -u plane0[1-F]csur/plane0[1-F]csur-combining.txt \
		>> $(TTFSRC)/combining_csur.txt
	cd $(TTFSRC) && \
	   $(MAKE) outline FONTFILE="unifont_csur" COMBINING="combining_csur" \
		FONTNAME="Unifont CSUR" PSNAME="UnifontCSUR"
	mv $(TTFSRC)/unifont_csur.sfd \
		$(COMPILED_DIR)/unifont_csur-$(VERSION).sfd 
	mv $(TTFSRC)/unifont_csur.ttf \
		$(COMPILED_DIR)/unifont_csur-$(VERSION).ttf 
	gzip $(GZFLAGS) $(COMPILED_DIR)/unifont_csur-$(VERSION).sfd 

#
# Now build the ConScript Unicode Registry PUA font.
#
upperttf: hex
	# First, copy the ordinary version, to make a TrueType font.
	install -p \
		$(COMPILED_DIR)/unifont_upper-$(VERSION).hex \
		$(TTFSRC)/unifont_upper.hex
	sort -u $(UPPER_COMBINING) > $(TTFSRC)/combining_upper.txt
	cd $(TTFSRC) && \
	   $(MAKE) outline FONTFILE="unifont_upper" COMBINING="combining_upper" \
		FONTNAME="Unifont Upper" PSNAME="UnifontUpper"
	mv $(TTFSRC)/unifont_upper.sfd \
		$(COMPILED_DIR)/unifont_upper-$(VERSION).sfd 
	mv $(TTFSRC)/unifont_upper.ttf \
		$(COMPILED_DIR)/unifont_upper-$(VERSION).ttf 
	gzip $(GZFLAGS) $(COMPILED_DIR)/unifont_upper-$(VERSION).sfd 
	# Second, copy unifont_upper_sample.bdf, to make an SBIT font.
	install -p \
		$(COMPILED_DIR)/unifont_upper_sample-$(VERSION).bdf \
		$(TTFSRC)/unifont_upper_sample.bdf
	cd $(TTFSRC) && \
	   $(MAKE) sbit FONTFILE="unifont_upper_sample" COMBINING="" \
		FONTNAME="Unifont Upper Sample" PSNAME="UnifontUpperSample"
	mv $(TTFSRC)/unifont_upper_sample.ttf \
		$(COMPILED_DIR)/unifont_upper_sample-$(VERSION).ttf 


#
# Copy the newly created files from $(COMPILED_DIR) to the precompiled/
# directory.  This has to be called manually, because the precompiled/
# directory usually remains untouched.
#
precompiled: all
	\rm -rf precompiled
	install -m0755 -d precompiled
	install -m0644 -p $(COMPILED_DIR)/unifont-$(VERSION).hex \
	                  $(COMPILED_DIR)/unifont-$(VERSION).bdf.gz \
	                  $(COMPILED_DIR)/unifont-$(VERSION).pcf.gz \
	                  $(COMPILED_DIR)/unifont_jp-$(VERSION).hex \
	                  $(COMPILED_DIR)/unifont_jp_sample-$(VERSION).hex \
	                  $(COMPILED_DIR)/Unifont-APL8x16-$(VERSION).psf.gz \
	                  $(COMPILED_DIR)/unifont-$(VERSION).ttf \
	                  $(COMPILED_DIR)/unifont_csur-$(VERSION).pcf.gz \
	                  $(COMPILED_DIR)/unifont_csur-$(VERSION).ttf \
	                  $(COMPILED_DIR)/unifont_jp-$(VERSION).bdf.gz \
	                  $(COMPILED_DIR)/unifont_jp-$(VERSION).ttf \
	                  $(COMPILED_DIR)/unifont_sample-$(VERSION).hex \
	                  $(COMPILED_DIR)/unifont_sample-$(VERSION).bdf.gz \
	                  $(COMPILED_DIR)/unifont_sample-$(VERSION).pcf.gz \
	                  $(COMPILED_DIR)/unifont_sample-$(VERSION).ttf \
	                  $(COMPILED_DIR)/coverage.txt \
	                  $(COMPILED_DIR)/unifont-$(VERSION).bmp \
	                  $(COMPILED_DIR)/unifont_plane1-$(VERSION).bmp \
	                  $(COMPILED_DIR)/unifont_upper-$(VERSION).hex \
	                  $(COMPILED_DIR)/unifont_upper-$(VERSION).ttf \
	                  $(COMPILED_DIR)/unifont_upper_sample-$(VERSION).hex \
	                  $(COMPILED_DIR)/unifont_upper_sample-$(VERSION).ttf \
	   precompiled

#
# This is set up for Debian.  Solaris places fonts in
# "/usr/share/fonts/TrueType".  Other unices use other directories.
# The original font format was BDF, but Debian doesn't use that and
# xfs should only need TrueType, so the BDF font isn't installed.
#
# The TrueType font is about 16 Megabytes.  To avoid duplication,
# the fonts are installed as symbolic links back to the original package.
# Alternatively, the fonts can be copied to the destination directory
# with "install -m0644" or moved there (but "mv" is a destructive
# one-time operation).
#
# After installing the new fonts, register them with X Window System using:
#
#      xset fp rehash
#
install:
	if [ x$(CURDIR) = x ] ; \
	then \
	   @echo "Fatal Error: CURDIR not defined -- define in Makefile." ; \
	   exit 1 ; \
	fi
	$(INSTALL) -m0755 -d $(CONSOLEDEST)
	$(INSTALL) -m0755 -d $(PCFDEST)
	$(INSTALL) -m0755 -d $(TTFDEST)
	if [ ! -d $(COMPILED_DIR) ] ; then \
	   $(INSTALL) -m0644 -p $(CURDIR)/precompiled/Unifont-APL8x16-$(VERSION).psf.gz $(CONSOLEDEST)/Unifont-APL8x16.psf.gz ; \
	   $(INSTALL) -m0644 -p $(CURDIR)/precompiled/unifont-$(VERSION).pcf.gz         $(PCFDEST)/unifont.pcf.gz ; \
	   $(INSTALL) -m0644 -p $(CURDIR)/precompiled/unifont_sample-$(VERSION).pcf.gz  $(PCFDEST)/unifont_sample.pcf.gz ; \
	   $(INSTALL) -m0644 -p $(CURDIR)/precompiled/unifont_csur-$(VERSION).pcf.gz    $(PCFDEST)/unifont_csur.pcf.gz ; \
	   $(INSTALL) -m0644 -p $(CURDIR)/precompiled/unifont-$(VERSION).ttf            $(TTFDEST)/unifont.ttf ; \
	   $(INSTALL) -m0644 -p $(CURDIR)/precompiled/unifont_sample-$(VERSION).ttf     $(TTFDEST)/unifont_sample.ttf ; \
	   $(INSTALL) -m0644 -p $(CURDIR)/precompiled/unifont_csur-$(VERSION).ttf       $(TTFDEST)/unifont_csur.ttf ; \
	   $(INSTALL) -m0644 -p $(CURDIR)/precompiled/unifont_upper-$(VERSION).ttf      $(TTFDEST)/unifont_upper.ttf ; \
	else \
	   $(INSTALL) -m0644 -p $(CURDIR)/$(COMPILED_DIR)/Unifont-APL8x16-$(VERSION).psf.gz $(CONSOLEDEST)/Unifont-APL8x16.psf.gz ; \
	   $(INSTALL) -m0644 -p $(CURDIR)/$(COMPILED_DIR)/unifont-$(VERSION).pcf.gz         $(PCFDEST)/unifont.pcf.gz ; \
	   $(INSTALL) -m0644 -p $(CURDIR)/$(COMPILED_DIR)/unifont_sample-$(VERSION).pcf.gz  $(PCFDEST)/unifont_sample.pcf.gz ; \
	   $(INSTALL) -m0644 -p $(CURDIR)/$(COMPILED_DIR)/unifont_csur-$(VERSION).pcf.gz    $(PCFDEST)/unifont_csur.pcf.gz ; \
	   $(INSTALL) -m0644 -p $(CURDIR)/$(COMPILED_DIR)/unifont-$(VERSION).ttf            $(TTFDEST)/unifont.ttf ; \
	   $(INSTALL) -m0644 -p $(CURDIR)/$(COMPILED_DIR)/unifont_sample-$(VERSION).ttf     $(TTFDEST)/unifont_sample.ttf ; \
	   $(INSTALL) -m0644 -p $(CURDIR)/$(COMPILED_DIR)/unifont_csur-$(VERSION).ttf       $(TTFDEST)/unifont_csur.ttf ; \
	   $(INSTALL) -m0644 -p $(CURDIR)/$(COMPILED_DIR)/unifont_upper-$(VERSION).ttf      $(TTFDEST)/unifont_upper.ttf ; \
	fi

clean:
	\rm -rf $(COMPILED_DIR)/png
	\rm -f $(COMPILED_DIR)/unifont-$(VERSION).hex
	\rm -f $(COMPILED_DIR)/unifont-$(VERSION).bdf
	\rm -f $(COMPILED_DIR)/unifont-$(VERSION).bdf.gz
	\rm -f $(COMPILED_DIR)/unifont-$(VERSION).sfd.gz
	\rm -f *~

#
# Note that distclean leaves precompiled/ alone.  This is intentional.
# The .DS files are created under Mac OS X.
#
distclean:
	\rm -rf $(COMPILED_DIR)
	$(MAKE) -C plane00csur distclean
	$(MAKE) -C plane01 distclean
	$(MAKE) -C ttfsrc distclean
	\rm -f *~
	\rm -rf .DS* ._.DS*

.PHONY: all hex bdf pcf coverage pagecount png bigpic csurttf upper ttf \
	precompiled install clean distclean 

