/*
   unibmp2hex - program to turn a .bmp or .wbmp glyph matrix into a
                GNU Unifont hex glyph set of 256 characters

   Synopsis: unibmp2hex [-iin_file.bmp] [-oout_file.hex] [-phex_page_num] [-w]


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
      - Modify to allow hard-coding of quadruple-width hex glyphs.
        The 32nd column (rightmost column) is cleared to zero, because
        that column contains the vertical cell border.
      - Set U+9FD8..U+9FE9 (complex CJK) to be quadruple-width.
      - Set U+011A00..U+011A4F (Masaram Gondi, non-digits) to be wide.
      - Set U+011A50..U+011AAF (Soyombo) to be wide.

   8 July 2017 [Paul Hardy]:
      - All CJK glyphs in the range U+4E00..u+9FFF are double width
        again; commented out the line that sets U+9FD8..U+9FE9 to be
        quadruple width.

   6 August 2017 [Paul Hardy]:
      - Remove hard-coding of U+01D200..U+01D24F Ancient Greek Musical
        Notation to double-width; allow range to be dual-width.

   12 August 2017 [Paul Hardy]:
      - Remove Miao script from list of wide scripts, so it can contain
        single-width glyphs.

   26 December 2017 Paul Hardy:
      - Removed Tibetan from list of wide scripts, so it can contain
        single-width glyphs.
      - Added a number of scripts to be explicitly double-width in case
        they are redrawn.
      - Added Miao script back as wide, because combining glyphs are
        added back to font/plane01/plane01-combining.txt.

   05 June 2018 Paul Hardy:
      - Made U+2329] and U+232A wide.
      - Added to wide settings for CJK Compatibility Forms over entire range.
      - Made Kayah Li script double-width.
      - Made U+232A (Right-pointing Angle Bracket) double-width.
      - Made U+01F5E7 (Three Rays Right) double-width.

   July 2018 Paul Hardy:
      - Changed 2017 to 2018 in previous change entry.
      - Added Dogra (U+011800..U+01184F) as double width.
      - Added Makasar (U+011EE0..U+011EFF) as dobule width.

   23 February 2019 [Paul Hardy]:
      - Set U+119A0..U+119FF (Nandinagari) to be wide.
      - Set U+1E2C0..U+1E2FF (Wancho) to be wide.

   25 May 2019 [Paul Hardy]:
      - Added support for the case when the original .bmp monochrome
        file has been converted to a 32 bit per pixel RGB file.
      - Added support for bitmap images stored from either top to bottom
        or bottom to top.
      - Add DEBUG compile flag to print header information, to ease
        adding support for additional bitmap formats in the future.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXBUF 256


unsigned hexdigit[16][4]; /* 32 bit representation of 16x8 0..F bitmap   */

unsigned uniplane=0;        /* Unicode plane number, 0..0xff ff ff       */
unsigned planeset=0;        /* =1: use plane specified with -p parameter */
unsigned flip=0;            /* =1 if we're transposing glyph matrix      */
unsigned forcewide=0;       /* =1 to set each glyph to 16 pixels wide    */

/* The six Unicode plane digits, from left-most (0) to right-most (5)    */
unsigned unidigit[6][4];


/* Bitmap Header parameters */
struct {
   char filetype[2];
   int file_size;
   int image_offset;
   int info_size;
   int width;
   int height;
   int nplanes;
   int bits_per_pixel;
   int compression;
   int image_size;
   int x_ppm;
   int y_ppm;
   int ncolors;
   int important_colors;
} bmp_header;

/* Bitmap Color Table -- maximum of 256 colors in a BMP file */
unsigned char color_table[256][4];  /* R, G, B, alpha for up to 256 colors */

// #define DEBUG

