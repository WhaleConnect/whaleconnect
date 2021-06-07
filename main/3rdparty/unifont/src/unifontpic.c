/*
   unifontpic.c - see the "Big Picture": the entire Unifont in one BMP bitmap.

   Author: Paul Hardy, 2013

   Copyright (C) 2013, 2017 Paul Hardy

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
     11 June 2017 [Paul Hardy]:
        -  Modified to take glyphs that are 24 or 32 pixels wide and
           compress them horizontally by 50%.

     8 July 2017 [Paul Hardy]:
        - Modified to print Unifont charts above Unicode Plane 0.
        - Adds "-P" option to specify Unicode plane in decimal,
          as "-P0" through "-P17".  Omitting this argument uses
          plane 0 as the default.
        - Appends Unicode plane number to chart title.
        - Reads in "unifontpic.h", which was added mainly to
          store ASCII chart title glyphs in an embedded array
          rather than requiring these ASCII glyphs to be in
          the ".hex" file that is read in for the chart body
          (which was the case previously, when all that was
          able to print was Unicode place 0).
        - Fixes truncated header in long bitmap format, making
          the long chart title glyphs single-spaced.  This leaves
          room for the Unicode plane to appear even in the narrow
          chart title of the "long" format chart.  The wide chart
          title still has double-spaced ASCII glyphs.
        - Adjusts centering of title on long and wide charts.

     11 May 2019 [Paul Hardy]:
        - Changed strncpy calls to memcpy.
        - Added "HDR_LEN" to define length of header string
	  for use in snprintf function call.
	- Changed sprintf function calls to snprintf function
	  calls for writing chart header string.
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "unifontpic.h"

/* Define length of header string for top of chart. */
#define HDR_LEN 33


/*
   Stylistic Note:

   Many variables in this program use multiple words scrunched
   together, with each word starting with an upper-case letter.
   This is only done to match the canonical field names in the
   Windows Bitmap Graphics spec.
*/

int
main (int argc, char **argv)
{
   /* Input line buffer */
   char instring[MAXSTRING];

   /* long and dpi are set from command-line options */
   int wide=1; /* =1 for a 256x256 grid, =0 for a 16x4096 grid */
   int dpi=96; /* change for 256x256 grid to fit paper if desired */
   int tinynum=0; /* whether to use tiny labels for 256x256 grid */

   int i, j; /* loop variables */

   int plane=0;      /* Unicode plane, 0..17; Plane 0 is default */
   /* 16 pixel rows for each of 65,536 glyphs in a Unicode plane */
   int plane_array[0x10000][16];

   void gethex();
   void genlongbmp();
   void genwidebmp();

   if (argc > 1) {
      for (i = 1; i < argc; i++) {
         if (strncmp (argv[i],"-l",2) == 0) { /* long display */
           wide = 0;
         }
         else if (strncmp (argv[i],"-d",2) == 0) {
            dpi = atoi (&argv[i][2]); /* dots/inch specified on command line */
         }
         else if (strncmp (argv[i],"-t",2) == 0) {
            tinynum = 1;
         }
         else if (strncmp (argv[i],"-P",2) == 0) {
            /* Get Unicode plane */
            for (j = 2; argv[i][j] != '\0'; j++) {
               if (argv[i][j] < '0' || argv[i][j] > '9') {
                  fprintf (stderr,
                           "ERROR: Specify Unicode plane as decimal number.\n\n");
                  exit (EXIT_FAILURE);
               }
            }
            plane = atoi (&argv[i][2]); /* Unicode plane, 0..17 */
            if (plane < 0 || plane > 17) {
               fprintf (stderr,
                        "ERROR: Plane out of Unicode range [0,17].\n\n");
               exit (EXIT_FAILURE);
            }
         }
      }
   }


   /*
      Initialize the ASCII bitmap array for chart titles
   */
   for (i = 0; i < 128; i++) {
      gethex (ascii_hex[i], plane_array, 0); /* convert Unifont hexadecimal string to bitmap */
      for (j = 0; j < 16; j++) ascii_bits[i][j] = plane_array[i][j];
   }


   /*
      Read in the Unifont hex file to render from standard input
   */
   memset ((void *)plane_array, 0, 0x10000 * 16 * sizeof (int));
   while (fgets (instring, MAXSTRING, stdin) != NULL) {
      gethex (instring, plane_array, plane); /* read .hex input file and fill plane_array with glyph data */
   }  /* while not EOF */


   /*
      Write plane_array glyph data to BMP file as wide or long bitmap.
   */
   if (wide) {
      genwidebmp (plane_array, dpi, tinynum, plane);
   }
   else {
      genlongbmp (plane_array, dpi, tinynum, plane);
   }

   exit (EXIT_SUCCESS);
}


