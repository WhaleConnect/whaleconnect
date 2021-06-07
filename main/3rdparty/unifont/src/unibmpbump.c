/*
   unibmpbump.c - adjust a Microsoft bitmap (.bmp) file that
                  was created by unihex2png but converted to .bmp,
                  so it matches the format of a unihex2bmp image.
                  This conversion then lets unibmp2hex decode it.


   Synopsis: unibmpbump [-iin_file.bmp] [-oout_file.bmp]


   Author: Paul Hardy, unifoundry <at> unifoundry.com
   
   
   Copyright (C) 2019 Paul Hardy

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
#include <string.h>
#include <stdlib.h>

#define VERSION "1.0"

#define MAX_COMPRESSION_METHOD 13


int main (int argc, char *argv[]) {

   /*
      Values preserved from file header (first 14 bytes).
   */
   char file_format[3];       /* "BM" for original Windows format           */
   unsigned filesize;         /* size of file in bytes                      */
   unsigned char rsvd_hdr[4]; /* 4 reserved bytes                           */
   unsigned image_start;      /* byte offset of image in file               */

   /*
      Values preserved from Device Independent Bitmap (DIB) Header.

      The DIB fields below are in the standard 40-byte header.  Version
      4 and version 5 headers have more information, mainly for color
      information.  That is skipped over, because a valid glyph image
      is just monochrome.
   */
   int dib_length;            /* in bytes, for parsing by header version    */
   int image_width = 0;       /* Signed image width                         */
   int image_height = 0;      /* Signed image height                        */
   int num_planes = 0;        /* number of planes; must be 1                */
   int bits_per_pixel = 0;    /* for palletized color maps (< 2^16 colors)  */
   /*
      The following fields are not in the original spec, so initialize
      them to 0 so we can correctly parse an original file format.
   */
   int compression_method=0;  /* 0 --> uncompressed RGB/monochrome          */
   int image_size = 0;        /* 0 is a valid size if no compression        */
   int hres = 0;              /* image horizontal resolution                */
   int vres = 0;              /* image vertical resolution                  */
   int num_colors = 0;        /* Number of colors for pallettized images    */
   int important_colors = 0;  /* Number of significant colors (0 or 2)      */

   int true_colors = 0;       /* interpret num_colors, which can equal 0    */

   /*
      Color map.  This should be a monochrome file, so only two
      colors are stored.
   */
   unsigned char color_map[2][4]; /* two of R, G, B, and possibly alpha  */

   /*
      The monochrome image bitmap, stored as a vector 544 rows by
      72*8 columns.
   */
   unsigned image_bytes[544*72];

   /*
      Flags for conversion & I/O.
   */
   int verbose      = 0;      /* Whether to print file info on stderr     */
   unsigned image_xor = 0x00; /* Invert (= 0xFF) if color 0 is not black  */

   /*
      Temporary variables.
   */
   int i, j, k;           /* loop variables */

   /* Compression type, for parsing file */
   char *compression_type[MAX_COMPRESSION_METHOD + 1] = {
      "BI_RGB",            /*  0 */
      "BI_RLE8",           /*  1 */
      "BI_RLE4",           /*  2 */
      "BI_BITFIELDS",      /*  3 */
      "BI_JPEG",           /*  4 */
      "BI_PNG",            /*  5 */
      "BI_ALPHABITFIELDS", /*  6 */
      "", "", "", "",      /*  7 - 10 */
      "BI_CMYK",           /* 11 */
      "BI_CMYKRLE8",       /* 12 */
      "BI_CMYKRLE4",       /* 13 */
   };

   /* Standard unihex2bmp.c header for BMP image */
   unsigned standard_header [62] = {
      /*  0 */ 0x42, 0x4d, 0x3e, 0x99, 0x00, 0x00, 0x00, 0x00,
      /*  8 */ 0x00, 0x00, 0x3e, 0x00, 0x00, 0x00, 0x28, 0x00,
      /* 16 */ 0x00, 0x00, 0x40, 0x02, 0x00, 0x00, 0x20, 0x02,
      /* 24 */ 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00,
      /* 32 */ 0x00, 0x00, 0x00, 0x99, 0x00, 0x00, 0xc4, 0x0e,
      /* 40 */ 0x00, 0x00, 0xc4, 0x0e, 0x00, 0x00, 0x00, 0x00,
      /* 48 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      /* 56 */ 0x00, 0x00, 0xff, 0xff, 0xff, 0x00
   };

   unsigned get_bytes (FILE *, int);
   void     regrid    (unsigned *);

   char *infile="", *outfile="";  /* names of input and output files         */
   FILE *infp, *outfp;            /* file pointers of input and output files */

   /*
      Process command line arguments.
   */
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
               case 'v':  /* verbose output */
                  verbose = 1;
                  break;
               case 'V':  /* print version & quit */
                  fprintf (stderr, "unibmpbump version %s\n\n", VERSION);
                  exit (EXIT_SUCCESS);
                  break;
               case '-':  /* see if "--verbose" */
                  if (strcmp (argv[i], "--verbose") == 0) {
                     verbose = 1;
                  }
                  else if (strcmp (argv[i], "--version") == 0) {
                     fprintf (stderr, "unibmpbump version %s\n\n", VERSION);
                     exit (EXIT_SUCCESS);
                  }
                  break;
               default:   /* if unrecognized option, print list and exit */
                  fprintf (stderr, "\nSyntax:\n\n");
                  fprintf (stderr, "   unibmpbump ");
                  fprintf (stderr, "-i<Input_File> -o<Output_File>\n\n");
                  fprintf (stderr, "-v or --verbose gives verbose output");
                  fprintf (stderr, " on stderr\n\n");
                  fprintf (stderr, "-V or --version prints version");
                  fprintf (stderr, " on stderr and exits\n\n");
                  fprintf (stderr, "\nExample:\n\n");
                  fprintf (stderr, "   unibmpbump -iuni0101.bmp");
                  fprintf (stderr, " -onew-uni0101.bmp\n\n");
                  exit (EXIT_SUCCESS);
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
         exit (EXIT_FAILURE);
      }
   }
   else {
      infp = stdin;
   }
   if (strlen (outfile) > 0) {
      if ((outfp = fopen (outfile, "w")) == NULL) {
         fprintf (stderr, "Error: can't open %s for output.\n", outfile);
         exit (EXIT_FAILURE);
      }
   }
   else {
      outfp = stdout;
   }


   /* Read bitmap file header */
   file_format[0] = get_bytes (infp, 1);
   file_format[1] = get_bytes (infp, 1);
   file_format[2] = '\0';  /* Terminate string with null */

   /* Read file size */
   filesize = get_bytes (infp, 4);

   /* Read Reserved bytes */
   rsvd_hdr[0] = get_bytes (infp, 1);
   rsvd_hdr[1] = get_bytes (infp, 1);
   rsvd_hdr[2] = get_bytes (infp, 1);
   rsvd_hdr[3] = get_bytes (infp, 1);

   /* Read Image Offset Address within file */
   image_start = get_bytes (infp, 4);

   /*
      See if this looks like a valid image file based on
      the file header first two bytes.
   */
   if (strncmp (file_format, "BM", 2) != 0) {
      fprintf (stderr, "\nInvalid file format: not file type \"BM\".\n\n");
      exit (EXIT_FAILURE);
   }

   if (verbose) {
      fprintf (stderr, "\nFile Header:\n");
      fprintf (stderr, "   File Type:   \"%s\"\n", file_format);
      fprintf (stderr, "   File Size:   %d bytes\n", filesize);
      fprintf (stderr, "   Reserved:   ");
      for (i = 0; i < 4; i++) fprintf (stderr, " 0x%02X", rsvd_hdr[i]);
      fputc ('\n', stderr);
      fprintf (stderr, "   Image Start: %d. = 0x%02X = 0%05o\n\n",
               image_start, image_start, image_start);
   }  /* if (verbose) */

   /*
      Device Independent Bitmap (DIB) Header: bitmap information header
      ("BM" format file DIB Header is 12 bytes long).
   */
   dib_length = get_bytes (infp, 4);

   /*
      Parse one of three versions of Device Independent Bitmap (DIB) format:

           Length  Format
           ------  ------
              12   BITMAPCOREHEADER
              40   BITMAPINFOHEADER
             108   BITMAPV4HEADER
             124   BITMAPV5HEADER
   */
   if (dib_length == 12) { /* BITMAPCOREHEADER format -- UNTESTED */
      image_width    = get_bytes (infp, 2);
      image_height   = get_bytes (infp, 2);
      num_planes     = get_bytes (infp, 2);
      bits_per_pixel = get_bytes (infp, 2);
   }
   else if (dib_length >= 40) { /* BITMAPINFOHEADER format or later */
      image_width = get_bytes (infp, 4);
      image_height       = get_bytes (infp, 4);
      num_planes         = get_bytes (infp, 2);
      bits_per_pixel     = get_bytes (infp, 2);
      compression_method = get_bytes (infp, 4);  /* BI_BITFIELDS */
      image_size         = get_bytes (infp, 4);
      hres               = get_bytes (infp, 4);
      vres               = get_bytes (infp, 4);
      num_colors         = get_bytes (infp, 4);
      important_colors   = get_bytes (infp, 4);

      /* true_colors is true number of colors in image */
      if (num_colors == 0)
         true_colors = 1 << bits_per_pixel;
      else
         true_colors = num_colors;

      /*
         If dib_length > 40, the format is BITMAPV4HEADER or
         BITMAPV5HEADER.  As this program is only designed
         to handle a monochrome image, we can ignore the rest
         of the header but must read past the remaining bytes.
      */
      for (i = 40; i < dib_length; i++) (void)get_bytes (infp, 1);
   }

   if (verbose) {
      fprintf (stderr, "Device Independent Bitmap (DIB) Header:\n");
      fprintf (stderr, "   DIB Length:  %9d bytes (version = ", dib_length);

      if      (dib_length ==  12) fprintf (stderr, "\"BITMAPCOREHEADER\")\n");
      else if (dib_length ==  40) fprintf (stderr, "\"BITMAPINFOHEADER\")\n");
      else if (dib_length == 108) fprintf (stderr, "\"BITMAPV4HEADER\")\n");
      else if (dib_length == 124) fprintf (stderr, "\"BITMAPV5HEADER\")\n");
      else fprintf (stderr, "unknown)");
      fprintf (stderr, "   Bitmap Width:   %6d pixels\n", image_width);
      fprintf (stderr, "   Bitmap Height:  %6d pixels\n", image_height);
      fprintf (stderr, "   Color Planes:   %6d\n",        num_planes);
      fprintf (stderr, "   Bits per Pixel: %6d\n",        bits_per_pixel);
      fprintf (stderr, "   Compression Method: %2d --> ", compression_method);
      if (compression_method <= MAX_COMPRESSION_METHOD) {
         fprintf (stderr, "%s", compression_type [compression_method]);
      }
      /*
         Supported compression method values:
              0 --> uncompressed RGB
             11 --> uncompressed CMYK
      */
      if (compression_method == 0 || compression_method == 11) {
         fprintf (stderr, " (no compression)");
      }
      else {
         fprintf (stderr, "Image uses compression; this is unsupported.\n\n");
         exit (EXIT_FAILURE);
      }
      fprintf (stderr, "\n");
      fprintf (stderr, "   Image Size:            %5d bytes\n", image_size);
      fprintf (stderr, "   Horizontal Resolution: %5d pixels/meter\n", hres);
      fprintf (stderr, "   Vertical Resolution:   %5d pixels/meter\n", vres);
      fprintf (stderr, "   Number of Colors:      %5d", num_colors);
      if (num_colors != true_colors) {
         fprintf (stderr, " --> %d", true_colors);
      }
      fputc ('\n', stderr);
      fprintf (stderr, "   Important Colors:      %5d", important_colors);
      if (important_colors == 0)
         fprintf (stderr, " (all colors are important)");
      fprintf (stderr, "\n\n");
   }  /* if (verbose) */

   /*
      Print Color Table information for images with pallettized colors.
   */
   if (bits_per_pixel <= 8) {
      for (i = 0; i < 2; i++) {
         color_map [i][0] = get_bytes (infp, 1);
         color_map [i][1] = get_bytes (infp, 1);
         color_map [i][2] = get_bytes (infp, 1);
         color_map [i][3] = get_bytes (infp, 1);
      }
      /* Skip remaining color table entries if more than 2 */
      while (i < true_colors) {
         (void) get_bytes (infp, 4);
         i++;
      }

      if (color_map [0][0] >= 128) image_xor = 0xFF;  /* Invert colors */
   }

   if (verbose) {
      fprintf (stderr, "Color Palette [R, G, B, %s] Values:\n",
               (dib_length <= 40) ? "reserved" : "Alpha");
      for (i = 0; i < 2; i++) {
         fprintf (stderr, "%7d: [", i);
         fprintf (stderr, "%3d,",   color_map [i][0] & 0xFF);
         fprintf (stderr, "%3d,",   color_map [i][1] & 0xFF);
         fprintf (stderr, "%3d,",   color_map [i][2] & 0xFF);
         fprintf (stderr, "%3d]\n", color_map [i][3] & 0xFF);
      }
      if (image_xor == 0xFF) fprintf (stderr, "Will Invert Colors.\n");
      fputc ('\n', stderr);

   }  /* if (verbose) */


   /*
      Check format before writing output file.
   */
   if (image_width != 560 && image_width != 576) {
      fprintf (stderr, "\nUnsupported image width: %d\n", image_width);
      fprintf (stderr, "Width should be 560 or 576 pixels.\n\n");
      exit (EXIT_FAILURE);
   }

   if (image_height != 544) {
      fprintf (stderr, "\nUnsupported image height: %d\n", image_height);
      fprintf (stderr, "Height should be 544 pixels.\n\n");
      exit (EXIT_FAILURE);
   }

   if (num_planes != 1) {
      fprintf (stderr, "\nUnsupported number of planes: %d\n", num_planes);
      fprintf (stderr, "Number of planes should be 1.\n\n");
      exit (EXIT_FAILURE);
   }

   if (bits_per_pixel != 1) {
      fprintf (stderr, "\nUnsupported number of bits per pixel: %d\n",
               bits_per_pixel);
      fprintf (stderr, "Bits per pixel should be 1.\n\n");
      exit (EXIT_FAILURE);
   }

   if (compression_method != 0 && compression_method != 11) {
      fprintf (stderr, "\nUnsupported compression method: %d\n",
               compression_method);
      fprintf (stderr, "Compression method should be 1 or 11.\n\n");
      exit (EXIT_FAILURE);
   }

   if (true_colors != 2) {
      fprintf (stderr, "\nUnsupported number of colors: %d\n", true_colors);
      fprintf (stderr, "Number of colors should be 2.\n\n");
      exit (EXIT_FAILURE);
   }


   /*
      If we made it this far, things look okay, so write out
      the standard header for image conversion.
   */
   for (i = 0; i < 62; i++) fputc (standard_header[i], outfp);


   /*
      Image Data.  Each row must be a multiple of 4 bytes, with
      padding at the end of each row if necessary.
   */
   k = 0;  /* byte number within the binary image */
   for (i = 0; i < 544; i++) {
      /*
         If original image is 560 pixels wide (not 576), add
         2 white bytes at beginning of row.
      */
      if (image_width == 560) {  /* Insert 2 white bytes */
         image_bytes[k++] = 0xFF;
         image_bytes[k++] = 0xFF;
      }
      for (j = 0; j < 70; j++) {  /* Copy next 70 bytes */
         image_bytes[k++] = (get_bytes (infp, 1) & 0xFF) ^ image_xor;
      }
      /*
         If original image is 560 pixels wide (not 576), skip
         2 padding bytes at end of row in file because we inserted
         2 white bytes at the beginning of the row.
      */
      if (image_width == 560) {
         (void) get_bytes (infp, 2);
      }
      else {  /* otherwise, next 2 bytes are part of the image so copy them */
         image_bytes[k++] = (get_bytes (infp, 1) & 0xFF) ^ image_xor;
         image_bytes[k++] = (get_bytes (infp, 1) & 0xFF) ^ image_xor;
      }
   }


   /*
      Change the image to match the unihex2bmp.c format if original wasn't
   */
   if (image_width == 560) {
      regrid (image_bytes);
   }

   for (i = 0; i < 544 * 576 / 8; i++) {
      fputc (image_bytes[i], outfp);
   }


   /*
      Wrap up.
   */
   fclose (infp);
   fclose (outfp);

   exit (EXIT_SUCCESS);
}


