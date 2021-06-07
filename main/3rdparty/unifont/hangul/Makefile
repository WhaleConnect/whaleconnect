
SHELL = /bin/sh

BINDIR = ../bin
GZFLAGS = -f -9 -n

#
VERSION = 13.0.06

COPYRIGHT = "Copyright (C) 2013 Paul Hardy.  \
Licensed under the GNU General Public License; either version 2, or \
(at your option) a later version, with the GNU Font Embedding Exception."


all: hangul-syllables

hangul-syllables: hangul-hex hangul-bmp ksx chojung

# Convert the hanbase-??.bmp files into a Johab .hex file,
# then convert that into a BDF font file.

hangul-hex:
	echo "0000:0000000000000000000000000000000000000000000000000000000000000000" >hangul-base.hex
	$(BINDIR)/unibmp2hex -w < hanbase-00.bmp >>hangul-base.hex
	$(BINDIR)/unibmp2hex -w < hanbase-01.bmp >>hangul-base.hex
	$(BINDIR)/unibmp2hex -w < hanbase-02.bmp >>hangul-base.hex
	$(BINDIR)/hex2bdf \
	   -f "Unifont Johab" -c $(COPYRIGHT) -v "$(VERSION)" \
	   < hangul-base.hex | \
	   sed -e "s/[Ii][Ss][Oo]10646/Johab/g" >hangul-base.bdf
	bdftopcf < hangul-base.bdf > hangul-base.pcf
	gzip $(GZFLAGS) hangul-base.pcf
	$(BINDIR)/johab2ucs2 < hangul-base.bdf > hangul-syllables.hex

# Convert the .hex file containing all Unicode Hangul Syllables
# into bitmap images for checking visually.