void
output4 (int thisword)
{

   putchar ( thisword        & 0xFF);
   putchar ((thisword >>  8) & 0xFF);
   putchar ((thisword >> 16) & 0xFF);
   putchar ((thisword >> 24) & 0xFF);

   return;
}


void
output2 (int thisword)
{

   putchar ( thisword       & 0xFF);
   putchar ((thisword >> 8) & 0xFF);

   return;
}


/*
   gethex reads a Unifont .hex-format input file from stdin.

   Each glyph can be 2, 4, 6, or 8 ASCII hexadecimal digits wide.
   Glyph height is fixed at 16 pixels.

   plane is the Unicode plane, 0..17.
*/
void
gethex (char *instring, int plane_array[0x10000][16], int plane)
{
   char *bitstring;  /* pointer into instring for glyph bitmap */
   int i;       /* loop variable                               */
   int codept;  /* the Unicode code point of the current glyph */
   int glyph_plane; /* Unicode plane of current glyph          */
   int ndigits; /* number of ASCII hexadecimal digits in glyph */
   int bytespl; /* bytes per line of pixels in a glyph         */
   int temprow; /* 1 row of a quadruple-width glyph            */
   int newrow;  /* 1 row of double-width output pixels         */
   unsigned bitmask; /* to mask off 2 bits of long width glyph */

   /*
      Read each input line and place its glyph into the bit array.
   */
   sscanf (instring, "%X", &codept);
   glyph_plane = codept >> 16;
   if (glyph_plane == plane) {
      codept &= 0xFFFF;  /* array index will only have 16 bit address */
      /* find the colon separator */
      for (i = 0; (i < 9) && (instring[i] != ':'); i++);
      i++; /* position past it */
      bitstring = &instring[i];
      ndigits = strlen (bitstring);
      /* don't count '\n' at end of line if present */
      if (bitstring[ndigits - 1] == '\n') ndigits--;
      bytespl = ndigits >> 5;  /* 16 rows per line, 2 digits per byte */

      if (bytespl >= 1 && bytespl <= 4) {
         for (i = 0; i < 16; i++) { /* 16 rows per glyph */
            /* Read correct number of hexadecimal digits given glyph width */
            switch (bytespl) {
               case 1: sscanf (bitstring, "%2X", &temprow);
                       bitstring += 2;
                       temprow <<= 8; /* left-justify single-width glyph */
                       break;
               case 2: sscanf (bitstring, "%4X", &temprow);
                       bitstring += 4;
                       break;
               /* cases 3 and 4 widths will be compressed by 50% (see below) */
               case 3: sscanf (bitstring, "%6X", &temprow);
                       bitstring += 6;
                       temprow <<= 8; /* left-justify */
                       break;
               case 4: sscanf (bitstring, "%8X", &temprow);
                       bitstring += 8;
                       break;
            }  /* switch on number of bytes per row */
            /* compress glyph width by 50% if greater than double-width */
            if (bytespl > 2) {
               newrow = 0x0000;
               /* mask off 2 bits at a time to convert each pair to 1 bit out */
               for (bitmask = 0xC0000000; bitmask != 0; bitmask >>= 2) {
                  newrow <<= 1;
                  if ((temprow & bitmask) != 0) newrow |= 1;
               }
               temprow = newrow;
            }  /* done conditioning glyphs beyond double-width */
            plane_array[codept][i] = temprow;  /* store glyph bitmap for output */
         }  /* for each row */
      }  /* if 1 to 4 bytes per row/line */
   }  /* if this is the plane we are seeking */

   return;
}