/* Get from 1 to 4 bytes, inclusive. */
unsigned get_bytes (FILE *infp, int nbytes) {
   int i;
   unsigned char inchar[4];
   unsigned inword;

   for (i = 0; i < nbytes; i++) {
      if (fread (&inchar[i], 1, 1, infp) != 1) {
         inchar[i] = 0;
      }
   }
   for (i = nbytes; i < 4; i++) inchar[i] = 0;

   inword = ((inchar[3] & 0xFF) << 24) | ((inchar[2] & 0xFF) << 16) |
            ((inchar[1] & 0xFF) <<  8) |  (inchar[0] & 0xFF);

   return inword;
}


/*
   After reading in a PNG image, change to match unihex2bmp.c format.
*/
void regrid (unsigned *image_bytes) {
   int i, j, k;  /* loop variables */
   int offset;
   unsigned glyph_row; /* one grid row of 32 pixels */
   unsigned last_pixel; /* last pixel in a byte, to preserve */

   /* To insert "00" after "U+" at top of image */
   char zero_pattern[16] = {
       0x00, 0x00, 0x00, 0x00, 0x18, 0x24, 0x42, 0x42,
       0x42, 0x42, 0x42, 0x42, 0x24, 0x18, 0x00, 0x00
   };

   /* This is the horizontal grid pattern on glyph boundaries */
   unsigned hgrid[72] = {
      /*  0 */ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe,
      /*  8 */ 0x00, 0x81, 0x81, 0x00, 0x00, 0x81, 0x81, 0x00,
      /* 16 */ 0x00, 0x81, 0x81, 0x00, 0x00, 0x81, 0x81, 0x00,
      /* 24 */ 0x00, 0x81, 0x81, 0x00, 0x00, 0x81, 0x81, 0x00,
      /* 32 */ 0x00, 0x81, 0x81, 0x00, 0x00, 0x81, 0x81, 0x00,
      /* 40 */ 0x00, 0x81, 0x81, 0x00, 0x00, 0x81, 0x81, 0x00,
      /* 48 */ 0x00, 0x81, 0x81, 0x00, 0x00, 0x81, 0x81, 0x00,
      /* 56 */ 0x00, 0x81, 0x81, 0x00, 0x00, 0x81, 0x81, 0x00,
      /* 64 */ 0x00, 0x81, 0x81, 0x00, 0x00, 0x81, 0x81, 0x00
   };


   /*
      First move "U+" left and insert "00" after it.
   */
   j = 15; /* rows are written bottom to top, so we'll decrement j */
   for (i = 543 - 8; i > 544 - 24; i--) {
      offset = 72 * i;
      image_bytes [offset + 0] = image_bytes [offset + 2];
      image_bytes [offset + 1] = image_bytes [offset + 3];
      image_bytes [offset + 2] = image_bytes [offset + 4];
      image_bytes [offset + 3] = image_bytes [offset + 4] =
         ~zero_pattern[15 - j--] & 0xFF;
   }

   /*
      Now move glyph bitmaps to the right by 8 pixels.
   */
   for (i = 0; i < 16; i++) { /* for each glyph row */
      for (j = 0; j < 16; j++) { /* for each glyph column */
         /* set offset to lower left-hand byte of next glyph */
         offset = (32 * 72 * i) + (9 * 72) + (4 * j) + 8;
         for (k = 0; k < 16; k++) { /* for each glyph row */
            glyph_row = (image_bytes [offset + 0] << 24) |
                        (image_bytes [offset + 1] << 16) |
                        (image_bytes [offset + 2] <<  8) |
                        (image_bytes [offset + 3]);
            last_pixel = glyph_row & 1;  /* preserve border */
            glyph_row >>= 4;
            glyph_row &=  0x0FFFFFFE;
            /* Set left 4 pixels to white and preserve last pixel */
            glyph_row |=  0xF0000000 | last_pixel;
            image_bytes [offset + 3] = glyph_row & 0xFF;
            glyph_row >>= 8;
            image_bytes [offset + 2] = glyph_row & 0xFF;
            glyph_row >>= 8;
            image_bytes [offset + 1] = glyph_row & 0xFF;
            glyph_row >>= 8;
            image_bytes [offset + 0] = glyph_row & 0xFF;
            offset += 72;  /* move up to next row in current glyph */
         }
      }
   }

   /* Replace horizontal grid with unihex2bmp.c grid */
   for (i = 0; i <= 16; i++) {
      offset = 32 * 72 * i;
      for (j = 0; j < 72; j++) {
         image_bytes [offset + j] = hgrid [j];
      }
   }

   return;
}