hangul-bmp: hangul-syllables.hex
	if [ ! -d syllables ]; then \
		mkdir syllables; \
	fi
	$(BINDIR)/unihex2bmp -pAC < hangul-syllables.hex > syllables/hangul-AC.bmp
	$(BINDIR)/unihex2bmp -pAD < hangul-syllables.hex > syllables/hangul-AD.bmp
	$(BINDIR)/unihex2bmp -pAE < hangul-syllables.hex > syllables/hangul-AE.bmp
	$(BINDIR)/unihex2bmp -pAF < hangul-syllables.hex > syllables/hangul-AF.bmp
	$(BINDIR)/unihex2bmp -pB0 < hangul-syllables.hex > syllables/hangul-B0.bmp
	$(BINDIR)/unihex2bmp -pB1 < hangul-syllables.hex > syllables/hangul-B1.bmp
	$(BINDIR)/unihex2bmp -pB2 < hangul-syllables.hex > syllables/hangul-B2.bmp
	$(BINDIR)/unihex2bmp -pB3 < hangul-syllables.hex > syllables/hangul-B3.bmp
	$(BINDIR)/unihex2bmp -pB4 < hangul-syllables.hex > syllables/hangul-B4.bmp
	$(BINDIR)/unihex2bmp -pB5 < hangul-syllables.hex > syllables/hangul-B5.bmp
	$(BINDIR)/unihex2bmp -pB6 < hangul-syllables.hex > syllables/hangul-B6.bmp
	$(BINDIR)/unihex2bmp -pB7 < hangul-syllables.hex > syllables/hangul-B7.bmp
	$(BINDIR)/unihex2bmp -pB8 < hangul-syllables.hex > syllables/hangul-B8.bmp
	$(BINDIR)/unihex2bmp -pB9 < hangul-syllables.hex > syllables/hangul-B9.bmp
	$(BINDIR)/unihex2bmp -pBA < hangul-syllables.hex > syllables/hangul-BA.bmp
	$(BINDIR)/unihex2bmp -pBB < hangul-syllables.hex > syllables/hangul-BB.bmp
	$(BINDIR)/unihex2bmp -pBC < hangul-syllables.hex > syllables/hangul-BC.bmp
	$(BINDIR)/unihex2bmp -pBD < hangul-syllables.hex > syllables/hangul-BD.bmp
	$(BINDIR)/unihex2bmp -pBE < hangul-syllables.hex > syllables/hangul-BE.bmp
	$(BINDIR)/unihex2bmp -pBF < hangul-syllables.hex > syllables/hangul-BF.bmp
	$(BINDIR)/unihex2bmp -pC0 < hangul-syllables.hex > syllables/hangul-C0.bmp
	$(BINDIR)/unihex2bmp -pC1 < hangul-syllables.hex > syllables/hangul-C1.bmp
	$(BINDIR)/unihex2bmp -pC2 < hangul-syllables.hex > syllables/hangul-C2.bmp
	$(BINDIR)/unihex2bmp -pC3 < hangul-syllables.hex > syllables/hangul-C3.bmp
	$(BINDIR)/unihex2bmp -pC4 < hangul-syllables.hex > syllables/hangul-C4.bmp
	$(BINDIR)/unihex2bmp -pC5 < hangul-syllables.hex > syllables/hangul-C5.bmp
	$(BINDIR)/unihex2bmp -pC6 < hangul-syllables.hex > syllables/hangul-C6.bmp
	$(BINDIR)/unihex2bmp -pC7 < hangul-syllables.hex > syllables/hangul-C7.bmp
	$(BINDIR)/unihex2bmp -pC8 < hangul-syllables.hex > syllables/hangul-C8.bmp
	$(BINDIR)/unihex2bmp -pC9 < hangul-syllables.hex > syllables/hangul-C9.bmp
	$(BINDIR)/unihex2bmp -pCA < hangul-syllables.hex > syllables/hangul-CA.bmp
	$(BINDIR)/unihex2bmp -pCB < hangul-syllables.hex > syllables/hangul-CB.bmp
	$(BINDIR)/unihex2bmp -pCC < hangul-syllables.hex > syllables/hangul-CC.bmp
	$(BINDIR)/unihex2bmp -pCD < hangul-syllables.hex > syllables/hangul-CD.bmp
	$(BINDIR)/unihex2bmp -pCE < hangul-syllables.hex > syllables/hangul-CE.bmp
	$(BINDIR)/unihex2bmp -pCF < hangul-syllables.hex > syllables/hangul-CF.bmp
	$(BINDIR)/unihex2bmp -pD0 < hangul-syllables.hex > syllables/hangul-D0.bmp
	$(BINDIR)/unihex2bmp -pD1 < hangul-syllables.hex > syllables/hangul-D1.bmp
	$(BINDIR)/unihex2bmp -pD2 < hangul-syllables.hex > syllables/hangul-D2.bmp
	$(BINDIR)/unihex2bmp -pD3 < hangul-syllables.hex > syllables/hangul-D3.bmp
	$(BINDIR)/unihex2bmp -pD4 < hangul-syllables.hex > syllables/hangul-D4.bmp
	$(BINDIR)/unihex2bmp -pD5 < hangul-syllables.hex > syllables/hangul-D5.bmp
	$(BINDIR)/unihex2bmp -pD6 < hangul-syllables.hex > syllables/hangul-D6.bmp
	$(BINDIR)/unihex2bmp -pD7 < hangul-syllables.hex > syllables/hangul-D7.bmp