int
main (int argc, char *argv[])
{

   int i, j, k;               /* loop variables                       */
   unsigned char inchar;       /* temporary input character */
   char header[MAXBUF];        /* input buffer for bitmap file header */
   int wbmp=0; /* =0 for Windows Bitmap (.bmp); 1 for Wireless Bitmap (.wbmp) */
   int fatal; /* =1 if a fatal error occurred */
   int match; /* =1 if we're still matching a pattern, 0 if no match */
   int empty1, empty2; /* =1 if bytes tested are all zeroes */
   unsigned char thischar1[16], thischar2[16]; /* bytes of hex char */
   unsigned char thischar0[16], thischar3[16]; /* bytes for quadruple-width */
   int thisrow; /* index to point into thischar1[] and thischar2[] */
   int tmpsum;  /* temporary sum to see if a character is blank */
   unsigned this_pixel;  /* color of one pixel, if > 1 bit per pixel */
   unsigned next_pixels; /* pending group of 8 pixels being read */
   unsigned color_mask = 0x00;  /* to invert monochrome bitmap, set to 0xFF */

   unsigned char bitmap[17*32][18*32/8]; /* final bitmap */
   /* For wide array:
         0 = don't force glyph to double-width;
         1 = force glyph to double-width;
         4 = force glyph to quadruple-width.
   */
   char wide[0x200000]={0x200000 * 0};

   char *infile="", *outfile="";  /* names of input and output files */
   FILE *infp, *outfp;      /* file pointers of input and output files */

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
               case 'p':  /* specify a Unicode plane */
                  sscanf (&argv[i][2], "%x", &uniplane); /* Get Unicode plane */
                  planeset = 1; /* Use specified range, not what's in bitmap */
                  break;
               case 'w': /* force wide (16 pixels) for each glyph */
                  forcewide = 1;
                  break;
               default:   /* if unrecognized option, print list and exit */
                  fprintf (stderr, "\nSyntax:\n\n");
                  fprintf (stderr, "   %s -p<Unicode_Page> ", argv[0]);
                  fprintf (stderr, "-i<Input_File> -o<Output_File> -w\n\n");
                  fprintf (stderr, "   -w specifies .wbmp output instead of ");
                  fprintf (stderr, "default Windows .bmp output.\n\n");
                  fprintf (stderr, "   -p is followed by 1 to 6 ");
                  fprintf (stderr, "Unicode plane hex digits ");
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
   /*
      Initialize selected code points for double width (16x16).
      Double-width is forced in cases where a glyph (usually a combining
      glyph) only occupies the left-hand side of a 16x16 grid, but must
      be rendered as double-width to appear properly with other glyphs
      in a given script.  If additions were made to a script after
      Unicode 5.0, the Unicode version is given in parentheses after
      the script name.
   */
   for (i = 0x0700; i <= 0x074F; i++) wide[i] = 1; /* Syriac                 */
   for (i = 0x0800; i <= 0x083F; i++) wide[i] = 1; /* Samaritan (5.2)        */
   for (i = 0x0900; i <= 0x0DFF; i++) wide[i] = 1; /* Indic                  */
   for (i = 0x1000; i <= 0x109F; i++) wide[i] = 1; /* Myanmar                */
   for (i = 0x1100; i <= 0x11FF; i++) wide[i] = 1; /* Hangul Jamo            */
   for (i = 0x1400; i <= 0x167F; i++) wide[i] = 1; /* Canadian Aboriginal    */
   for (i = 0x1700; i <= 0x171F; i++) wide[i] = 1; /* Tagalog                */
   for (i = 0x1720; i <= 0x173F; i++) wide[i] = 1; /* Hanunoo                */
   for (i = 0x1740; i <= 0x175F; i++) wide[i] = 1; /* Buhid                  */
   for (i = 0x1760; i <= 0x177F; i++) wide[i] = 1; /* Tagbanwa               */
   for (i = 0x1780; i <= 0x17FF; i++) wide[i] = 1; /* Khmer                  */
   for (i = 0x18B0; i <= 0x18FF; i++) wide[i] = 1; /* Ext. Can. Aboriginal   */
   for (i = 0x1800; i <= 0x18AF; i++) wide[i] = 1; /* Mongolian              */
   for (i = 0x1900; i <= 0x194F; i++) wide[i] = 1; /* Limbu                  */
// for (i = 0x1980; i <= 0x19DF; i++) wide[i] = 1; /* New Tai Lue            */
   for (i = 0x1A00; i <= 0x1A1F; i++) wide[i] = 1; /* Buginese               */
   for (i = 0x1A20; i <= 0x1AAF; i++) wide[i] = 1; /* Tai Tham (5.2)         */
   for (i = 0x1B00; i <= 0x1B7F; i++) wide[i] = 1; /* Balinese               */
   for (i = 0x1B80; i <= 0x1BBF; i++) wide[i] = 1; /* Sundanese (5.1)        */
   for (i = 0x1BC0; i <= 0x1BFF; i++) wide[i] = 1; /* Batak (6.0)            */
   for (i = 0x1C00; i <= 0x1C4F; i++) wide[i] = 1; /* Lepcha (5.1)           */
   for (i = 0x1CC0; i <= 0x1CCF; i++) wide[i] = 1; /* Sundanese Supplement   */
   for (i = 0x1CD0; i <= 0x1CFF; i++) wide[i] = 1; /* Vedic Extensions (5.2) */
   wide[0x2329] = wide[0x232A] = 1; /* Left- & Right-pointing Angle Brackets */
   for (i = 0x2E80; i <= 0xA4CF; i++) wide[i] = 1; /* CJK                    */
// for (i = 0x9FD8; i <= 0x9FE9; i++) wide[i] = 4; /* CJK quadruple-width    */
   for (i = 0xA900; i <= 0xA92F; i++) wide[i] = 1; /* Kayah Li (5.1)         */
   for (i = 0xA930; i <= 0xA95F; i++) wide[i] = 1; /* Rejang (5.1)           */
   for (i = 0xA960; i <= 0xA97F; i++) wide[i] = 1; /* Hangul Jamo Extended-A */
   for (i = 0xA980; i <= 0xA9DF; i++) wide[i] = 1; /* Javanese (5.2)         */
   for (i = 0xAA00; i <= 0xAA5F; i++) wide[i] = 1; /* Cham (5.1)             */
   for (i = 0xA9E0; i <= 0xA9FF; i++) wide[i] = 1; /* Myanmar Extended-B     */
   for (i = 0xAA00; i <= 0xAA5F; i++) wide[i] = 1; /* Cham                   */
   for (i = 0xAA60; i <= 0xAA7F; i++) wide[i] = 1; /* Myanmar Extended-A     */
   for (i = 0xAAE0; i <= 0xAAFF; i++) wide[i] = 1; /* Meetei Mayek Ext (6.0) */
   for (i = 0xABC0; i <= 0xABFF; i++) wide[i] = 1; /* Meetei Mayek (5.2)     */
   for (i = 0xAC00; i <= 0xD7AF; i++) wide[i] = 1; /* Hangul Syllables       */
   for (i = 0xD7B0; i <= 0xD7FF; i++) wide[i] = 1; /* Hangul Jamo Extended-B */
   for (i = 0xF900; i <= 0xFAFF; i++) wide[i] = 1; /* CJK Compatibility      */
   for (i = 0xFE10; i <= 0xFE1F; i++) wide[i] = 1; /* Vertical Forms         */
   for (i = 0xFE30; i <= 0xFE60; i++) wide[i] = 1; /* CJK Compatibility Forms*/
   for (i = 0xFFE0; i <= 0xFFE6; i++) wide[i] = 1; /* CJK Compatibility Forms*/

   wide[0x303F] = 0; /* CJK half-space fill */

   /* Supplemental Multilingual Plane (Plane 01) */
   for (i = 0x010A00; i <= 0x010A5F; i++) wide[i] = 1; /* Kharoshthi         */
   for (i = 0x011000; i <= 0x01107F; i++) wide[i] = 1; /* Brahmi             */
   for (i = 0x011080; i <= 0x0110CF; i++) wide[i] = 1; /* Kaithi             */
   for (i = 0x011100; i <= 0x01114F; i++) wide[i] = 1; /* Chakma             */
   for (i = 0x011180; i <= 0x0111DF; i++) wide[i] = 1; /* Sharada            */
   for (i = 0x011200; i <= 0x01124F; i++) wide[i] = 1; /* Khojki             */
   for (i = 0x0112B0; i <= 0x0112FF; i++) wide[i] = 1; /* Khudawadi          */
   for (i = 0x011300; i <= 0x01137F; i++) wide[i] = 1; /* Grantha            */
   for (i = 0x011400; i <= 0x01147F; i++) wide[i] = 1; /* Newa               */
   for (i = 0x011480; i <= 0x0114DF; i++) wide[i] = 1; /* Tirhuta            */
   for (i = 0x011580; i <= 0x0115FF; i++) wide[i] = 1; /* Siddham            */
   for (i = 0x011600; i <= 0x01165F; i++) wide[i] = 1; /* Modi               */
   for (i = 0x011660; i <= 0x01167F; i++) wide[i] = 1; /* Mongolian Suppl.   */
   for (i = 0x011680; i <= 0x0116CF; i++) wide[i] = 1; /* Takri              */
   for (i = 0x011700; i <= 0x01173F; i++) wide[i] = 1; /* Ahom               */
   for (i = 0x011800; i <= 0x01184F; i++) wide[i] = 1; /* Dogra              */
   for (i = 0x011900; i <= 0x01195F; i++) wide[i] = 1; /* Dives Akuru        */
   for (i = 0x0119A0; i <= 0x0119FF; i++) wide[i] = 1; /* Nandinagari        */
   for (i = 0x011A00; i <= 0x011A4F; i++) wide[i] = 1; /* Zanabazar Square   */
   for (i = 0x011A50; i <= 0x011AAF; i++) wide[i] = 1; /* Soyombo            */
   for (i = 0x011C00; i <= 0x011C6F; i++) wide[i] = 1; /* Bhaiksuki          */
   for (i = 0x011C70; i <= 0x011CBF; i++) wide[i] = 1; /* Marchen            */
   for (i = 0x011D00; i <= 0x011D5F; i++) wide[i] = 1; /* Masaram Gondi      */
   for (i = 0x011EE0; i <= 0x011EFF; i++) wide[i] = 1; /* Makasar            */
   /* Make Bassa Vah all single width or all double width */
   for (i = 0x016AD0; i <= 0x016AFF; i++) wide[i] = 1; /* Bassa Vah          */
   for (i = 0x016B00; i <= 0x016B8F; i++) wide[i] = 1; /* Pahawh Hmong       */
   for (i = 0x016F00; i <= 0x016F9F; i++) wide[i] = 1; /* Miao               */
   for (i = 0x016FE0; i <= 0x016FFF; i++) wide[i] = 1; /* Ideograph Sym/Punct*/
   for (i = 0x017000; i <= 0x0187FF; i++) wide[i] = 1; /* Tangut             */
   for (i = 0x018800; i <= 0x018AFF; i++) wide[i] = 1; /* Tangut Components  */
   for (i = 0x01B000; i <= 0x01B0FF; i++) wide[i] = 1; /* Kana Supplement    */
   for (i = 0x01B100; i <= 0x01B12F; i++) wide[i] = 1; /* Kana Extended-A    */
   for (i = 0x01B170; i <= 0x01B2FF; i++) wide[i] = 1; /* Nushu              */
   for (i = 0x01D100; i <= 0x01D1FF; i++) wide[i] = 1; /* Musical Symbols    */
   for (i = 0x01D800; i <= 0x01DAAF; i++) wide[i] = 1; /* Sutton SignWriting */
   for (i = 0x01E2C0; i <= 0x01E2FF; i++) wide[i] = 1; /* Wancho             */
   for (i = 0x01E800; i <= 0x01E8DF; i++) wide[i] = 1; /* Mende Kikakui      */
   for (i = 0x01F200; i <= 0x01F2FF; i++) wide[i] = 1; /* Encl Ideograp Suppl*/
   wide[0x01F5E7] = 1;                                 /* Three Rays Right   */

   /*
      Determine whether or not the file is a Microsoft Windows Bitmap file.
      If it starts with 'B', 'M', assume it's a Windows Bitmap file.
      Otherwise, assume it's a Wireless Bitmap file.

      WARNING: There isn't much in the way of error checking here --
      if you give it a file that wasn't first created by hex2bmp.c,
      all bets are off.
   */
   fatal = 0;  /* assume everything is okay with reading input file */
   if ((header[0] = fgetc (infp)) != EOF) {
      if ((header[1] = fgetc (infp)) != EOF) {
         if (header[0] == 'B' && header[1] == 'M') {
            wbmp = 0; /* Not a Wireless Bitmap -- it's a Windows Bitmap */
         }
         else {
            wbmp = 1; /* Assume it's a Wireless Bitmap */
         }
      }
      else
         fatal = 1;
   }
   else
      fatal = 1;

   if (fatal) {
      fprintf (stderr, "Fatal error; end of input file.\n\n");
      exit (1);
   }
   /*
      If this is a Wireless Bitmap (.wbmp) format file,
      skip the header and point to the start of the bitmap itself.
   */
   if (wbmp) {
      for (i=2; i<6; i++)
         header[i] = fgetc (infp);
      /*
         Now read the bitmap.
      */
      for (i=0; i < 32*17; i++) {
         for (j=0; j < 32*18/8; j++) {
            inchar = fgetc (infp);
            bitmap[i][j] = ~inchar;  /* invert bits for proper color */
         }
      }
   }
   /*
      Otherwise, treat this as a Windows Bitmap file, because we checked
      that it began with "BM".  Save the header contents for future use.
      Expect a 14 byte standard BITMAPFILEHEADER format header followed
      by a 40 byte standard BITMAPINFOHEADER Device Independent Bitmap
      header, with data stored in little-endian format.
   */
   else {
      for (i = 2; i < 54; i++)
         header[i] = fgetc (infp);

      bmp_header.filetype[0] = 'B';
      bmp_header.filetype[1] = 'M';

      bmp_header.file_size =
          (header[2] & 0xFF)        | ((header[3] & 0xFF) <<  8) |
         ((header[4] & 0xFF) << 16) | ((header[5] & 0xFF) << 24);

      /* header bytes 6..9 are reserved */

      bmp_header.image_offset =
          (header[10] & 0xFF)        | ((header[11] & 0xFF) <<  8) |
         ((header[12] & 0xFF) << 16) | ((header[13] & 0xFF) << 24);

      bmp_header.info_size =
          (header[14] & 0xFF)        | ((header[15] & 0xFF) <<  8) |
         ((header[16] & 0xFF) << 16) | ((header[17] & 0xFF) << 24);

      bmp_header.width =
          (header[18] & 0xFF)        | ((header[19] & 0xFF) <<  8) |
         ((header[20] & 0xFF) << 16) | ((header[21] & 0xFF) << 24);

      bmp_header.height =
          (header[22] & 0xFF)        | ((header[23] & 0xFF) <<  8) |
         ((header[24] & 0xFF) << 16) | ((header[25] & 0xFF) << 24);

      bmp_header.nplanes =
          (header[26] & 0xFF)        | ((header[27] & 0xFF) <<  8);

      bmp_header.bits_per_pixel =
          (header[28] & 0xFF)        | ((header[29] & 0xFF) <<  8);

      bmp_header.compression =
          (header[30] & 0xFF)        | ((header[31] & 0xFF) <<  8) |
         ((header[32] & 0xFF) << 16) | ((header[33] & 0xFF) << 24);

      bmp_header.image_size =
          (header[34] & 0xFF)        | ((header[35] & 0xFF) <<  8) |
         ((header[36] & 0xFF) << 16) | ((header[37] & 0xFF) << 24);

      bmp_header.x_ppm =
          (header[38] & 0xFF)        | ((header[39] & 0xFF) <<  8) |
         ((header[40] & 0xFF) << 16) | ((header[41] & 0xFF) << 24);

      bmp_header.y_ppm =
          (header[42] & 0xFF)        | ((header[43] & 0xFF) <<  8) |
         ((header[44] & 0xFF) << 16) | ((header[45] & 0xFF) << 24);

      bmp_header.ncolors =
          (header[46] & 0xFF)        | ((header[47] & 0xFF) <<  8) |
         ((header[48] & 0xFF) << 16) | ((header[49] & 0xFF) << 24);

      bmp_header.important_colors =
          (header[50] & 0xFF)        | ((header[51] & 0xFF) <<  8) |
         ((header[52] & 0xFF) << 16) | ((header[53] & 0xFF) << 24);

      if (bmp_header.ncolors == 0)
         bmp_header.ncolors = 1 << bmp_header.bits_per_pixel;

      /* If a Color Table exists, read it */
      if (bmp_header.ncolors > 0 && bmp_header.bits_per_pixel <= 8) {
         for (i = 0; i < bmp_header.ncolors; i++) {
            color_table[i][0] = fgetc (infp);  /* Red   */
            color_table[i][1] = fgetc (infp);  /* Green */
            color_table[i][2] = fgetc (infp);  /* Blue  */
            color_table[i][3] = fgetc (infp);  /* Alpha */
         }
         /*
            Determine from the first color table entry whether we
            are inverting the resulting bitmap image.
         */
         if ( (color_table[0][0] + color_table[0][1] + color_table[0][2]) 
              < (3 * 128) ) {
            color_mask = 0xFF;
         }
      }

#ifdef DEBUG

      /*
         Print header info for possibly adding support for
         additional file formats in the future, to determine
         how the bitmap is encoded.
      */
      fprintf (stderr, "Filetype: '%c%c'\n",
                       bmp_header.filetype[0], bmp_header.filetype[1]);
      fprintf (stderr, "File Size: %d\n", bmp_header.file_size);
      fprintf (stderr, "Image Offset: %d\n", bmp_header.image_offset);
      fprintf (stderr, "Info Header Size: %d\n", bmp_header.info_size);
      fprintf (stderr, "Image Width: %d\n", bmp_header.width);
      fprintf (stderr, "Image Height: %d\n", bmp_header.height);
      fprintf (stderr, "Number of Planes: %d\n", bmp_header.nplanes);
      fprintf (stderr, "Bits per Pixel: %d\n", bmp_header.bits_per_pixel);
      fprintf (stderr, "Compression Method: %d\n", bmp_header.compression);
      fprintf (stderr, "Image Size: %d\n", bmp_header.image_size);
      fprintf (stderr, "X Pixels per Meter: %d\n", bmp_header.x_ppm);
      fprintf (stderr, "Y Pixels per Meter: %d\n", bmp_header.y_ppm);
      fprintf (stderr, "Number of Colors: %d\n", bmp_header.ncolors);
      fprintf (stderr, "Important Colors: %d\n", bmp_header.important_colors);

#endif

      /*
         Now read the bitmap.
      */
      for (i = 32*17-1; i >= 0; i--) {
         for (j=0; j < 32*18/8; j++) {
            next_pixels = 0x00;  /* initialize next group of 8 pixels */
            /* Read a monochrome image -- the original case */
            if (bmp_header.bits_per_pixel == 1) {
               next_pixels = fgetc (infp);
            }
            /* Read a 32 bit per pixel RGB image; convert to monochrome */
            else if (bmp_header.bits_per_pixel == 32) {
               next_pixels = 0;
               for (k = 0; k < 8; k++) {  /* get next 8 pixels */
                  this_pixel = (fgetc (infp) & 0xFF) +
                               (fgetc (infp) & 0xFF) +
                               (fgetc (infp) & 0xFF);

                  (void) fgetc (infp);  /* ignore alpha value */

                  /* convert RGB color space to monochrome */
                  if (this_pixel >= (128 * 3))
                     this_pixel = 0;
                  else
                     this_pixel = 1;

                  /* shift next pixel color into place for 8 pixels total */
                  next_pixels = (next_pixels << 1) | this_pixel;
               }
            }
            if (bmp_header.height < 0) {  /* Bitmap drawn top to bottom */
               bitmap [(32*17-1) - i] [j] = next_pixels;
            }
            else {  /* Bitmap drawn bottom to top */
               bitmap [i][j] = next_pixels;
            }
         }
      }

      /*
         If any bits are set in color_mask, apply it to
         entire bitmap to invert black <--> white.
      */
      if (color_mask != 0x00) {
         for (i = 32*17-1; i >= 0; i--) {
            for (j=0; j < 32*18/8; j++) {
               bitmap [i][j] ^= color_mask;
            }
         }
      }

   }

   /*
      We've read the entire file.  Now close the input file pointer.
   */
   fclose (infp);
  /*
      We now have the header portion in the header[] array,
      and have the bitmap portion from top-to-bottom in the bitmap[] array.
   */
   /*
      If no Unicode range (U+nnnnnn00 through U+nnnnnnFF) was specified
      with a -p parameter, determine the range from the digits in the
      bitmap itself.

      Store bitmaps for the hex digit patterns that this file uses.
   */
   if (!planeset) {  /* If Unicode range not specified with -p parameter */
      for (i = 0x0; i <= 0xF; i++) {  /* hex digit pattern we're storing */
         for (j = 0; j < 4; j++) {
            hexdigit[i][j]   =
               ((unsigned)bitmap[32 * (i+1) + 4 * j + 8    ][6] << 24 ) |
               ((unsigned)bitmap[32 * (i+1) + 4 * j + 8 + 1][6] << 16 ) |
               ((unsigned)bitmap[32 * (i+1) + 4 * j + 8 + 2][6] <<  8 ) |
               ((unsigned)bitmap[32 * (i+1) + 4 * j + 8 + 3][6]       );
         }
      }
      /*
         Read the Unicode plane digits into arrays for comparison, to
         determine the upper four hex digits of the glyph addresses.
      */
      for (i = 0; i < 4; i++) {
         for (j = 0; j < 4; j++) {
            unidigit[i][j] =
               ((unsigned)bitmap[32 * 0 + 4 * j + 8 + 1][i + 3] << 24 ) |
               ((unsigned)bitmap[32 * 0 + 4 * j + 8 + 2][i + 3] << 16 ) |
               ((unsigned)bitmap[32 * 0 + 4 * j + 8 + 3][i + 3] <<  8 ) |
               ((unsigned)bitmap[32 * 0 + 4 * j + 8 + 4][i + 3]       );
         }
      }
   
      tmpsum = 0;
      for (i = 4; i < 6; i++) {
         for (j = 0; j < 4; j++) {
            unidigit[i][j] =
               ((unsigned)bitmap[32 * 1 + 4 * j + 8    ][i] << 24 ) |
               ((unsigned)bitmap[32 * 1 + 4 * j + 8 + 1][i] << 16 ) |
               ((unsigned)bitmap[32 * 1 + 4 * j + 8 + 2][i] <<  8 ) |
               ((unsigned)bitmap[32 * 1 + 4 * j + 8 + 3][i]       );
            tmpsum |= unidigit[i][j];
         }
      }
      if (tmpsum == 0) {  /* the glyph matrix is transposed */
         flip = 1;  /* note transposed order for processing glyphs in matrix */
         /*
            Get 5th and 6th hex digits by shifting first column header left by
            1.5 columns, thereby shifting the hex digit right after the leading
            "U+nnnn" page number.
         */
         for (i = 0x08; i < 0x18; i++) {
            bitmap[i][7] = (bitmap[i][8] << 4) | ((bitmap[i][ 9] >> 4) & 0xf);
            bitmap[i][8] = (bitmap[i][9] << 4) | ((bitmap[i][10] >> 4) & 0xf);
         }
         for (i = 4; i < 6; i++) {
            for (j = 0; j < 4; j++) {
               unidigit[i][j] =
                  ((unsigned)bitmap[4 * j + 8 + 1][i + 3] << 24 ) |
                  ((unsigned)bitmap[4 * j + 8 + 2][i + 3] << 16 ) |
                  ((unsigned)bitmap[4 * j + 8 + 3][i + 3] <<  8 ) |
                  ((unsigned)bitmap[4 * j + 8 + 4][i + 3]       );
            }
         }
      }
   
      /*
         Now determine the Unicode plane by comparing unidigit[0..5] to
         the hexdigit[0x0..0xF] array.
      */
      uniplane = 0;
      for (i=0; i<6; i++) { /* go through one bitmap digit at a time */
         match = 0; /* haven't found pattern yet */
         for (j = 0x0; !match && j <= 0xF; j++) {
            if (unidigit[i][0] == hexdigit[j][0] &&
                unidigit[i][1] == hexdigit[j][1] &&
                unidigit[i][2] == hexdigit[j][2] &&
                unidigit[i][3] == hexdigit[j][3]) { /* we found the digit */
               uniplane |= j;
               match = 1;
            }
         }
         uniplane <<= 4;
      }
      uniplane >>= 4;
   }
   /*
      Now read each glyph and print it as hex.
   */
   for (i = 0x0; i <= 0xf; i++) {
      for (j = 0x0; j <= 0xf; j++) {
         for (k = 0; k < 16; k++) {
            if (flip) {  /* transpose glyph matrix */
               thischar0[k] = bitmap[32*(j+1) + k + 7][4 * (i+2)    ];
               thischar1[k] = bitmap[32*(j+1) + k + 7][4 * (i+2) + 1];
               thischar2[k] = bitmap[32*(j+1) + k + 7][4 * (i+2) + 2];
               thischar3[k] = bitmap[32*(j+1) + k + 7][4 * (i+2) + 3];
            }
            else {
               thischar0[k] = bitmap[32*(i+1) + k + 7][4 * (j+2)    ];
               thischar1[k] = bitmap[32*(i+1) + k + 7][4 * (j+2) + 1];
               thischar2[k] = bitmap[32*(i+1) + k + 7][4 * (j+2) + 2];
               thischar3[k] = bitmap[32*(i+1) + k + 7][4 * (j+2) + 3];
            }
         }
         /*
            If the second half of the 16*16 character is all zeroes, this
            character is only 8 bits wide, so print a half-width character.
         */
         empty1 = empty2 = 1;
         for (k=0; (empty1 || empty2) && k < 16; k++) {
            if (thischar1[k] != 0) empty1 = 0;
            if (thischar2[k] != 0) empty2 = 0;
            }
         /*
            Only print this glyph if it isn't blank.
         */
         if (!empty1 || !empty2) {
            /*
               If the second half is empty, this is a half-width character.
               Only print the first half.
            */
            /*
               Original GNU Unifont format is four hexadecimal digit character
               code followed by a colon followed by a hex string.  Add support
               for codes beyond the Basic Multilingual Plane.

               Unicode ranges from U+0000 to U+10FFFF, so print either a
               4-digit or a 6-digit code point.  Note that this software
               should support up to an 8-digit code point, extending beyond
               the normal Unicode range, but this has not been fully tested.
            */
            if (uniplane > 0xff)
               fprintf (outfp, "%04X%X%X:", uniplane, i, j); // 6 digit code pt.
            else
               fprintf (outfp, "%02X%X%X:", uniplane, i, j); // 4 digit code pt.
            for (thisrow=0; thisrow<16; thisrow++) {
               /*
                  If second half is empty and we're not forcing this
                  code point to double width, print as single width.
               */
               if (!forcewide &&
                   empty2 && !wide[(uniplane << 8) | (i << 4) | j]) {
                  fprintf (outfp,
                          "%02X",
                          thischar1[thisrow]);
               }
               else if (wide[(uniplane << 8) | (i << 4) | j] == 4) {
                  /* quadruple-width; force 32nd pixel to zero */
                  fprintf (outfp,
                          "%02X%02X%02X%02X",
                          thischar0[thisrow], thischar1[thisrow],
                          thischar2[thisrow], thischar3[thisrow] & 0xFE);
               }
               else { /* treat as double-width */
                  fprintf (outfp,
                          "%02X%02X",
                          thischar1[thisrow], thischar2[thisrow]);
               }
            }
            fprintf (outfp, "\n");
         }
      }
   }
   exit (0);
}
