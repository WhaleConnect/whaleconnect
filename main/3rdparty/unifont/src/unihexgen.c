/*
   unihexgen.c - generate a series of four- or six-digit hexadecimal
                 numbers within a 16x16 glyph, rendered as white digits
                 on a black background.

                 argv[1] is the starting code point (as a hexadecimal
                 string, with no leading "0x".

                 argv[2] is the ending code point (as a hexadecimal
                 string, with no leading "0x".

                 For example:

                    unihexgen e000 f8ff > pua.hex

                 This generates the Private Use Area glyph file.

   This utility program works in Roman Czyborra's unifont.hex file
   format, the basis of the GNU Unifont package.

   This program is released under the terms of the GNU General Public
   License version 2, or (at your option) a later version.

   Author: Paul Hardy, 2013

   Copyright (C) 2013 Paul Hardy

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

#include <stdio.h>
#include <stdlib.h>


/*
   hexdigit[][] definition: the bitmap pattern for
   each hexadecimal digit.

   Each digit is drawn as a 4 wide by 5 high bitmap,
   so each digit row is one hexadecimal digit, and
   each entry has 5 rows.

   For example, the entry for digit 1 is:

      {0x2,0x6,0x2,0x2,0x7},

   which corresponds graphically to:

      --#-  ==>  0010  ==>  0x2
      -##-  ==>  0110  ==>  0x6
      --#-  ==>  0010  ==>  0x2
      --#-  ==>  0010  ==>  0x2
      -###  ==>  0111  ==>  0x7

   These row values will then be exclusive-ORed with four one bits
   (binary 1111, or 0xF) to form white digits on a black background.


   Functions hexprint4 and hexprint6 share the hexdigit array;
   they print four-digit and six-digit hexadecimal code points
   in a single glyph, respectively.
*/
char hexdigit[16][5] = {
   {0x6,0x9,0x9,0x9,0x6},  /* 0x0 */
   {0x2,0x6,0x2,0x2,0x7},  /* 0x1 */
   {0xF,0x1,0xF,0x8,0xF},  /* 0x2 */
   {0xE,0x1,0x7,0x1,0xE},  /* 0x3 */
   {0x9,0x9,0xF,0x1,0x1},  /* 0x4 */
   {0xF,0x8,0xF,0x1,0xF},  /* 0x5 */
   {0x6,0x8,0xE,0x9,0x6},  /* 0x6 */ // {0x8,0x8,0xF,0x9,0xF} [alternate square form of 6]
   {0xF,0x1,0x2,0x4,0x4},  /* 0x7 */
   {0x6,0x9,0x6,0x9,0x6},  /* 0x8 */
   {0x6,0x9,0x7,0x1,0x6},  /* 0x9 */ // {0xF,0x9,0xF,0x1,0x1} [alternate square form of 9]
   {0xF,0x9,0xF,0x9,0x9},  /* 0xA */
   {0xE,0x9,0xE,0x9,0xE},  /* 0xB */
   {0x7,0x8,0x8,0x8,0x7},  /* 0xC */
   {0xE,0x9,0x9,0x9,0xE},  /* 0xD */
   {0xF,0x8,0xE,0x8,0xF},  /* 0xE */
   {0xF,0x8,0xE,0x8,0x8}   /* 0xF */
};


int
main (int argc, char *argv[])
{

   int startcp, endcp, thiscp;
   void hexprint4(int); /* function to print one 4-digit unifont.hex code point */
   void hexprint6(int); /* function to print one 6-digit unifont.hex code point */

   if (argc != 3) {
      fprintf (stderr,"\n%s - generate unifont.hex code points as\n", argv[0]);
      fprintf (stderr,"four-digit hexadecimal numbers in a 2 by 2 grid,\n");
      fprintf (stderr,"or six-digit hexadecimal numbers in a 3 by 2 grid.\n");
      fprintf (stderr,"Syntax:\n\n");
      fprintf (stderr,"     %s first_code_point last_code_point > glyphs.hex\n\n", argv[0]);
      fprintf (stderr,"Example (to generate glyphs for the Private Use Area):\n\n");
      fprintf (stderr,"     %s e000 f8ff > pua.hex\n\n", argv[0]);
      exit (EXIT_FAILURE);
   }

   sscanf (argv[1], "%x", &startcp);
   sscanf (argv[2], "%x", &endcp);

   startcp &= 0xFFFFFF; /* limit to 6 hex digits */
   endcp   &= 0xFFFFFF; /* limit to 6 hex digits */

   /*
      For each code point in the desired range, generate a glyph.
   */
   for (thiscp = startcp; thiscp <= endcp; thiscp++) {
      if (thiscp <= 0xFFFF) {
         hexprint4 (thiscp); /* print digits 2/line, 2 lines */
      }
      else {
         hexprint6 (thiscp); /* print digits 3/line, 2 lines */
      }
   }
   exit (EXIT_SUCCESS);
}