/*
   genlongbmp generates the BMP output file from a bitmap parameter.
   This is a long bitmap, 16 glyphs wide by 4,096 glyphs tall.
*/
void
genlongbmp (int plane_array[0x10000][16], int dpi, int tinynum, int plane)
{

   char header_string[HDR_LEN]; /* centered header             */
   char raw_header[HDR_LEN];    /* left-aligned header         */
   int header[16][16];     /* header row, for chart title */
   int hdrlen;             /* length of HEADER_STRING     */
   int startcol;           /* column to start printing header, for centering */

   unsigned leftcol[0x1000][16]; /* code point legend on left side of chart */
   int d1, d2, d3, d4;           /* digits for filling leftcol[][] legend   */
   int codept;                   /* current starting code point for legend  */
   int thisrow;                  /* glyph row currently being rendered      */
   unsigned toprow[16][16];      /* code point legend on top of chart       */
   int digitrow;       /* row we're in (0..4) for the above hexdigit digits */

   /*
      DataOffset = BMP Header bytes + InfoHeader bytes + ColorTable bytes.
   */
   int DataOffset = 14 + 40 + 8; /* fixed size for monochrome BMP */
   int ImageSize;
   int FileSize;
   int Width, Height; /* bitmap image width and height in pixels */
   int ppm;     /* integer pixels per meter */

   int i, j, k;

   unsigned bytesout;

   void output4(int), output2(int);

   /*
      Image width and height, in pixels.

         N.B.: Width must be an even multiple of 32 pixels, or 4 bytes.
   */
   Width  =   18 * 16;  /* (2 legend +   16 glyphs) * 16 pixels/glyph */
   Height = 4099 * 16;  /* (1 header + 4096 glyphs) * 16 rows/glyph   */

   ImageSize = Height * (Width / 8); /* in bytes, calculated from pixels */

   FileSize = DataOffset + ImageSize;

   /* convert dots/inch to pixels/meter */
   if (dpi == 0) dpi = 96;
   ppm = (int)((double)dpi * 100.0 / 2.54 + 0.5);

   /*
      Generate the BMP Header
   */
   putchar ('B');
   putchar ('M');

   /*
      Calculate file size:

         BMP Header + InfoHeader + Color Table + Raster Data
   */
   output4 (FileSize);  /* FileSize */
   output4 (0x0000); /* reserved */

   /* Calculate DataOffset */
   output4 (DataOffset);

   /*
      InfoHeader
   */
   output4 (40);         /* Size of InfoHeader                       */
   output4 (Width);      /* Width of bitmap in pixels                */
   output4 (Height);     /* Height of bitmap in pixels               */
   output2 (1);          /* Planes (1 plane)                         */
   output2 (1);          /* BitCount (1 = monochrome)                */
   output4 (0);          /* Compression (0 = none)                   */
   output4 (ImageSize);  /* ImageSize, in bytes                      */
   output4 (ppm);        /* XpixelsPerM (96 dpi = 3780 pixels/meter) */
   output4 (ppm);        /* YpixelsPerM (96 dpi = 3780 pixels/meter) */
   output4 (2);          /* ColorsUsed (= 2)                         */
   output4 (2);          /* ColorsImportant (= 2)                    */
   output4 (0x00000000); /* black (reserved, B, G, R)                */
   output4 (0x00FFFFFF); /* white (reserved, B, G, R)                */

   /*
      Create header row bits.
   */
   snprintf (raw_header, HDR_LEN, "%s Plane %d", HEADER_STRING, plane);
   memset ((void *)header, 0, 16 * 16 * sizeof (int)); /* fill with white */
   memset ((void *)header_string, ' ', 32 * sizeof (char)); /* 32 spaces */
   header_string[32] = '\0';  /* null-terminated */

   hdrlen = strlen (raw_header);
   if (hdrlen > 32) hdrlen = 32;        /* only 32 columns to print header */
   startcol = 16 - ((hdrlen + 1) >> 1); /* to center header                */
   /* center up to 32 chars */
   memcpy (&header_string[startcol], raw_header, hdrlen);

   /* Copy each letter's bitmap from the plane_array[][] we constructed. */
   /* Each glyph must be single-width, to fit two glyphs in 16 pixels */
   for (j = 0; j < 16; j++) {
      for (i = 0; i < 16; i++) {
         header[i][j] =
            (ascii_bits[header_string[j+j  ] & 0x7F][i] & 0xFF00) |
            (ascii_bits[header_string[j+j+1] & 0x7F][i] >> 8);
      }
   }

   /*
      Create the left column legend.
   */
   memset ((void *)leftcol, 0, 4096 * 16 * sizeof (unsigned));

   for (codept = 0x0000; codept < 0x10000; codept += 0x10) {
      d1 = (codept >> 12) & 0xF; /* most significant hex digit */
      d2 = (codept >>  8) & 0xF;
      d3 = (codept >>  4) & 0xF;

      thisrow = codept >> 4; /* rows of 16 glyphs */

      /* fill in first and second digits */
      for (digitrow = 0; digitrow < 5; digitrow++) {
         leftcol[thisrow][2 + digitrow] =
            (hexdigit[d1][digitrow] << 10) |
            (hexdigit[d2][digitrow] <<  4);
      }

      /* fill in third digit */
      for (digitrow = 0; digitrow < 5; digitrow++) {
         leftcol[thisrow][9 + digitrow] = hexdigit[d3][digitrow] << 10;
      }
      leftcol[thisrow][9 + 4] |= 0xF << 4; /* underscore as 4th digit */

      for (i = 0; i < 15; i ++) {
         leftcol[thisrow][i] |= 0x00000002;      /* right border */
      }

      leftcol[thisrow][15] = 0x0000FFFE;        /* bottom border */

      if (d3 == 0xF) {                     /* 256-point boundary */
         leftcol[thisrow][15] |= 0x00FF0000;  /* longer tic mark */
      }

      if ((thisrow % 0x40) == 0x3F) {    /* 1024-point boundary */
         leftcol[thisrow][15] |= 0xFFFF0000; /* longest tic mark */
      }
   }

   /*
      Create the top row legend.
   */
   memset ((void *)toprow, 0, 16 * 16 * sizeof (unsigned));

   for (codept = 0x0; codept <= 0xF; codept++) {
      d1 = (codept >> 12) & 0xF; /* most significant hex digit */
      d2 = (codept >>  8) & 0xF;
      d3 = (codept >>  4) & 0xF;
      d4 =  codept        & 0xF; /* least significant hex digit */

      /* fill in last digit */
      for (digitrow = 0; digitrow < 5; digitrow++) {
         toprow[6 + digitrow][codept] = hexdigit[d4][digitrow] << 6;
      }
   }

   for (j = 0; j < 16; j++) {
      /* force bottom pixel row to be white, for separation from glyphs */
      toprow[15][j] = 0x0000;
   }

   /* 1 pixel row with left-hand legend line */
   for (j = 0; j < 16; j++) {
      toprow[14][j] |= 0xFFFF;
   }

   /* 14 rows with line on left to fill out this character row */
   for (i = 13; i >= 0; i--) {
      for (j = 0; j < 16; j++) {
         toprow[i][j] |= 0x0001;
      }
   }

   /*
      Now write the raster image.

      XOR each byte with 0xFF because black = 0, white = 1 in BMP.
   */

   /* Write the glyphs, bottom-up, left-to-right, in rows of 16 (i.e., 0x10) */
   for (i = 0xFFF0; i >= 0; i -= 0x10) {
      thisrow = i >> 4; /* 16 glyphs per row */
      for (j = 15; j >= 0; j--) {
         /* left-hand legend */
         putchar ((~leftcol[thisrow][j] >> 24) & 0xFF);
         putchar ((~leftcol[thisrow][j] >> 16) & 0xFF);
         putchar ((~leftcol[thisrow][j] >>  8) & 0xFF);
         putchar ( ~leftcol[thisrow][j]        & 0xFF);
         /* Unifont glyph */
         for (k = 0; k < 16; k++) {
            bytesout = ~plane_array[i+k][j] & 0xFFFF;
            putchar ((bytesout >> 8) & 0xFF);
            putchar ( bytesout       & 0xFF);
         }
      }
   }

   /*
      Write the top legend.
   */
   /* i == 15: bottom pixel row of header is output here */
   /* left-hand legend: solid black line except for right-most pixel */
   putchar (0x00);
   putchar (0x00);
   putchar (0x00);
   putchar (0x01);
   for (j = 0; j < 16; j++) {
      putchar ((~toprow[15][j] >> 8) & 0xFF);
      putchar ( ~toprow[15][j]       & 0xFF);
   }

   putchar (0xFF);
   putchar (0xFF);
   putchar (0xFF);
   putchar (0xFC);
   for (j = 0; j < 16; j++) {
      putchar ((~toprow[14][j] >> 8) & 0xFF);
      putchar ( ~toprow[14][j]       & 0xFF);
   }

   for (i = 13; i >= 0; i--) {
      putchar (0xFF);
      putchar (0xFF);
      putchar (0xFF);
      putchar (0xFD);
      for (j = 0; j < 16; j++) {
         putchar ((~toprow[i][j] >> 8) & 0xFF);
         putchar ( ~toprow[i][j]       & 0xFF);
      }
   }

   /*
      Write the header.
   */

   /* 7 completely white rows */
   for (i = 7; i >= 0; i--) {
      for (j = 0; j < 18; j++) {
         putchar (0xFF);
         putchar (0xFF);
      }
   }

   for (i = 15; i >= 0; i--) {
      /* left-hand legend */
      putchar (0xFF);
      putchar (0xFF);
      putchar (0xFF);
      putchar (0xFF);
      /* header glyph */
      for (j = 0; j < 16; j++) {
         bytesout = ~header[i][j] & 0xFFFF;
         putchar ((bytesout >> 8) & 0xFF);
         putchar ( bytesout       & 0xFF);
      }
   }

   /* 8 completely white rows at very top */
   for (i = 7; i >= 0; i--) {
      for (j = 0; j < 18; j++) {
      putchar (0xFF);
      putchar (0xFF);
      }
   }

   return;
}



