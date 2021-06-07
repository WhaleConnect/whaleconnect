
SHELL = /bin/sh

CC = gcc

CFLAGS   = -g -O2 -Wall

LOCALBINDIR = ../bin

VPATH = . $(LOCALBINDIR)

#
# INSTALL can be defined on the command line to override
# the next definition.  This is only applied to C programs
# to allow INSTALL="install -s" to be specified.  Perl
# programs just use "install", not "$(INSTALL)".
#
INSTALL=install
#
# $(PREFIX) is defined in top-level directory, so don't do a
# 'make install' within this directory -- do it from above.
#
INSTALLDIR = $(PREFIX)/bin

#
# Programs written in C.
#
CPROGS = unibmp2hex unicoverage unidup unibdf2hex unibmpbump \
	 unifont1per unifontpic unigencircles unigenwidth \
	 unihex2bmp unihexgen unipagecount

#
# Programs written in Perl.
#
# The programs johab2ucs2, unifontchojung, and unifontksx are
# only used for generation of Hangul syllables.
#
PPROGS = bdfimplode hex2bdf hex2sfd hexbraille hexdraw hexkinya hexmerge \
	 johab2ucs2 unifontchojung unifontksx unihex2png unihexfill \
	 unihexrotate unipng2hex unifont-viewer

PROGS = $(CPROGS) $(PPROGS)


all: $(CPROGS) bin

# .c:

bin: bin-stamp

bin-stamp: $(CPROGS) $(PPROGS) $(PNGPROGS)
	install -m0755 -d $(LOCALBINDIR)
	$(INSTALL) -m0755 $(CPROGS) $(LOCALBINDIR)
	install    -m0755 $(PPROGS) $(LOCALBINDIR)
	\rm -f $(CPROGS)
	touch $@

install:
	if [ x$(PREFIX) = x ] ; \
	then \
	   @echo "Fatal error: PREFIX directory is not defined." && \
	   @echo "Run 'make install' from top-level directory" && \
	   exit 1 ; \
	fi
	install -m0755 -d $(INSTALLDIR)
	set -e && \
	   cd $(LOCALBINDIR) && \
	   $(INSTALL) -m0755 $(CPROGS) $(INSTALLDIR)
	set -e && \
	   cd ../bin && \
	   install    -m0755 $(PPROGS) $(INSTALLDIR)

#
# The *.exe is for cygwin; it does nothing elsewhere.
# The *.dSYM is for Mac OS X.
#
clean:
	\rm -f $(CPROGS) *.o
	\rm -f *~
	\rm -f *.exe
	\rm -rf *.dSYM

distclean: clean
	\rm -f bin-stamp *~

.PHONY: all bin install clean distclean
