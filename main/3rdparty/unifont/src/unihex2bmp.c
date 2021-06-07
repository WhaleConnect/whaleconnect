/*
   unihex2bmp - program to turn a GNU Unifont hex glyph page of 256 code
                points into a Microsoft Bitmap (.bmp) or Wireless Bitmap file

   Synopsis: unihex2bmp [-iin_file.hex] [-oout_file.bmp]
                [-f] [-phex_page_num] [-w]


   Author: Paul Hardy, unifoundry <at> unifoundry.com, December 2007
   
   Copyright (C) 2007, 2008, 2013, 2017 Paul Hardy

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
   - Adds capability to output triple-width and quadruple-width (31 pixels
     wide, not 32) glyphs.  The 32nd column in a glyph cell is occupied by
     the vertical cell border, so a quadruple-width glyph can only occupy
     the first 31 columns; the 32nd column is ignored.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXBUF 256


/*
   These are the GNU Unifont hex strings for '0'-'9' and 'A'-'F',
   for encoding as bit strings in row and column headers.

   Looking at the final bitmap as a grid of 32*32 bit tiles, the
   first row contains a hexadecimal character string of the first
   3 hex digits in a 4 digit Unicode character name; the top column
   contains a hex character string of the 4th (low-order) hex digit
   of the Unicode character.
*/
char *hex[18]= {
      "0030:00000000182442424242424224180000",  /* Hex digit 0 */
      "0031:000000000818280808080808083E0000",  /* Hex digit 1 */
      "0032:000000003C4242020C102040407E0000",  /* Hex digit 2 */
      "0033:000000003C4242021C020242423C0000",  /* Hex digit 3 */
      "0034:00000000040C142444447E0404040000",  /* Hex digit 4 */
      "0035:000000007E4040407C020202423C0000",  /* Hex digit 5 */
      "0036:000000001C2040407C424242423C0000",  /* Hex digit 6 */
      "0037:000000007E0202040404080808080000",  /* Hex digit 7 */
      "0038:000000003C4242423C424242423C0000",  /* Hex digit 8 */
      "0039:000000003C4242423E02020204380000",  /* Hex digit 9 */
      "0041:0000000018242442427E424242420000",  /* Hex digit A */
      "0042:000000007C4242427C424242427C0000",  /* Hex digit B */
      "0043:000000003C42424040404042423C0000",  /* Hex digit C */
      "0044:00000000784442424242424244780000",  /* Hex digit D */
      "0045:000000007E4040407C404040407E0000",  /* Hex digit E */
      "0046:000000007E4040407C40404040400000",  /* Hex digit F */
      "0055:000000004242424242424242423C0000",  /* Unicode 'U' */
      "002B:0000000000000808087F080808000000"   /* Unicode '+' */
   };
unsigned char hexbits[18][32]; /* The above digits converted into bitmap */

unsigned unipage=0;        /* Unicode page number, 0x00..0xff */
int flip=1;                /* transpose entire matrix as in Unicode book */