#
# Extract those syllables that appear in KS X 1001:1992.
# These are the 2,350 syllables in modern usage out of
# a total possible 11,172 syllables.
#
ksx: hangul-syllables.hex
	if [ ! -d ksx-syllables ]; then \
		mkdir ksx-syllables; \
	fi
	$(BINDIR)/unifontksx < hangul-syllables.hex > ksx-syllables.hex
	$(BINDIR)/unihex2bmp -pAC < ksx-syllables.hex > ksx-syllables/han-AC.bmp
	$(BINDIR)/unihex2bmp -pAD < ksx-syllables.hex > ksx-syllables/han-AD.bmp
	$(BINDIR)/unihex2bmp -pAE < ksx-syllables.hex > ksx-syllables/han-AE.bmp
	$(BINDIR)/unihex2bmp -pAF < ksx-syllables.hex > ksx-syllables/han-AF.bmp
	$(BINDIR)/unihex2bmp -pB0 < ksx-syllables.hex > ksx-syllables/han-B0.bmp
	$(BINDIR)/unihex2bmp -pB1 < ksx-syllables.hex > ksx-syllables/han-B1.bmp
	$(BINDIR)/unihex2bmp -pB2 < ksx-syllables.hex > ksx-syllables/han-B2.bmp
	$(BINDIR)/unihex2bmp -pB3 < ksx-syllables.hex > ksx-syllables/han-B3.bmp
	$(BINDIR)/unihex2bmp -pB4 < ksx-syllables.hex > ksx-syllables/han-B4.bmp
	$(BINDIR)/unihex2bmp -pB5 < ksx-syllables.hex > ksx-syllables/han-B5.bmp
	$(BINDIR)/unihex2bmp -pB6 < ksx-syllables.hex > ksx-syllables/han-B6.bmp
	$(BINDIR)/unihex2bmp -pB7 < ksx-syllables.hex > ksx-syllables/han-B7.bmp
	$(BINDIR)/unihex2bmp -pB8 < ksx-syllables.hex > ksx-syllables/han-B8.bmp
	$(BINDIR)/unihex2bmp -pB9 < ksx-syllables.hex > ksx-syllables/han-B9.bmp
	$(BINDIR)/unihex2bmp -pBA < ksx-syllables.hex > ksx-syllables/han-BA.bmp
	$(BINDIR)/unihex2bmp -pBB < ksx-syllables.hex > ksx-syllables/han-BB.bmp
	$(BINDIR)/unihex2bmp -pBC < ksx-syllables.hex > ksx-syllables/han-BC.bmp
	$(BINDIR)/unihex2bmp -pBD < ksx-syllables.hex > ksx-syllables/han-BD.bmp
	$(BINDIR)/unihex2bmp -pBE < ksx-syllables.hex > ksx-syllables/han-BE.bmp
	$(BINDIR)/unihex2bmp -pBF < ksx-syllables.hex > ksx-syllables/han-BF.bmp
	$(BINDIR)/unihex2bmp -pC0 < ksx-syllables.hex > ksx-syllables/han-C0.bmp
	$(BINDIR)/unihex2bmp -pC1 < ksx-syllables.hex > ksx-syllables/han-C1.bmp
	$(BINDIR)/unihex2bmp -pC2 < ksx-syllables.hex > ksx-syllables/han-C2.bmp
	$(BINDIR)/unihex2bmp -pC3 < ksx-syllables.hex > ksx-syllables/han-C3.bmp
	$(BINDIR)/unihex2bmp -pC4 < ksx-syllables.hex > ksx-syllables/han-C4.bmp
	$(BINDIR)/unihex2bmp -pC5 < ksx-syllables.hex > ksx-syllables/han-C5.bmp
	$(BINDIR)/unihex2bmp -pC6 < ksx-syllables.hex > ksx-syllables/han-C6.bmp
	$(BINDIR)/unihex2bmp -pC7 < ksx-syllables.hex > ksx-syllables/han-C7.bmp
	$(BINDIR)/unihex2bmp -pC8 < ksx-syllables.hex > ksx-syllables/han-C8.bmp
	$(BINDIR)/unihex2bmp -pC9 < ksx-syllables.hex > ksx-syllables/han-C9.bmp
	$(BINDIR)/unihex2bmp -pCA < ksx-syllables.hex > ksx-syllables/han-CA.bmp
	$(BINDIR)/unihex2bmp -pCB < ksx-syllables.hex > ksx-syllables/han-CB.bmp
	$(BINDIR)/unihex2bmp -pCC < ksx-syllables.hex > ksx-syllables/han-CC.bmp
	$(BINDIR)/unihex2bmp -pCD < ksx-syllables.hex > ksx-syllables/han-CD.bmp
	$(BINDIR)/unihex2bmp -pCE < ksx-syllables.hex > ksx-syllables/han-CE.bmp
	$(BINDIR)/unihex2bmp -pCF < ksx-syllables.hex > ksx-syllables/han-CF.bmp
	$(BINDIR)/unihex2bmp -pD0 < ksx-syllables.hex > ksx-syllables/han-D0.bmp
	$(BINDIR)/unihex2bmp -pD1 < ksx-syllables.hex > ksx-syllables/han-D1.bmp
	$(BINDIR)/unihex2bmp -pD2 < ksx-syllables.hex > ksx-syllables/han-D2.bmp
	$(BINDIR)/unihex2bmp -pD3 < ksx-syllables.hex > ksx-syllables/han-D3.bmp
	$(BINDIR)/unihex2bmp -pD4 < ksx-syllables.hex > ksx-syllables/han-D4.bmp
	$(BINDIR)/unihex2bmp -pD5 < ksx-syllables.hex > ksx-syllables/han-D5.bmp
	$(BINDIR)/unihex2bmp -pD6 < ksx-syllables.hex > ksx-syllables/han-D6.bmp
	$(BINDIR)/unihex2bmp -pD7 < ksx-syllables.hex > ksx-syllables/han-D7.bmp

