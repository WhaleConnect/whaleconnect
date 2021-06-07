
SHELL = /bin/sh
GZFLAGS = -f -9 -n

MAN1PAGES = bdfimplode.1 hex2bdf.1 hex2sfd.1 \
	hexbraille.1 hexdraw.1 hexmerge.1 hexkinya.1 johab2ucs2.1 \
	unibdf2hex.1 unibmp2hex.1 unibmpbump.1 unicoverage.1 unidup.1 \
	unifont1per.1 unifontpic.1 unifontchojung.1 unifontksx.1 \
	unigencircles.1 unigenwidth.1 unihex2bmp.1 unihex2png.1 unihexfill.1 \
	unihexgen.1 unihexrotate.1 unipagecount.1 unipng2hex.1 unifont-viewer.1

MAN5PAGES = unifont.5

#
# $(PREFIX) is defined in the top-level Makefile or on the command line
# to override the setting below.
#
PREFIX = /usr
INSTALLDIR = $(PREFIX)/share/man

#
# Set "COMPRESS = 0" to install uncompressed man pages (the default), or
# override on the command line.
#
# Set "COMPRESS = 1" or anything else non-zero to install gzipped man pages.
#
COMPRESS = 1

all:

install:
	install -m0755 -d $(INSTALLDIR)
	install -m0755 -d $(INSTALLDIR)/man1
	install -m0755 -d $(INSTALLDIR)/man5
	for i in $(MAN1PAGES) ; do \
	   if [ $(COMPRESS) = 0 ] ; then \
	      install -m0644 $(MAN1PAGES) $(INSTALLDIR)/man1 ; \
	   else \
	      gzip $(GZFLAGS) < $$i >$$i.gz && \
	      chmod 644 $$i.gz && \
	      mv $$i.gz $(INSTALLDIR)/man1 ; \
	   fi \
	done
	for i in $(MAN5PAGES) ; do \
	   if [ $(COMPRESS) = 0 ] ; then \
	      install -m0644 $(MAN5PAGES) $(INSTALLDIR)/man5 ; \
	   else \
	      gzip $(GZFLAGS) < $$i >$$i.gz && \
	      chmod 644 $$i.gz && \
	      mv $$i.gz $(INSTALLDIR)/man5 ; \
	   fi \
	done

clean:
	\rm -f *~

distclean: clean

.PHONY: all install clean distclean
