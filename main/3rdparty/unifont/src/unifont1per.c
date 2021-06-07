/*
   unifont1per.c - read a Unifont .hex file from standard input and
                   produce one glyph per ".bmp" bitmap file as output.

                   Each glyph is 16 pixels tall, and can be 8, 16, 24,
                   or 32 pixels wide.  The width of each output graphic
                   file is determined automatically by the width of each
                   Unifont hex representation..

                   Creates files of the form "U+<codepoint>.bmp", 1 per glyph.

   Synopsis: unifont1per < unifont.hex

   Author: Paul Hardy, unifoundry <at> unifoundry.com, December 2016
   
   Copyright (C) 2016, 2017 Paul Hardy

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

   Example:

      mkdir my-bmp
      cd my-bmp
      unifont1per < ../glyphs.hex

*/

/*
   11 May 2019 [Paul Hardy]:
      - Changed sprintf function call to snprintf for writing
	"filename" character string.
      - Defined MAXFILENAME to hold size of "filename" array
	for snprintf function call.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Maximum size of an input line in a Unifont .hex file */
#define MAXSTRING 266

/* Maximum size of a filename of the form "U+%06X.bmp" */
#define MAXFILENAME 20

int
main () {

   int i; /* loop variable */

   /*
      Define bitmap header bytes
   */
   unsigned char header [62] = {
      /*
         Bitmap File Header -- 14 bytes
      */
      'B', 'M',       /* Signature          */
      0x7E, 0, 0, 0,  /* File Size          */
         0, 0, 0, 0,  /* Reserved           */
      0x3E, 0, 0, 0,  /* Pixel Array Offset */

      /*
         Device Independent Bitmap Header -- 40 bytes

         Image Width and Image Height are assigned final values
         based on the dimensions of each glyph.
      */
      0x28,    0,    0,    0,  /* DIB Header Size                 */
      0x10,    0,    0,    0,  /* Image Width = 16 pixels         */
      0xF0, 0xFF, 0xFF, 0xFF,  /* Image Height = -16 pixels       */
      0x01,    0,              /* Planes                          */
      0x01,    0,              /* Bits Per Pixel                  */
         0,    0,    0,    0,  /* Compression                     */
      0x40,    0,    0,    0,  /* Image Size                      */
      0x14, 0x0B,    0,    0,  /* X Pixels Per Meter = 72 dpi     */
      0x14, 0x0B,    0,    0,  /* Y Pixels Per Meter = 72 dpi     */
      0x02,    0,    0,    0,  /* Colors In Color Table           */
         0,    0,    0,    0,  /* Important Colors                */

      /*
         Color Palette -- 8 bytes
      */
      0xFF, 0xFF, 0xFF, 0,  /* White */
         0,    0,    0, 0   /* Black */
   };

   char instring[MAXSTRING]; /* input string                        */
   int  code_point;          /* current Unicode code point          */
   char glyph[MAXSTRING];    /* bitmap string for this glyph        */
   int  glyph_height=16;     /* for now, fixed at 16 pixels high    */
   int  glyph_width;         /* 8, 16, 24, or 32 pixels wide        */
   char filename[MAXFILENAME];/* name of current output file        */
   FILE *outfp;              /* file pointer to current output file */

   int string_index;  /* pointer into hexadecimal glyph string */
   int nextbyte;      /* next set of 8 bits to print out       */

   /* Repeat for each line in the input stream */
   while (fgets (instring, MAXSTRING - 1, stdin) != NULL) {
      /* Read next Unifont ASCII hexadecimal format glyph description */
      sscanf (instring, "%X:%s", &code_point, glyph);
      /* Calculate width of a glyph in pixels; 4 bits per ASCII hex digit */
      glyph_width = strlen (glyph) / (glyph_height / 4);
      snprintf (filename, MAXFILENAME, "U+%06X.bmp", code_point);
      header [18] =  glyph_width;  /* bitmap width */
      header [22] = -glyph_height; /* negative height --> draw top to bottom */
      if ((outfp = fopen (filename, "w")) != NULL) {
         for (i = 0; i < 62; i++) fputc (header[i], outfp);
         /*
            Bitmap, with each row padded with zeroes if necessary
            so each row is four bytes wide.  (Each row must end
            on a four-byte boundary, and four bytes is the maximum
            possible row length for up to 32 pixels in a row.)
         */
         string_index = 0;
         for (i = 0; i < glyph_height; i++) {
            /* Read 2 ASCII hexadecimal digits (1 byte of output pixels) */
            sscanf (&glyph[string_index], "%2X", &nextbyte);
            string_index += 2;
            fputc (nextbyte, outfp);  /* write out the 8 pixels    */
            if (glyph_width <= 8) {   /* pad row with 3 zero bytes */
               fputc (0x00, outfp); fputc (0x00, outfp); fputc (0x00, outfp);
            }
            else { /* get 8 more pixels */
               sscanf (&glyph[string_index], "%2X", &nextbyte);
               string_index += 2;
               fputc (nextbyte, outfp);  /* write out the 8 pixels    */
               if (glyph_width <= 16) {  /* pad row with 2 zero bytes */
                  fputc (0x00, outfp); fputc (0x00, outfp);
               }
               else { /* get 8 more pixels */
                  sscanf (&glyph[string_index], "%2X", &nextbyte);
                  string_index += 2;
                  fputc (nextbyte, outfp);  /* write out the 8 pixels   */
                  if (glyph_width <= 24) {  /* pad row with 1 zero byte */
                     fputc (0x00, outfp);
                  }
                  else { /* get 8 more pixels */
                     sscanf (&glyph[string_index], "%2X", &nextbyte);
                     string_index += 2;
                     fputc (nextbyte, outfp); /* write out the 8 pixels */
                  }  /* glyph is 32 pixels wide */
               }  /* glyph is 24 pixels wide */
            }  /* glyph is 16 pixels wide */
         }  /* glyph is 8 pixels wide */

         fclose (outfp);
      }
   }

   exit (EXIT_SUCCESS);
}