#
# Extract those syllables that only contain choseong and
# jungseong (initial consonant plus vowel).
#
chojung: hangul-syllables.hex
	if [ ! -d chojung-syllables ]; then \
		mkdir chojung-syllables; \
	fi
	$(BINDIR)/unifontchojung < hangul-syllables.hex > chojung-syllables.hex
	$(BINDIR)/unihex2bmp -pAC < chojung-syllables.hex > chojung-syllables/han-AC.bmp
	$(BINDIR)/unihex2bmp -pAD < chojung-syllables.hex > chojung-syllables/han-AD.bmp
	$(BINDIR)/unihex2bmp -pAE < chojung-syllables.hex > chojung-syllables/han-AE.bmp
	$(BINDIR)/unihex2bmp -pAF < chojung-syllables.hex > chojung-syllables/han-AF.bmp
	$(BINDIR)/unihex2bmp -pB0 < chojung-syllables.hex > chojung-syllables/han-B0.bmp
	$(BINDIR)/unihex2bmp -pB1 < chojung-syllables.hex > chojung-syllables/han-B1.bmp
	$(BINDIR)/unihex2bmp -pB2 < chojung-syllables.hex > chojung-syllables/han-B2.bmp
	$(BINDIR)/unihex2bmp -pB3 < chojung-syllables.hex > chojung-syllables/han-B3.bmp
	$(BINDIR)/unihex2bmp -pB4 < chojung-syllables.hex > chojung-syllables/han-B4.bmp
	$(BINDIR)/unihex2bmp -pB5 < chojung-syllables.hex > chojung-syllables/han-B5.bmp
	$(BINDIR)/unihex2bmp -pB6 < chojung-syllables.hex > chojung-syllables/han-B6.bmp
	$(BINDIR)/unihex2bmp -pB7 < chojung-syllables.hex > chojung-syllables/han-B7.bmp
	$(BINDIR)/unihex2bmp -pB8 < chojung-syllables.hex > chojung-syllables/han-B8.bmp
	$(BINDIR)/unihex2bmp -pB9 < chojung-syllables.hex > chojung-syllables/han-B9.bmp
	$(BINDIR)/unihex2bmp -pBA < chojung-syllables.hex > chojung-syllables/han-BA.bmp
	$(BINDIR)/unihex2bmp -pBB < chojung-syllables.hex > chojung-syllables/han-BB.bmp
	$(BINDIR)/unihex2bmp -pBC < chojung-syllables.hex > chojung-syllables/han-BC.bmp
	$(BINDIR)/unihex2bmp -pBD < chojung-syllables.hex > chojung-syllables/han-BD.bmp
	$(BINDIR)/unihex2bmp -pBE < chojung-syllables.hex > chojung-syllables/han-BE.bmp
	$(BINDIR)/unihex2bmp -pBF < chojung-syllables.hex > chojung-syllables/han-BF.bmp
	$(BINDIR)/unihex2bmp -pC0 < chojung-syllables.hex > chojung-syllables/han-C0.bmp
	$(BINDIR)/unihex2bmp -pC1 < chojung-syllables.hex > chojung-syllables/han-C1.bmp
	$(BINDIR)/unihex2bmp -pC2 < chojung-syllables.hex > chojung-syllables/han-C2.bmp
	$(BINDIR)/unihex2bmp -pC3 < chojung-syllables.hex > chojung-syllables/han-C3.bmp
	$(BINDIR)/unihex2bmp -pC4 < chojung-syllables.hex > chojung-syllables/han-C4.bmp
	$(BINDIR)/unihex2bmp -pC5 < chojung-syllables.hex > chojung-syllables/han-C5.bmp
	$(BINDIR)/unihex2bmp -pC6 < chojung-syllables.hex > chojung-syllables/han-C6.bmp
	$(BINDIR)/unihex2bmp -pC7 < chojung-syllables.hex > chojung-syllables/han-C7.bmp
	$(BINDIR)/unihex2bmp -pC8 < chojung-syllables.hex > chojung-syllables/han-C8.bmp
	$(BINDIR)/unihex2bmp -pC9 < chojung-syllables.hex > chojung-syllables/han-C9.bmp
	$(BINDIR)/unihex2bmp -pCA < chojung-syllables.hex > chojung-syllables/han-CA.bmp
	$(BINDIR)/unihex2bmp -pCB < chojung-syllables.hex > chojung-syllables/han-CB.bmp
	$(BINDIR)/unihex2bmp -pCC < chojung-syllables.hex > chojung-syllables/han-CC.bmp
	$(BINDIR)/unihex2bmp -pCD < chojung-syllables.hex > chojung-syllables/han-CD.bmp
	$(BINDIR)/unihex2bmp -pCE < chojung-syllables.hex > chojung-syllables/han-CE.bmp
	$(BINDIR)/unihex2bmp -pCF < chojung-syllables.hex > chojung-syllables/han-CF.bmp
	$(BINDIR)/unihex2bmp -pD0 < chojung-syllables.hex > chojung-syllables/han-D0.bmp
	$(BINDIR)/unihex2bmp -pD1 < chojung-syllables.hex > chojung-syllables/han-D1.bmp
	$(BINDIR)/unihex2bmp -pD2 < chojung-syllables.hex > chojung-syllables/han-D2.bmp
	$(BINDIR)/unihex2bmp -pD3 < chojung-syllables.hex > chojung-syllables/han-D3.bmp
	$(BINDIR)/unihex2bmp -pD4 < chojung-syllables.hex > chojung-syllables/han-D4.bmp
	$(BINDIR)/unihex2bmp -pD5 < chojung-syllables.hex > chojung-syllables/han-D5.bmp
	$(BINDIR)/unihex2bmp -pD6 < chojung-syllables.hex > chojung-syllables/han-D6.bmp
	$(BINDIR)/unihex2bmp -pD7 < chojung-syllables.hex > chojung-syllables/han-D7.bmp

precompiled: hangul-hex
	test -d precompiled || mkdir precompiled
	mv hangul-base.bdf hangul-base.pcf.gz hangul-base.hex hangul-syllables.hex \
	   precompiled

# Remove the files that this Makefile creates.

clean:
	\rm -f hangul-base.bdf hangul-syllables.hex
	\rm -f ksx-syllables.hex chojung-syllables.hex
	\rm -f syllables/* ksx-syllables/* chojung-syllables/*
	\rm -f *~

distclean: clean
	\rm -rf syllables ksx-syllables chojung-syllables *~

.PHONY: all hangul-syllables hangul-hex hangul-bmp ksx chojung clean distclean
