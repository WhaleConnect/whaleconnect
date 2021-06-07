
#
# Create the Plane 1 unassigned.hex file, with filler glyphs.
# Create filler glyphs in areas where Unicode scripts are not assigned;
# these are placed in directory "all/" so they are not included with
# the main font build, which reads "plane01/*.hex".
#
all: plane01.hex space.hex plane01-nonprinting.hex unassigned-ranges.txt \
	noscript-ranges.txt
	if [ ! -d all ]; then \
		mkdir all; \
	fi
	set -e && \
	   grep "^[0-9A-Fa-f]" unassigned-ranges.txt | \
	   awk 'NF == 1 {print "../../bin/unihexgen " $$1 " " $$1;} \
		NF == 2 {print "../../bin/unihexgen " $$1 " " $$2;}' | \
	   /bin/sh -s > plane01-unassigned.hex
	set -e && \
	   grep "^[0-9A-Fa-f]" noscript-ranges.txt | \
	   awk 'NF == 1 {print "../../bin/unihexgen " $$1 " " $$1;} \
		NF == 2 {print "../../bin/unihexgen " $$1 " " $$2;}' | \
	   /bin/sh -s > noscript.hex
	sort -u *.hex > all/plane01-all.hex

clean:
	\rm -f noscript.hex plane01-unassigned.hex

distclean: clean
	\rm -rf all

.PHONY: all clean distclean
