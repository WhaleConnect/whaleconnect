#
# unifoundry.com utilities for the GNU Unifont
#
# Typing "make && make install" will make and
# install the binary programs and man pages.
# To build only the font from scratch, use
# "cd font ; make"
#
SHELL = /bin/sh
INSTALL = install
GZFLAGS = -f -9 -n

PACKAGE = "unifont"
VERSION = 14.0.01

#
# The settings below will install software, man pages, and documentation
# in /usr/local.  To install in a different location, modify USRDIR to
# your liking.
#
USRDIR = usr
# USRDIR = usr/local
PREFIX = $(DESTDIR)/$(USRDIR)
PKGDEST = $(PREFIX)/share/unifont

VPATH = lib font/plane00 font/ttfsrc

HEXFILES = hangul-syllables.hex plane00-nonprinting.hex pua.hex spaces.hex \
	   plane00-unassigned.hex unifont-base.hex wqy.hex

#
# HEXWIDTH and ZEROWIDTH are for forming wcwidth values.
#
HEXWIDTH = font/plane00/hangul-syllables.hex \
	   font/plane00/spaces.hex \
	   font/plane00/unifont-base.hex \
	   font/plane00/wqy.hex \
	   font/plane00/custom00.hex \
	   font/plane00/plane00-nonprinting.hex \
	   font/plane00csur/plane00csur.hex \
	   font/plane00csur/plane00csur-spaces.hex \
	   font/plane01/plane01.hex \
	   font/plane01/plane01-nonprinting.hex \
	   font/plane0Fcsur/plane0Fcsur.hex

ZEROWIDTH = font/plane00/plane00-combining.txt \
	    font/plane00csur/plane00csur-combining.txt \
	    font/plane01/plane01-combining.txt \
	    font/plane0E/plane0E-combining.txt \
	    font/plane0Fcsur/plane0Fcsur-combining.txt

TEXTFILES = ChangeLog INSTALL NEWS README

#
# Whether to build the font or not (default is not).
# Set to non-null value to build font.
#
BUILDFONT=

#
# Whether to install man pages uncompressed (COMPRESS = 0) or
# compressed (COMPRESS != 0).
#
COMPRESS = 1

all: bindir libdir docdir buildfont
	@echo "Make is done."

#
# Build a distribution tarball.
#
dist: distclean
	(cd .. && tar cf $(PACKAGE)-$(VERSION).tar $(PACKAGE)-$(VERSION) && \
		gzip $(GZFLAGS) $(PACKAGE)-$(VERSION).tar)

bindir:
	set -e && $(MAKE) -C src

#
# Conditionally build the font, depending on the value of BUILDFONT.
# To build the font unconditionally, use the "fontdir" target below.
#
buildfont: bindir
	if [ x$(BUILDFONT) != x ] ; \
        then \
           set -e && $(MAKE) -C font ; \
        fi

#
# Not invoked automatically; the font files are taken from
# font/precompiled by default.
#
fontdir:
	set -e && $(MAKE) -C font

libdir: lib/wchardata.c

docdir:
	set -e && $(MAKE) -C doc

mandir:
	set -e && $(MAKE) -C man

precompiled:
	set -e && $(MAKE) precompiled -C font

#
# Create lib/wchardata.c.  If you want to also build the object file
# wchardata.o, uncomment the last line
#
lib/wchardata.c: $(HEXWIDTH) $(ZEROWIDTH) bindir
	install -m0755 -d lib
	sort $(HEXWIDTH) > unifonttemp.hex
	sort $(ZEROWIDTH) > combiningtemp.txt
	bin/unigenwidth unifonttemp.hex combiningtemp.txt > lib/wchardata.c
	\rm -f unifonttemp.hex combiningtemp.txt
#	(cd lib && $(CC) $(CFLAGS) -c wchardata.c && chmod 644 wchardata.o )

install: bindir libdir docdir
	$(MAKE) -C src install PREFIX=$(PREFIX)
	$(MAKE) -C man install PREFIX=$(PREFIX) COMPRESS=$(COMPRESS)
	$(MAKE) -C font install PREFIX=$(PREFIX) DESTDIR=$(DESTDIR)
	install -m0755 -d $(PKGDEST)
	# install -m0644 -p $(TEXTFILES) doc/unifont.txt doc/unifont.info $(PKGDEST)
	# for i in $(TEXTFILES) unifont.txt unifont.info ; do \
	#    gzip $(GZFLAGS) $(PKGDEST)/$$i ; \
	# done
	install -m0644 -p lib/wchardata.c $(PKGDEST)
	install -m0644 -p font/plane00/plane00-combining.txt $(PKGDEST)
	# If "make" wasn't run before, font/compiled won't exist.
	if [ ! -d font/compiled ] ; then \
	   install -m0644 -p font/precompiled/unifont-$(VERSION).hex   $(PKGDEST)/unifont.hex && \
	   install -m0644 -p font/precompiled/unifont-$(VERSION).bmp $(PKGDEST)/unifont.bmp ; \
	else \
	   install -m0644 -p font/compiled/unifont-$(VERSION).hex   $(PKGDEST)/unifont.hex && \
	   install -m0644 -p font/compiled/unifont-$(VERSION).bmp $(PKGDEST)/unifont.bmp ; \
	fi
	gzip $(GZFLAGS) $(PKGDEST)/unifont.bmp

clean:
	$(MAKE) -C src  clean
	$(MAKE) -C doc  clean
	$(MAKE) -C man  clean
	$(MAKE) -C font clean
	\rm -rf *~

#
# The .DS files are created under Mac OS X
#
distclean:
	$(MAKE) -C src  distclean
	$(MAKE) -C doc  distclean
	$(MAKE) -C man  distclean
	$(MAKE) -C font distclean
	\rm -rf bin lib
	\rm -f unifonttemp.hex
	\rm -rf *~
	\rm -rf .DS* ._.DS*

.PHONY: all dist bindir buildfont fontdir libdir docdir mandir precompiled install clean distclean