/*
   Takes a 4-digit Unicode code point as an argument
   and prints a unifont.hex string for it to stdout.
*/
void
hexprint4 (int thiscp)
{

   int grid[16]; /* the glyph grid we'll build */

   int row;      /* row number in current glyph */
   int digitrow; /* row number in current hex digit being rendered */
   int rowbits;  /* 1 & 0 bits to draw current glyph row */

   int d1, d2, d3, d4; /* four hexadecimal digits of each code point */

   d1 = (thiscp >> 12) & 0xF;
   d2 = (thiscp >>  8) & 0xF;
   d3 = (thiscp >>  4) & 0xF;
   d4 = (thiscp      ) & 0xF;

   /* top and bottom rows are white */
   grid[0] = grid[15] = 0x0000;

   /* 14 inner rows are 14-pixel wide black lines, centered */
   for (row = 1; row < 15; row++) grid[row] = 0x7FFE;

   printf ("%04X:", thiscp);

   /*
      Render the first row of 2 hexadecimal digits
   */
   digitrow = 0; /* start at top of first row of digits to render */
   for (row = 2; row < 7; row++) {
      rowbits = (hexdigit[d1][digitrow] << 9) |
                (hexdigit[d2][digitrow] << 3);
      grid[row] ^= rowbits; /* digits appear as white on black background */
      digitrow++;
   }

   /*
      Render the second row of 2 hexadecimal digits
   */
   digitrow = 0; /* start at top of first row of digits to render */
   for (row = 9; row < 14; row++) {
      rowbits = (hexdigit[d3][digitrow] << 9) |
                (hexdigit[d4][digitrow] << 3);
      grid[row] ^= rowbits; /* digits appear as white on black background */
      digitrow++;
   }

   for (row = 0; row < 16; row++) printf ("%04X", grid[row] & 0xFFFF);

   putchar ('\n');

   return;
}


/*
   Takes a 6-digit Unicode code point as an argument
   and prints a unifont.hex string for it to stdout.
*/
void
hexprint6 (int thiscp)
{

   int grid[16]; /* the glyph grid we'll build */

   int row;      /* row number in current glyph */
   int digitrow; /* row number in current hex digit being rendered */
   int rowbits;  /* 1 & 0 bits to draw current glyph row */

   int d1, d2, d3, d4, d5, d6; /* six hexadecimal digits of each code point */

   d1 = (thiscp >> 20) & 0xF;
   d2 = (thiscp >> 16) & 0xF;
   d3 = (thiscp >> 12) & 0xF;
   d4 = (thiscp >>  8) & 0xF;
   d5 = (thiscp >>  4) & 0xF;
   d6 = (thiscp      ) & 0xF;

   /* top and bottom rows are white */
   grid[0] = grid[15] = 0x0000;

   /* 14 inner rows are 16-pixel wide black lines, centered */
   for (row = 1; row < 15; row++) grid[row] = 0xFFFF;


   printf ("%06X:", thiscp);

   /*
      Render the first row of 3 hexadecimal digits
   */
   digitrow = 0; /* start at top of first row of digits to render */
   for (row = 2; row < 7; row++) {
      rowbits = (hexdigit[d1][digitrow] << 11) |
                (hexdigit[d2][digitrow] <<  6) |
                (hexdigit[d3][digitrow] <<  1);
      grid[row] ^= rowbits; /* digits appear as white on black background */
      digitrow++;
   }

   /*
      Render the second row of 3 hexadecimal digits
   */
   digitrow = 0; /* start at top of first row of digits to render */
   for (row = 9; row < 14; row++) {
      rowbits = (hexdigit[d4][digitrow] << 11) |
                (hexdigit[d5][digitrow] <<  6) |
                (hexdigit[d6][digitrow] <<  1);
      grid[row] ^= rowbits; /* digits appear as white on black background */
      digitrow++;
   }

   for (row = 0; row < 16; row++) printf ("%04X", grid[row] & 0xFFFF);

   putchar ('\n');

   return;
}

