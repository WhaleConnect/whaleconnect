
SHELL = /bin/sh

BINDIR = ../../bin

FONTFORGE = fontforge

#
# Default values, if not set on the command line when make is invoked:
#
# FONTFILE:  Prefix of the file name for input and output files.
# FONTNAME:  Name of the font inside a TTF file.
# PSNAME:    PostScript name of the font inside a TTF file; can't have spaces.
# COMBINING: Prefix of the file containing a list of combining characters.
#
FONTFILE="unifont"
FONTNAME="Unifont"
PSNAME="Unifont"
COMBINING="combining"

#
# Designate language for TrueType text as American English.
#
#
TTF_LANG = 0x409
#
# The PostScript name of a font can't contain spaces--remove them.
# Could also use bash string replacement if you know you're using bash.
#

COPYRIGHT = "Copyright © 1998-2021 Roman Czyborra, Paul Hardy, \
Qianqian Fang, Andrew Miller, Johnnie Weaver, David Corbett, \
Nils Moskopp, Rebecca Bettencourt, et al."

VERSION = 14.0.01

#
# How to build unifont.ttf from GNU Unifont's unifont.hex
# -------------------------------------------------------
# Written by Luis Alejandro Gonzalez Miranda - http://www.lgm.cl/
#

#
# 2008 - Paul Hardy created this Makefile from Luis' original
# howto-build.sh and other bash scripts.  Those original scripts
# don't appear in this archive, but they can be retrieved from
# http://www.lgm.cl/.
#

# First of all, you need a Perl interpreter and FontForge.
#
# I don't remember all the steps, but I think it was as described by
# this script.

# This division is done only so the Simplify and RemoveOverlap
# operations don't use up too much memory, and because
# a .sfd generated from the whole unifont.hex would be too big to
# process all at once.

all: outline

#
# Commented out this operation on SFD file because not all applications
# correctly interpreted the settings:
#
#	    SetFontNames("UnifontMedium", "GNU", "Unifont", "Medium", $(COPYRIGHT), "$(VERSION)"); \
#
# Convert unifont.hex to unifont.sfd as a single file, then generate
# an outline TrueType font.
#
#
# SetFontNames(fontname[, family[, fullname[, weight[, copyright-notice[, fontversion]]]]])
#
outline: $(FONTFILE).hex $(BINDIR)/hex2sfd
	@echo "Converting font as a single file."
	$(BINDIR)/hex2sfd $(COMBINING).txt < $(FONTFILE).hex > $(FONTFILE).sfd
	$(FONTFORGE) -lang=ff -c \
	   'Open($$1); \
	    SetFontNames("$(PSNAME)Medium", \
		"$(FONTNAME)", "$(FONTNAME)", "Medium", \
		$(COPYRIGHT)); \
	    SetTTFName($(TTF_LANG),  2, "Regular"); \
	    SetTTFName($(TTF_LANG),  5, "Version $(VERSION)"); \
	    SetTTFName($(TTF_LANG), 11, "https://unifoundry.com/unifont/"); \
	    SetTTFName($(TTF_LANG), 13, "Dual license: SIL Open Font License version 1.1, and GNU GPL version 2 or later with the GNU Font Embedding Exception."); \
	    SetTTFName($(TTF_LANG), 14, "http://gnu.org/licenses/gpl.html"); \
	    SelectAll(); \
	    RemoveOverlap(); \
	    Simplify(64,1); \
	    Save($$1);' \
	   $(FONTFILE).sfd
	@echo "Converting .sfd font into .ttf font"
	$(FONTFORGE) -lang=ff -c \
	   'Open($$1); \
	    Generate($$2)' $(FONTFILE).sfd $(FONTFILE).ttf
	\rm -f $(FONTFILE).hex $(COMBINING).txt

#
# This fontforge script reads a BDF font file and generates an SBIT font file.
# Author: written by Qianqian Fang, given to Paul Hardy in 2008.
# The SBIT font is far smaller than the default outline TrueType font
# and takes far less time to build than the outline font.  However, it
# isn't scalable.  An SBIT font could be created and merged with the
# larger TTF font using fontforge, but I (Paul Hardy) haven't noticed
# any degradation in the screen rendering of just the outline TTF font
# that this Makefile produces as its final product.  This is with
# daily use of this Makefile's default TrueType font.
#
# This builds an SBIT font from the unifont_sample BDF font.  The
# BDF font already contains font name, etc., so they don't need to
# be set using SetFontNames; those parameters are left null so the
# existing font's values will be preserved.  However, Fontforge
# does not read the FONT_VERSION property so Paul Hardy added the
# the SetFontNames call.
#
# Commented out because not all applications correctly interpreted
# the settings:
#
#	    SetFontNames("","","","","","$(VERSION)"); \

sbit: $(FONTFILE).bdf
	$(FONTFORGE) -lang=ff -c \
	   'New(); \
	    Import($$1); \
	    SetFontNames("","","","","","$(VERSION)"); \
	    Generate($$2, "ttf"); \
	    Close()' \
	   $(FONTFILE).bdf $(FONTFILE).ttf
	\rm -f $(FONTFILE).bdf

#
# Delete files copied into this directory to build TTF fonts.
#
clean:
	\rm -f *.bdf *.hex *.txt

#
# Delete files created within this directory while building TTF fonts.
#
distclean: clean
	\rm -f *.sfd *.ttf

.PHONY: all outline sbit clean distclean
