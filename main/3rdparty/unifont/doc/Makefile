
SHELL = /bin/sh

MAKEINFO = makeinfo
TEXI2PDF = texi2pdf

OUTPUT_FILES = unifont.info unifont.txt unifont.pdf

.SUFFIXES:
.SUFFIXES: .texi .txt .info .pdf

.texi.info:
	$(MAKEINFO) $< -o $@

.texi.txt:
	$(MAKEINFO) --plaintext $< -o $@

.texi.pdf:
	$(TEXI2PDF) $<

#
# The "all:" target does nothing, so that a higher-level "make" invocation
# does not remake the texinfo files, in case the target system does not have
# makeinfo and texi2pdf installed.
# 
# To rebuild the files, issue the command
#
#      make distclean ; make docprep ; make doc ; make clean
#
all:
#
# Type "make docprep" before typing "make doc".
#
docprep:
	\rm -f $(OUTPUT_FILES)

doc: $(OUTPUT_FILES)

install:

clean:
	\rm -f unifont.aux unifont.cp unifont.fn unifont.ky unifont.log
	\rm -f unifont.pg unifont.toc unifont.tp unifont.vr
	\rm -f *~

#
# Only uncomment the line to remove $(OUTPUT_FILES) if your system has
# texinfo, texi2pdf, TeX, and other necessary software installed to
# rebuild the documentation from scratch.
#
distclean: clean
#	\rm -f $(OUTPUT_FILES)

.PHONY: all install clean distclean
