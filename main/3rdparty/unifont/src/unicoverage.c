/*
   unicoverage - Show the coverage of Unicode Basic Multilingual
                 Plane scripts for a GNU Unifont hex glyph file

   Synopsis: unicoverage [-ifont_file.hex] [-ocoverage_file.txt]

   This program requires the file "coverage.dat" to be present
   in the directory from which it is run.

   Author: Paul Hardy, unifoundry <at> unifoundry.com, 6 January 2008
   
   Copyright (C) 2008, 2013 Paul Hardy

   LICENSE:

      This program is free software: you can redistribute it and/or modify
      it under the terms of the GNU General Public License as published by
      the Free Software Foundation, either version 2 of the License, or
      (at your option) any later version.

      This program is distributed in the hope that it will be useful,
      but WITHOUT ANY WARRANTY; without even the implied warranty of
      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
      GNU General Public License for more details.

      You should have received a copy of the GNU General Public License
      along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
   2016 (Paul Hardy): Modified in Unifont 9.0.01 release to remove non-existent
   "-p" option and empty example from help printout.

   2018 (Paul Hardy): Modified to cover entire Unicode range, not just Plane 0.

   11 May 2019: [Paul Hardy] changed strcpy function call to strlcpy
   for better error handling.

   31 May 2019: [Paul Hardy] replaced strlcpy call with strncpy
   for compilation on more systems.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define MAXBUF 256


int
main (int argc, char *argv[])
{

   unsigned i;                /* loop variable                       */
   unsigned slen;             /* string length of coverage file line */
   char inbuf[256];           /* input buffer                        */
   unsigned thischar;         /* the current character               */

   char *infile="", *outfile="";  /* names of input and output files        */
   FILE *infp, *outfp;        /* file pointers of input and output files    */
   FILE *coveragefp;          /* file pointer to coverage.dat file          */
   int cstart, cend;          /* current coverage start and end code points */
   char coverstring[MAXBUF];  /* description of current coverage range      */
   int nglyphs;               /* number of glyphs in this section           */
   int nextrange();           /* to get next range & name of Unicode glyphs */

   if ((coveragefp = fopen ("coverage.dat", "r")) == NULL) {
      fprintf (stderr, "\nError: data file \"coverage.dat\" not found.\n\n");
      exit (0);
   }

   if (argc > 1) {
      for (i = 1; i < argc; i++) {
         if (argv[i][0] == '-') {  /* this is an option argument */
            switch (argv[i][1]) {
               case 'i':  /* name of input file */
                  infile = &argv[i][2];
                  break;
               case 'o':  /* name of output file */
                  outfile = &argv[i][2];
                  break;
               default:   /* if unrecognized option, print list and exit */
                  fprintf (stderr, "\nSyntax:\n\n");
                  fprintf (stderr, "   %s -p<Unicode_Page> ", argv[0]);
                  fprintf (stderr, "-i<Input_File> -o<Output_File> -w\n\n");
                  exit (1);
            }
         }
      }
   }
   /*
      Make sure we can open any I/O files that were specified before
      doing anything else.
   */
   if (strlen (infile) > 0) {
      if ((infp = fopen (infile, "r")) == NULL) {
         fprintf (stderr, "Error: can't open %s for input.\n", infile);
         exit (1);
      }
   }
   else {
      infp = stdin;
   }
   if (strlen (outfile) > 0) {
      if ((outfp = fopen (outfile, "w")) == NULL) {
         fprintf (stderr, "Error: can't open %s for output.\n", outfile);
         exit (1);
      }
   }
   else {
      outfp = stdout;
   }
   slen = nextrange (coveragefp, &cstart, &cend, coverstring);
   nglyphs = 0;
   /*
      Print header row.
   */
   fprintf (outfp, "Covered      Range       Script\n");
   fprintf (outfp, "-------      -----       ------\n\n");
   /*
      Read in the glyphs in the file
   */
   while (slen != 0 && fgets (inbuf, MAXBUF-1, infp) != NULL) {
      sscanf (inbuf, "%x", &thischar);
      while (cend < thischar && slen != 0) {
         /* print old range total */
         fprintf (outfp, " %5.1f%%  U+%04X..U+%04X  %s\n",
                 100.0*nglyphs/(1+cend-cstart), cstart, cend, coverstring);
         /* start new range total */
         slen = nextrange (coveragefp, &cstart, &cend, coverstring);
         /*
            Count Non-characters as existing for totals counts.
            The last two code points of each Unicode plane
            (U+*FFFE and U+*FFFF) are reserved.
         */
         nglyphs = 0;
         if (cstart <= 0xFDD0 && cend >= 0xFDEF && nglyphs == 0)
            nglyphs += 32;
         else if ((cstart & 0xFFFF) <= 0xFFFE &&
                  (cend   & 0xFFFF) >= 0xFFFF && nglyphs == 0)
            nglyphs += 2;
      }
      /*
         If we read in a noncharacter, don't count it -- we already
         counted it once above.
      */
      if ( thischar < 0xFDD0 ||
          (thischar > 0xFDEF && thischar < 0xFFFE) ||
           thischar > 0xFFFF) {
         nglyphs++;
      }
   }
   /* print last range total */
   fprintf (outfp, " %5.1f%%  U+%04X..U+%04X  %s\n",
           100.0*nglyphs/(1+cend-cstart), cstart, cend, coverstring);
   exit (0);
}

/*
   nextrange - get next Unicode range
*/
int
nextrange (FILE *coveragefp,
              int *cstart, int *cend,
              char *coverstring)
{

   int i;
   static char inbuf[MAXBUF];
   int retval;         /* the return value */

   do {
      if (fgets (inbuf, MAXBUF-1, coveragefp) != NULL) {
         retval = strlen (inbuf);
         if ((inbuf[0] >= '0' && inbuf[0] <= '9') ||
             (inbuf[0] >= 'A' && inbuf[0] <= 'F') ||
             (inbuf[0] >= 'a' && inbuf[0] <= 'f')) {
            sscanf (inbuf, "%x-%x", cstart, cend);
            i = 0;
            while (inbuf[i] != ' ') i++;  /* find first blank */
            while (inbuf[i] == ' ') i++;  /* find next non-blank */
            strncpy (coverstring, &inbuf[i], MAXBUF);
         }
         else retval = 0;
      }
      else retval = 0;
   } while (retval == 0 && !feof (coveragefp));

   return (retval);
}