/*
   genwidebmp generates the BMP output file from a bitmap parameter.
   This is a wide bitmap, 256 glyphs wide by 256 glyphs tall.
*/
void
genwidebmp (int plane_array[0x10000][16], int dpi, int tinynum, int plane)
{

   char header_string[257];
   char raw_header[HDR_LEN];
   int header[16][256]; /* header row, for chart title */
   int hdrlen;         /* length of HEADER_STRING */
   int startcol;       /* column to start printing header, for centering */

   unsigned leftcol[0x100][16]; /* code point legend on left side of chart */
   int d1, d2, d3, d4;          /* digits for filling leftcol[][] legend   */
   int codept;                  /* current starting code point for legend  */
   int thisrow;                 /* glyph row currently being rendered      */
   unsigned toprow[32][256];    /* code point legend on top of chart       */
   int digitrow;      /* row we're in (0..4) for the above hexdigit digits */
   int hexalpha1, hexalpha2;    /* to convert hex digits to ASCII          */

   /*
      DataOffset = BMP Header bytes + InfoHeader bytes + ColorTable bytes.
   */
   int DataOffset = 14 + 40 + 8; /* fixed size for monochrome BMP */
   int ImageSize;
   int FileSize;
   int Width, Height; /* bitmap image width and height in pixels */
   int ppm;     /* integer pixels per meter */

   int i, j, k;

   unsigned bytesout;

   void output4(int), output2(int);

   /*
      Image width and height, in pixels.

         N.B.: Width must be an even multiple of 32 pixels, or 4 bytes.
   */
   Width  = 258 * 16;  /* (           2 legend + 256 glyphs) * 16 pixels/glyph */
   Height = 260 * 16;  /* (2 header + 2 legend + 256 glyphs) * 16 rows/glyph   */

   ImageSize = Height * (Width / 8); /* in bytes, calculated from pixels */

   FileSize = DataOffset + ImageSize;

   /* convert dots/inch to pixels/meter */
   if (dpi == 0) dpi = 96;
   ppm = (int)((double)dpi * 100.0 / 2.54 + 0.5);

   /*
      Generate the BMP Header
   */
   putchar ('B');
   putchar ('M');
   /*
      Calculate file size:

         BMP Header + InfoHeader + Color Table + Raster Data
   */
   output4 (FileSize);  /* FileSize */
   output4 (0x0000); /* reserved */
   /* Calculate DataOffset */
   output4 (DataOffset);

   /*
      InfoHeader
   */
   output4 (40);         /* Size of InfoHeader                       */
   output4 (Width);      /* Width of bitmap in pixels                */
   output4 (Height);     /* Height of bitmap in pixels               */
   output2 (1);          /* Planes (1 plane)                         */
   output2 (1);          /* BitCount (1 = monochrome)                */
   output4 (0);          /* Compression (0 = none)                   */
   output4 (ImageSize);  /* ImageSize, in bytes                      */
   output4 (ppm);        /* XpixelsPerM (96 dpi = 3780 pixels/meter) */
   output4 (ppm);        /* YpixelsPerM (96 dpi = 3780 pixels/meter) */
   output4 (2);          /* ColorsUsed (= 2)                         */
   output4 (2);          /* ColorsImportant (= 2)                    */
   output4 (0x00000000); /* black (reserved, B, G, R)                */
   output4 (0x00FFFFFF); /* white (reserved, B, G, R)                */

   /*
      Create header row bits.
   */
   snprintf (raw_header, HDR_LEN, "%s Plane %d", HEADER_STRING, plane);
   memset ((void *)header, 0, 256 * 16 * sizeof (int)); /* fill with white */
   memset ((void *)header_string, ' ', 256 * sizeof (char)); /* 256 spaces */
   header_string[256] = '\0';  /* null-terminated */

   hdrlen = strlen (raw_header);
   /* Wide bitmap can print 256 columns, but limit to 32 columns for long bitmap. */
   if (hdrlen > 32) hdrlen = 32;
   startcol = 127 - ((hdrlen - 1) >> 1);  /* to center header */
   /* center up to 32 chars */
   memcpy (&header_string[startcol], raw_header, hdrlen);

   /* Copy each letter's bitmap from the plane_array[][] we constructed. */
   for (j = 0; j < 256; j++) {
      for (i = 0; i < 16; i++) {
         header[i][j] = ascii_bits[header_string[j] & 0x7F][i];
      }
   }

   /*
      Create the left column legend.
   */
   memset ((void *)leftcol, 0, 256 * 16 * sizeof (unsigned));

   for (codept = 0x0000; codept < 0x10000; codept += 0x100) {
      d1 = (codept >> 12) & 0xF; /* most significant hex digit */
      d2 = (codept >>  8) & 0xF;

      thisrow = codept >> 8; /* rows of 256 glyphs */

      /* fill in first and second digits */

      if (tinynum) { /* use 4x5 pixel glyphs */
         for (digitrow = 0; digitrow < 5; digitrow++) {
            leftcol[thisrow][6 + digitrow] =
               (hexdigit[d1][digitrow] << 10) |
               (hexdigit[d2][digitrow] <<  4);
         }
      }
      else { /* bigger numbers -- use glyphs from Unifont itself */
         /* convert hexadecimal digits to ASCII equivalent */
         hexalpha1 = d1 < 0xA ? '0' + d1 : 'A' + d1 - 0xA;
         hexalpha2 = d2 < 0xA ? '0' + d2 : 'A' + d2 - 0xA;

         for (i = 0 ; i < 16; i++) {
            leftcol[thisrow][i] =
               (ascii_bits[hexalpha1][i] << 2) |
               (ascii_bits[hexalpha2][i] >> 6);
         }
      }

      for (i = 0; i < 15; i ++) {
         leftcol[thisrow][i] |= 0x00000002;      /* right border */
      }

      leftcol[thisrow][15] = 0x0000FFFE;        /* bottom border */

      if (d2 == 0xF) {                     /* 4096-point boundary */
         leftcol[thisrow][15] |= 0x00FF0000;  /* longer tic mark */
      }

      if ((thisrow % 0x40) == 0x3F) {    /* 16,384-point boundary */
         leftcol[thisrow][15] |= 0xFFFF0000; /* longest tic mark */
      }
   }

   /*
      Create the top row legend.
   */
   memset ((void *)toprow, 0, 32 * 256 * sizeof (unsigned));

   for (codept = 0x00; codept <= 0xFF; codept++) {
      d3 = (codept >>  4) & 0xF;
      d4 =  codept        & 0xF; /* least significant hex digit */

      if (tinynum) {
         for (digitrow = 0; digitrow < 5; digitrow++) {
            toprow[16 + 6 + digitrow][codept] =
               (hexdigit[d3][digitrow] << 10) |
               (hexdigit[d4][digitrow] <<  4);
         }
      }
      else {
         /* convert hexadecimal digits to ASCII equivalent */
         hexalpha1 = d3 < 0xA ? '0' + d3 : 'A' + d3 - 0xA;
         hexalpha2 = d4 < 0xA ? '0' + d4 : 'A' + d4 - 0xA;
         for (i = 0 ; i < 16; i++) {
            toprow[14 + i][codept] =
               (ascii_bits[hexalpha1][i]     ) |
               (ascii_bits[hexalpha2][i] >> 7);
         }
      }
   }

   for (j = 0; j < 256; j++) {
      /* force bottom pixel row to be white, for separation from glyphs */
      toprow[16 + 15][j] = 0x0000;
   }

   /* 1 pixel row with left-hand legend line */
   for (j = 0; j < 256; j++) {
      toprow[16 + 14][j] |= 0xFFFF;
   }

   /* 14 rows with line on left to fill out this character row */
   for (i = 13; i >= 0; i--) {
      for (j = 0; j < 256; j++) {
         toprow[16 + i][j] |= 0x0001;
      }
   }

   /* Form the longer tic marks in top legend */
   for (i = 8; i < 16; i++) {
      for (j = 0x0F; j < 0x100; j += 0x10) {
         toprow[i][j] |= 0x0001;
      }
   }

   /*
      Now write the raster image.

      XOR each byte with 0xFF because black = 0, white = 1 in BMP.
   */

   /* Write the glyphs, bottom-up, left-to-right, in rows of 16 (i.e., 0x10) */
   for (i = 0xFF00; i >= 0; i -= 0x100) {
      thisrow = i >> 8; /* 256 glyphs per row */
      for (j = 15; j >= 0; j--) {
         /* left-hand legend */
         putchar ((~leftcol[thisrow][j] >> 24) & 0xFF);
         putchar ((~leftcol[thisrow][j] >> 16) & 0xFF);
         putchar ((~leftcol[thisrow][j] >>  8) & 0xFF);
         putchar ( ~leftcol[thisrow][j]        & 0xFF);
         /* Unifont glyph */
         for (k = 0x00; k < 0x100; k++) {
            bytesout = ~plane_array[i+k][j] & 0xFFFF;
            putchar ((bytesout >> 8) & 0xFF);
            putchar ( bytesout       & 0xFF);
         }
      }
   }

   /*
      Write the top legend.
   */
   /* i == 15: bottom pixel row of header is output here */
   /* left-hand legend: solid black line except for right-most pixel */
   putchar (0x00);
   putchar (0x00);
   putchar (0x00);
   putchar (0x01);
   for (j = 0; j < 256; j++) {
      putchar ((~toprow[16 + 15][j] >> 8) & 0xFF);
      putchar ( ~toprow[16 + 15][j]       & 0xFF);
   }

   putchar (0xFF);
   putchar (0xFF);
   putchar (0xFF);
   putchar (0xFC);
   for (j = 0; j < 256; j++) {
      putchar ((~toprow[16 + 14][j] >> 8) & 0xFF);
      putchar ( ~toprow[16 + 14][j]       & 0xFF);
   }

   for (i = 16 + 13; i >= 0; i--) {
      if (i >= 8) { /* make vertical stroke on right */
         putchar (0xFF);
         putchar (0xFF);
         putchar (0xFF);
         putchar (0xFD);
      }
      else { /* all white */
         putchar (0xFF);
         putchar (0xFF);
         putchar (0xFF);
         putchar (0xFF);
      }
      for (j = 0; j < 256; j++) {
         putchar ((~toprow[i][j] >> 8) & 0xFF);
         putchar ( ~toprow[i][j]       & 0xFF);
      }
   }

   /*
      Write the header.
   */

   /* 8 completely white rows */
   for (i = 7; i >= 0; i--) {
      for (j = 0; j < 258; j++) {
         putchar (0xFF);
         putchar (0xFF);
      }
   }

   for (i = 15; i >= 0; i--) {
      /* left-hand legend */
      putchar (0xFF);
      putchar (0xFF);
      putchar (0xFF);
      putchar (0xFF);
      /* header glyph */
      for (j = 0; j < 256; j++) {
         bytesout = ~header[i][j] & 0xFFFF;
         putchar ((bytesout >> 8) & 0xFF);
         putchar ( bytesout       & 0xFF);
      }
   }

   /* 8 completely white rows at very top */
   for (i = 7; i >= 0; i--) {
      for (j = 0; j < 258; j++) {
      putchar (0xFF);
      putchar (0xFF);
      }
   }

   return;
}