int
main (int argc, char *argv[])
{

   int i, j;                  /* loop variables                    */
   unsigned k0;               /* temp Unicode char variable        */
   unsigned swap;             /* temp variable for swapping values */
   char inbuf[256];           /* input buffer                      */
   unsigned filesize;         /* size of file in bytes             */
   unsigned bitmapsize;       /* size of bitmap image in bytes     */
   unsigned thischar;         /* the current character             */
   unsigned char thischarbyte; /* unsigned char lowest byte of Unicode char */
   int thischarrow;           /* row 0..15 where this character belongs  */
   int thiscol;               /* column 0..15 where this character belongs */
   int toppixelrow;           /* pixel row, 0..16*32-1               */
   unsigned lastpage=0;       /* the last Unicode page read in font file */
   int wbmp=0;                /* set to 1 if writing .wbmp format file */

   unsigned char bitmap[17*32][18*4]; /* final bitmap */
   unsigned char charbits[32][4];  /* bitmap for one character, 4 bytes/row */

   char *infile="", *outfile="";  /* names of input and output files */
   FILE *infp, *outfp;      /* file pointers of input and output files */

   int init();                  /* initializes bitmap row/col labeling, &c. */
   int hex2bit();               /* convert hex string --> bitmap */

   bitmapsize = 17*32*18*4;  /* 17 rows by 18 cols, each 4 bytes */

   if (argc > 1) {
      for (i = 1; i < argc; i++) {
         if (argv[i][0] == '-') {  /* this is an option argument */
            switch (argv[i][1]) {
               case 'f':  /* flip (transpose) glyphs in bitmap as in standard */
                  flip = !flip;
                  break;
               case 'i':  /* name of input file */
                  infile = &argv[i][2];
                  break;
               case 'o':  /* name of output file */
                  outfile = &argv[i][2];
                  break;
               case 'p':  /* specify a Unicode page other than default of 0 */
                  sscanf (&argv[i][2], "%x", &unipage); /* Get Unicode page */
                  break;
               case 'w':  /* write a .wbmp file instead of a .bmp file */
                  wbmp = 1;
                  break;
               default:   /* if unrecognized option, print list and exit */
                  fprintf (stderr, "\nSyntax:\n\n");
                  fprintf (stderr, "   %s -p<Unicode_Page> ", argv[0]);
                  fprintf (stderr, "-i<Input_File> -o<Output_File> -w\n\n");
                  fprintf (stderr, "   -w specifies .wbmp output instead of ");
                  fprintf (stderr, "default Windows .bmp output.\n\n");
                  fprintf (stderr, "   -p is followed by 1 to 6 ");
                  fprintf (stderr, "Unicode page hex digits ");
                  fprintf (stderr, "(default is Page 0).\n\n");
                  fprintf (stderr, "\nExample:\n\n");
                  fprintf (stderr, "   %s -p83 -iunifont.hex -ou83.bmp\n\n\n",
                         argv[0]);
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

   (void)init(bitmap); /* initialize bitmap with row/column headers, etc. */

   /*
      Read in the characters in the page
   */
   while (lastpage <= unipage && fgets (inbuf, MAXBUF-1, infp) != NULL) {
      sscanf (inbuf, "%x", &thischar);
      lastpage = thischar >> 8; /* keep Unicode page to see if we can stop */
      if (lastpage == unipage) {
         thischarbyte = (unsigned char)(thischar & 0xff);
         for (k0=0; inbuf[k0] != ':'; k0++);
         k0++;
         hex2bit (&inbuf[k0], charbits);  /* convert hex string to 32*4 bitmap */

         /*
            Now write character bitmap upside-down in page array, to match
            .bmp file order.  In the .wbmp` and .bmp files, white is a '1'
            bit and black is a '0' bit, so complement charbits[][].
         */

         thiscol = (thischarbyte & 0xf) + 2;  /* column number will be 1..16  */
         thischarrow = thischarbyte >> 4;     /* charcter row number, 0..15   */
         if (flip) {  /* swap row and column placement */
            swap = thiscol;
            thiscol = thischarrow;
            thischarrow = swap;
            thiscol += 2;       /* column index starts at 1 */
            thischarrow -= 2;   /* row index starts at 0    */
         }
         toppixelrow = 32 * (thischarrow + 1) - 1; /* from bottom to top    */
   
         /*
            Copy the center of charbits[][] because hex characters only
            occupy rows 8 to 23 and column byte 2 (and for 16 bit wide
            characters, byte 3).  The charbits[][] array was given 32 rows
            and 4 column bytes for completeness in the beginning.
         */
         for (i=8; i<24; i++) {
            bitmap[toppixelrow + i][(thiscol << 2) | 0] =
               ~charbits[i][0] & 0xff;
            bitmap[toppixelrow + i][(thiscol << 2) | 1] =
               ~charbits[i][1] & 0xff;
            bitmap[toppixelrow + i][(thiscol << 2) | 2] =
               ~charbits[i][2] & 0xff;
            /* Only use first 31 bits; leave vertical rule in 32nd column */
            bitmap[toppixelrow + i][(thiscol << 2) | 3] =
               ~charbits[i][3] & 0xfe;
         }
         /*
            Leave white space in 32nd column of rows 8, 14, 15, and 23
            to leave 16 pixel height upper, middle, and lower guides.
         */
         bitmap[toppixelrow +  8][(thiscol << 2) | 3] |= 1;
         bitmap[toppixelrow + 14][(thiscol << 2) | 3] |= 1;
         bitmap[toppixelrow + 15][(thiscol << 2) | 3] |= 1;
         bitmap[toppixelrow + 23][(thiscol << 2) | 3] |= 1;
      }
   }
   /*
      Now write the appropriate bitmap file format, either
      Wireless Bitmap or Microsoft Windows bitmap.
   */
   if (wbmp) {  /* Write a Wireless Bitmap .wbmp format file */
      /*
         Write WBMP header
      */
      fprintf (outfp, "%c", 0x00); /* Type of image; always 0 (monochrome) */
      fprintf (outfp, "%c", 0x00); /* Reserved; always 0                   */
      fprintf (outfp, "%c%c", 0x84, 0x40); /* Width  = 576 pixels          */
      fprintf (outfp, "%c%c", 0x84, 0x20); /* Height = 544 pixels          */
      /*
         Write bitmap image
      */
      for (toppixelrow=0; toppixelrow <= 17*32-1; toppixelrow++) {
         for (j=0; j<18; j++) {
            fprintf (outfp, "%c", bitmap[toppixelrow][(j<<2)    ]);
            fprintf (outfp, "%c", bitmap[toppixelrow][(j<<2) | 1]);
            fprintf (outfp, "%c", bitmap[toppixelrow][(j<<2) | 2]);
            fprintf (outfp, "%c", bitmap[toppixelrow][(j<<2) | 3]);
         }
      }
   }
   else {  /* otherwise, write a Microsoft Windows .bmp format file */
      /*
         Write the .bmp file -- start with the header, then write the bitmap
      */

      /* 'B', 'M' appears at start of every .bmp file */
      fprintf (outfp, "%c%c", 0x42, 0x4d);

      /* Write file size in bytes */
      filesize   = 0x3E + bitmapsize;
      fprintf (outfp, "%c", (unsigned char)((filesize        ) & 0xff));
      fprintf (outfp, "%c", (unsigned char)((filesize >> 0x08) & 0xff));
      fprintf (outfp, "%c", (unsigned char)((filesize >> 0x10) & 0xff));
      fprintf (outfp, "%c", (unsigned char)((filesize >> 0x18) & 0xff));

      /* Reserved - 0's */
      fprintf (outfp, "%c%c%c%c", 0x00, 0x00, 0x00, 0x00);

      /* Offset from start of file to bitmap data */
      fprintf (outfp, "%c%c%c%c", 0x3E, 0x00, 0x00, 0x00);

      /* Length of bitmap info header */
      fprintf (outfp, "%c%c%c%c", 0x28, 0x00, 0x00, 0x00);

      /* Width of bitmap in pixels */
      fprintf (outfp, "%c%c%c%c", 0x40, 0x02, 0x00, 0x00);

      /* Height of bitmap in pixels */
      fprintf (outfp, "%c%c%c%c", 0x20, 0x02, 0x00, 0x00);

      /* Planes in bitmap (fixed at 1) */
      fprintf (outfp, "%c%c", 0x01, 0x00);

      /* bits per pixel (1 = monochrome) */
      fprintf (outfp, "%c%c", 0x01, 0x00);

      /* Compression (0 = none) */
      fprintf (outfp, "%c%c%c%c", 0x00, 0x00, 0x00, 0x00);

      /* Size of bitmap data in bytes */
      fprintf (outfp, "%c", (unsigned char)((bitmapsize        ) & 0xff));
      fprintf (outfp, "%c", (unsigned char)((bitmapsize >> 0x08) & 0xff));
      fprintf (outfp, "%c", (unsigned char)((bitmapsize >> 0x10) & 0xff));
      fprintf (outfp, "%c", (unsigned char)((bitmapsize >> 0x18) & 0xff));

      /* Horizontal resolution in pixels per meter */
      fprintf (outfp, "%c%c%c%c", 0xC4, 0x0E, 0x00, 0x00);

      /* Vertical resolution in pixels per meter */
      fprintf (outfp, "%c%c%c%c", 0xC4, 0x0E, 0x00, 0x00);

      /* Number of colors used */
      fprintf (outfp, "%c%c%c%c", 0x02, 0x00, 0x00, 0x00);

      /* Number of important colors */
      fprintf (outfp, "%c%c%c%c", 0x02, 0x00, 0x00, 0x00);

      /* The color black: B=0x00, G=0x00, R=0x00, Filler=0xFF */
      fprintf (outfp, "%c%c%c%c", 0x00, 0x00, 0x00, 0x00);

      /* The color white: B=0xFF, G=0xFF, R=0xFF, Filler=0xFF */
      fprintf (outfp, "%c%c%c%c", 0xFF, 0xFF, 0xFF, 0x00);

      /*
         Now write the raw data bits.  Data is written from the lower
         left-hand corner of the image to the upper right-hand corner
         of the image.
      */
      for (toppixelrow=17*32-1; toppixelrow >= 0; toppixelrow--) {
         for (j=0; j<18; j++) {
            fprintf (outfp, "%c", bitmap[toppixelrow][(j<<2)    ]);
            fprintf (outfp, "%c", bitmap[toppixelrow][(j<<2) | 1]);
            fprintf (outfp, "%c", bitmap[toppixelrow][(j<<2) | 2]);

            fprintf (outfp, "%c", bitmap[toppixelrow][(j<<2) | 3]);
         }
      }
   }
   exit (0);
}

/*
   Convert the portion of a hex string after the ':' into a character bitmap.

   If string is >= 128 characters, it will fill all 4 bytes per row.
   If string is >= 64 characters and < 128, it will fill 2 bytes per row.
   Otherwise, it will fill 1 byte per row.
*/
int
hex2bit (char *instring, unsigned char character[32][4])
{

   int i;  /* current row in bitmap character */
   int j;  /* current character in input string */
   int k;  /* current byte in bitmap character */
   int width;  /* number of output bytes to fill - 1: 0, 1, 2, or 3 */

   for (i=0; i<32; i++)  /* erase previous character */
      character[i][0] = character[i][1] = character[i][2] = character[i][3] = 0; 
   j=0;  /* current location is at beginning of instring */

   if (strlen (instring) <= 34)  /* 32 + possible '\r', '\n' */
      width = 0;
   else if (strlen (instring) <= 66)  /* 64 + possible '\r', '\n' */
      width = 1;
   else if (strlen (instring) <= 98)  /* 96 + possible '\r', '\n' */
      width = 3;
   else  /* the maximum allowed is quadruple-width */
      width = 4;

   k = (width > 1) ? 0 : 1; /* if width > double, start at index 1 else at 0 */

   for (i=8; i<24; i++) {  /* 16 rows per input character, rows 8..23 */
      sscanf (&instring[j], "%2hhx", &character[i][k]);
      j += 2;
      if (width > 0) { /* add next pair of hex digits to this row */
         sscanf (&instring[j], "%2hhx", &character[i][k+1]);
         j += 2;
         if (width > 1) { /* add next pair of hex digits to this row */
            sscanf (&instring[j], "%2hhx", &character[i][k+2]);
            j += 2;
            if (width > 2) {  /* quadruple-width is maximum width */
               sscanf (&instring[j], "%2hhx", &character[i][k+3]);
               j += 2;
            }
         }
      }
   }

   return (0);
}

int
init (unsigned char bitmap[17*32][18*4])
{
   int i, j;
   unsigned char charbits[32][4];  /* bitmap for one character, 4 bytes/row */
   unsigned toppixelrow;
   unsigned thiscol;
   unsigned char pnybble0, pnybble1, pnybble2, pnybble3;

   for (i=0; i<18; i++) { /* bitmaps for '0'..'9', 'A'-'F', 'u', '+' */

      hex2bit (&hex[i][5], charbits);  /* convert hex string to 32*4 bitmap */

      for (j=0; j<32; j++) hexbits[i][j] = ~charbits[j][1];
   }

   /*
      Initialize bitmap to all white.
   */
   for (toppixelrow=0; toppixelrow < 17*32; toppixelrow++) {
      for (thiscol=0; thiscol<18; thiscol++) {
         bitmap[toppixelrow][(thiscol << 2)    ] = 0xff;
         bitmap[toppixelrow][(thiscol << 2) | 1] = 0xff;
         bitmap[toppixelrow][(thiscol << 2) | 2] = 0xff;
         bitmap[toppixelrow][(thiscol << 2) | 3] = 0xff;
      }
   }
   /*
      Write the "u+nnnn" table header in the upper left-hand corner,
      where nnnn is the upper 16 bits of a 32-bit Unicode assignment.
   */
   pnybble3 = (unipage >> 20);
   pnybble2 = (unipage >> 16) & 0xf;
   pnybble1 = (unipage >> 12) & 0xf;
   pnybble0 = (unipage >>  8) & 0xf;
   for (i=0; i<32; i++) {
      bitmap[i][1] = hexbits[16][i];  /* copy 'u' */
      bitmap[i][2] = hexbits[17][i];  /* copy '+' */
      bitmap[i][3] = hexbits[pnybble3][i];
      bitmap[i][4] = hexbits[pnybble2][i];
      bitmap[i][5] = hexbits[pnybble1][i];
      bitmap[i][6] = hexbits[pnybble0][i];
   }
   /*
      Write low-order 2 bytes of Unicode number assignments, as hex labels
   */
   pnybble3 = (unipage >> 4) & 0xf;  /* Highest-order hex digit */
   pnybble2 = (unipage     ) & 0xf;  /* Next highest-order hex digit */
   /*
      Write the column headers in bitmap[][] (row headers if flipped)
   */
   toppixelrow = 32 * 17 - 1; /* maximum pixel row number */
   /*
      Label the column headers. The hexbits[][] bytes are split across two
      bitmap[][] entries to center a the hex digits in a column of 4 bytes.
      OR highest byte with 0xf0 and lowest byte with 0x0f to make outer
      nybbles white (0=black, 1-white).
   */
   for (i=0; i<16; i++) {
      for (j=0; j<32; j++) {
         if (flip) {  /* transpose matrix */
            bitmap[j][((i+2) << 2) | 0]  = (hexbits[pnybble3][j] >> 4) | 0xf0;
            bitmap[j][((i+2) << 2) | 1]  = (hexbits[pnybble3][j] << 4) |
                                           (hexbits[pnybble2][j] >> 4);
            bitmap[j][((i+2) << 2) | 2]  = (hexbits[pnybble2][j] << 4) |
                                           (hexbits[i][j] >> 4);
            bitmap[j][((i+2) << 2) | 3]  = (hexbits[i][j] << 4) | 0x0f;
         }
         else {
            bitmap[j][((i+2) << 2) | 1] = (hexbits[i][j] >> 4) | 0xf0;
            bitmap[j][((i+2) << 2) | 2] = (hexbits[i][j] << 4) | 0x0f;
         }
      }
   }
   /*
      Now use the single hex digit column graphics to label the row headers.
   */
   for (i=0; i<16; i++) {
      toppixelrow = 32 * (i + 1) - 1; /* from bottom to top    */

      for (j=0; j<32; j++) {
         if (!flip) {  /* if not transposing matrix */
            bitmap[toppixelrow + j][4] = hexbits[pnybble3][j];
            bitmap[toppixelrow + j][5] = hexbits[pnybble2][j];
         }
         bitmap[toppixelrow + j][6] = hexbits[i][j];
      }
   }
   /*
      Now draw grid lines in bitmap, around characters we just copied.
   */
   /* draw vertical lines 2 pixels wide */
   for (i=1*32; i<17*32; i++) {
      if ((i & 0x1f) == 7)
         i++;
      else if ((i & 0x1f) == 14)
         i += 2;
      else if ((i & 0x1f) == 22)
         i++;
      for (j=1; j<18; j++) {
         bitmap[i][(j << 2) | 3] &= 0xfe;
      }
   }
   /* draw horizontal lines 1 pixel tall */
   for (i=1*32-1; i<18*32-1; i+=32) {
      for (j=2; j<18; j++) {
         bitmap[i][(j << 2)    ] = 0x00;
         bitmap[i][(j << 2) | 1] = 0x81;
         bitmap[i][(j << 2) | 2] = 0x81;
         bitmap[i][(j << 2) | 3] = 0x00;
      }
   }
   /* fill in top left corner pixel of grid */
   bitmap[31][7] = 0xfe;

   return (0);
}
