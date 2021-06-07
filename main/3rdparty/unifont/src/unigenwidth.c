/*
   unigenwidth - IEEE 1003.1-2008 setup to calculate wchar_t string widths.
                 All glyphs are treated as 16 pixels high, and can be
                 8, 16, 24, or 32 pixels wide (resulting in widths of
                 1, 2, 3, or 4, respectively).

   Author: Paul Hardy, 2013, 2017

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
   20 June 2017 [Paul Hardy]:
      - Now handles glyphs that are 24 or 32 pixels wide.

   8 July 2017 [Paul Hardy]:
      - Modifies sscanf format strings to ignore second field after
        the ":" field separator, newly added to "*combining.txt" files
        and already present in "*.hex" files.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXSTRING	256

/* Definitions for Pikto in Plane 15 */
#define PIKTO_START	0x0F0E70
#define PIKTO_END	0x0F11EF
#define PIKTO_SIZE	(PIKTO_END - PIKTO_START + 1)


int
main (int argc, char **argv)
{

   int i; /* loop variable */

   char teststring[MAXSTRING];
   int  loc;
   char *gstart;

   char glyph_width[0x20000];
   char pikto_width[PIKTO_SIZE];

   FILE *infilefp;

   if (argc != 3) {
      fprintf (stderr, "\n\nUsage: %s <unifont.hex> <combining.txt>\n\n", argv[0]);
      exit (EXIT_FAILURE);
   }

   /*
      Read the collection of hex glyphs.
   */
   if ((infilefp = fopen (argv[1],"r")) == NULL) {
      fprintf (stderr,"ERROR - hex input file %s not found.\n\n", argv[1]);
      exit (EXIT_FAILURE);
   }

   /* Flag glyph as non-existent until found. */
   memset (glyph_width, -1, 0x20000 * sizeof (char));
   memset (pikto_width, -1, (PIKTO_SIZE) * sizeof (char));

   teststring[MAXSTRING-1] = '\0';
   while (fgets (teststring, MAXSTRING-1, infilefp) != NULL) {
      sscanf (teststring, "%X:%*s", &loc);
      if (loc < 0x20000) {
         gstart = strchr (teststring,':') + 1;
         /*
            16 rows per glyph, 2 ASCII hexadecimal digits per byte,
            so divide number of digits by 32 (shift right 5 bits).
         */
         glyph_width[loc] = (strlen (gstart) - 1) >> 5;
      }
      else if ((loc >= PIKTO_START) && (loc <= PIKTO_END)) {
         gstart = strchr (teststring,':') + 1;
         pikto_width[loc - PIKTO_START] = strlen (gstart) <= 34 ? 1 : 2;
      }
   }

   fclose (infilefp);

   /*
      Now read the combining character code points.  These have width of 0.
   */
   if ((infilefp = fopen (argv[2],"r")) == NULL) {
      fprintf (stderr,"ERROR - combining characters file %s not found.\n\n", argv[2]);
      exit (EXIT_FAILURE);
   }

   while (fgets (teststring, MAXSTRING-1, infilefp) != NULL) {
      sscanf (teststring, "%X:%*s", &loc);
      if (loc < 0x20000) glyph_width[loc] = 0;
   }

   fclose (infilefp);

   /*
      Code Points with Unusual Properties (Unicode Standard, Chapter 4).

      As of Unifont 10.0.04, use the widths in the "*-nonprinting.hex"
      files.  If an application is smart enough to know how to handle
      these special cases, it will not render the "nonprinting" glyph
      and will treat the code point as being zero-width.
   */
// glyph_width[0]=0; /* NULL character */
// for (i = 0x0001; i <= 0x001F; i++) glyph_width[i]=-1; /* Control Characters */
// for (i = 0x007F; i <= 0x009F; i++) glyph_width[i]=-1; /* Control Characters */

// glyph_width[0x034F]=0; /* combining grapheme joiner               */
// glyph_width[0x180B]=0; /* Mongolian free variation selector one   */
// glyph_width[0x180C]=0; /* Mongolian free variation selector two   */
// glyph_width[0x180D]=0; /* Mongolian free variation selector three */
// glyph_width[0x180E]=0; /* Mongolian vowel separator               */
// glyph_width[0x200B]=0; /* zero width space                        */
// glyph_width[0x200C]=0; /* zero width non-joiner                   */
// glyph_width[0x200D]=0; /* zero width joiner                       */
// glyph_width[0x200E]=0; /* left-to-right mark                      */
// glyph_width[0x200F]=0; /* right-to-left mark                      */
// glyph_width[0x202A]=0; /* left-to-right embedding                 */
// glyph_width[0x202B]=0; /* right-to-left embedding                 */
// glyph_width[0x202C]=0; /* pop directional formatting              */
// glyph_width[0x202D]=0; /* left-to-right override                  */
// glyph_width[0x202E]=0; /* right-to-left override                  */
// glyph_width[0x2060]=0; /* word joiner                             */
// glyph_width[0x2061]=0; /* function application                    */
// glyph_width[0x2062]=0; /* invisible times                         */
// glyph_width[0x2063]=0; /* invisible separator                     */
// glyph_width[0x2064]=0; /* invisible plus                          */
// glyph_width[0x206A]=0; /* inhibit symmetric swapping              */
// glyph_width[0x206B]=0; /* activate symmetric swapping             */
// glyph_width[0x206C]=0; /* inhibit arabic form shaping             */
// glyph_width[0x206D]=0; /* activate arabic form shaping            */
// glyph_width[0x206E]=0; /* national digit shapes                   */
// glyph_width[0x206F]=0; /* nominal digit shapes                    */

// /* Variation Selector-1 to Variation Selector-16 */
// for (i = 0xFE00; i <= 0xFE0F; i++) glyph_width[i] = 0;

// glyph_width[0xFEFF]=0; /* zero width no-break space         */
// glyph_width[0xFFF9]=0; /* interlinear annotation anchor     */
// glyph_width[0xFFFA]=0; /* interlinear annotation separator  */
// glyph_width[0xFFFB]=0; /* interlinear annotation terminator */
   /*
      Let glyph widths represent 0xFFFC (object replacement character)
      and 0xFFFD (replacement character).
   */

   /*
      Hangul Jamo:

         Leading Consonant (Choseong): leave spacing as is.

         Hangul Choseong Filler (U+115F): set width to 2.

         Hangul Jungseong Filler, Hangul Vowel (Jungseong), and
         Final Consonant (Jongseong): set width to 0, because these
         combine with the leading consonant as one composite syllabic
         glyph.  As of Unicode 5.2, the Hangul Jamo block (U+1100..U+11FF)
         is completely filled.
   */
   // for (i = 0x1160; i <= 0x11FF; i++) glyph_width[i]=0; /* Vowels & Final Consonants */

   /*
      Private Use Area -- the width is undefined, but likely
      to be 2 charcells wide either from a graphic glyph or
      from a four-digit hexadecimal glyph representing the
      code point.  Therefore if any PUA glyph does not have
      a non-zero width yet, assign it a default width of 2.
      The Unicode Standard allows giving PUA characters
      default property values; see for example The Unicode
      Standard Version 5.0, p. 91.  This same default is
      used for higher plane PUA code points below.
   */
   // for (i = 0xE000; i <= 0xF8FF; i++) {
   //    if (glyph_width[i] == 0) glyph_width[i]=2;
   // }

   /*
      <not a character>
   */
   for (i = 0xFDD0; i <= 0xFDEF; i++) glyph_width[i] = -1;
   glyph_width[0xFFFE] = -1; /* Byte Order Mark */
   glyph_width[0xFFFF] = -1; /* Byte Order Mark */

   /* Surrogate Code Points */
   for (i = 0xD800; i <= 0xDFFF; i++) glyph_width[i]=-1;

   /* CJK Code Points */
   for (i = 0x4E00; i <= 0x9FFF; i++) if (glyph_width[i] < 0) glyph_width[i] = 2;
   for (i = 0x3400; i <= 0x4DBF; i++) if (glyph_width[i] < 0) glyph_width[i] = 2;
   for (i = 0xF900; i <= 0xFAFF; i++) if (glyph_width[i] < 0) glyph_width[i] = 2;

   /*
      Now generate the output file.
   */
   printf ("/*\n");
   printf ("   wcwidth and wcswidth functions, as per IEEE 1003.1-2008\n");
   printf ("   System Interfaces, pp. 2241 and 2251.\n\n");
   printf ("   Author: Paul Hardy, 2013\n\n");
   printf ("   Copyright (c) 2013 Paul Hardy\n\n");
   printf ("   LICENSE:\n");
   printf ("\n");
   printf ("      This program is free software: you can redistribute it and/or modify\n");
   printf ("      it under the terms of the GNU General Public License as published by\n");
   printf ("      the Free Software Foundation, either version 2 of the License, or\n");
   printf ("      (at your option) any later version.\n");
   printf ("\n");
   printf ("      This program is distributed in the hope that it will be useful,\n");
   printf ("      but WITHOUT ANY WARRANTY; without even the implied warranty of\n");
   printf ("      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n");
   printf ("      GNU General Public License for more details.\n");
   printf ("\n");
   printf ("      You should have received a copy of the GNU General Public License\n");
   printf ("      along with this program.  If not, see <http://www.gnu.org/licenses/>.\n");
   printf ("*/\n\n");

   printf ("#include <wchar.h>\n\n");
   printf ("/* Definitions for Pikto CSUR Private Use Area glyphs */\n");
   printf ("#define PIKTO_START\t0x%06X\n", PIKTO_START);
   printf ("#define PIKTO_END\t0x%06X\n", PIKTO_END);
   printf ("#define PIKTO_SIZE\t(PIKTO_END - PIKTO_START + 1)\n");
   printf ("\n\n");
   printf ("/* wcwidth -- return charcell positions of one code point */\n");
   printf ("inline int\nwcwidth (wchar_t wc)\n{\n");
   printf ("   return (wcswidth (&wc, 1));\n");
   printf ("}\n");
   printf ("\n\n");
   printf ("int\nwcswidth (const wchar_t *pwcs, size_t n)\n{\n\n");
   printf ("   int i;                    /* loop variable                                      */\n");
   printf ("   unsigned codept;          /* Unicode code point of current character            */\n");
   printf ("   unsigned plane;           /* Unicode plane, 0x00..0x10                          */\n");
   printf ("   unsigned lower17;         /* lower 17 bits of Unicode code point                */\n");
   printf ("   unsigned lower16;         /* lower 16 bits of Unicode code point                */\n");
   printf ("   int lowpt, midpt, highpt; /* for binary searching in plane1zeroes[]             */\n");
   printf ("   int found;                /* for binary searching in plane1zeroes[]             */\n");
   printf ("   int totalwidth;           /* total width of string, in charcells (1 or 2/glyph) */\n");
   printf ("   int illegalchar;          /* Whether or not this code point is illegal          */\n");
   putchar ('\n');

   /*
      Print the glyph_width[] array for glyphs widths in the
      Basic Multilingual Plane (Plane 0).
   */
   printf ("   char glyph_width[0x20000] = {");
   for (i = 0; i < 0x10000; i++) {
      if ((i & 0x1F) == 0)
         printf ("\n      /* U+%04X */ ", i);
      printf ("%d,", glyph_width[i]);
   }
   for (i = 0x10000; i < 0x20000; i++) {
      if ((i & 0x1F) == 0)
         printf ("\n      /* U+%06X */ ", i);
      printf ("%d", glyph_width[i]);
      if (i < 0x1FFFF) putchar (',');
   }
   printf ("\n   };\n\n");

   /*
      Print the pikto_width[] array for Pikto glyph widths.
   */
   printf ("   char pikto_width[PIKTO_SIZE] = {");
   for (i = 0; i < PIKTO_SIZE; i++) {
      if ((i & 0x1F) == 0)
         printf ("\n      /* U+%06X */ ", PIKTO_START + i);
      printf ("%d", pikto_width[i]);
      if ((PIKTO_START + i) < PIKTO_END) putchar (',');
   }
   printf ("\n   };\n\n");

   /*
      Execution part of wcswidth.
   */
   printf ("\n");
   printf ("   illegalchar = totalwidth = 0;\n");
   printf ("   for (i = 0; !illegalchar && i < n; i++) {\n");
   printf ("      codept  = pwcs[i];\n");
   printf ("      plane   = codept >> 16;\n");
   printf ("      lower17 = codept & 0x1FFFF;\n");
   printf ("      lower16 = codept & 0xFFFF;\n");
   printf ("      if (plane < 2) { /* the most common case */\n");
   printf ("         if (glyph_width[lower17] < 0) illegalchar = 1;\n");
   printf ("         else totalwidth += glyph_width[lower17];\n");
   printf ("      }\n");
   printf ("      else { /* a higher plane or beyond Unicode range */\n");
   printf ("         if  ((lower16 == 0xFFFE) || (lower16 == 0xFFFF)) {\n");
   printf ("            illegalchar = 1;\n");
   printf ("         }\n");
   printf ("         else if (plane < 4) {  /* Ideographic Plane */\n");
   printf ("            totalwidth += 2; /* Default ideographic width */\n");
   printf ("         }\n");
   printf ("         else if (plane == 0x0F) {  /* CSUR Private Use Area */\n");
   printf ("            if (lower16 <= 0x0E6F) { /* Kinya */\n");
   printf ("               totalwidth++; /* all Kinya syllables have width 1 */\n");
   printf ("            }\n");
   printf ("            else if (lower16 <= (PIKTO_END & 0xFFFF)) { /* Pikto */\n");
   printf ("               if (pikto_width[lower16 - (PIKTO_START & 0xFFFF)] < 0) illegalchar = 1;\n");
   printf ("               else totalwidth += pikto_width[lower16 - (PIKTO_START & 0xFFFF)];\n");
   printf ("            }\n");
   printf ("         }\n");
   printf ("         else if (plane > 0x10) {\n");
   printf ("            illegalchar = 1;\n");
   printf ("         }\n");
   printf ("         /* Other non-printing in higher planes; return -1 as per IEEE 1003.1-2008. */\n");
   printf ("         else if (/* language tags */\n");
   printf ("                  codept == 0x0E0001 || (codept >= 0x0E0020 && codept <= 0x0E007F) ||\n");
   printf ("                  /* variation selectors, 0x0E0100..0x0E01EF */\n");
   printf ("                  (codept >= 0x0E0100 && codept <= 0x0E01EF)) {\n");
   printf ("            illegalchar = 1;\n");
   printf ("         }\n");
   printf ("         /*\n");
   printf ("            Unicode plane 0x02..0x10 printing character\n");
   printf ("         */\n");
   printf ("         else {\n");
   printf ("            illegalchar = 1; /* code is not in font */\n");
   printf ("         }\n");
   printf ("\n");
   printf ("      }\n");
   printf ("   }\n");
   printf ("   if (illegalchar) totalwidth = -1;\n");
   printf ("\n");
   printf ("   return (totalwidth);\n");
   printf ("\n");
   printf ("}\n");

   exit (EXIT_SUCCESS);
}
