/*
   unipagecount - program to count the number of glyphs defined in each page
                  of 256 bytes, and print them in an 8 x 8 grid.  Input is
                  from stdin.  Output is to stdout.

   Synopsis: unipagecount < font_file.hex > count.txt
             unipagecount -phex_page_num < font_file.hex  -- just 256 points
             unipagecount -h < font_file.hex              -- HTML table
             unipagecount -P1 -h < font.hex > count.html  -- Plane 1, HTML out
             unipagecount -l < font_file.hex              -- linked HTML table

   Author: Paul Hardy, unifoundry <at> unifoundry.com, December 2007
   
   Copyright (C) 2007, 2008, 2013, 2014 Paul Hardy

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
   2018, Paul Hardy: Changed "Private Use" to "Private Use Area" in
   output HTML file.
*/

#include <stdio.h>
#include <stdlib.h>

#define MAXBUF 256 /* maximum input line size */

int
main (int argc, char *argv[])
{

   char inbuf[MAXBUF]; /* Max 256 characters in an input line */
   int i, j;  /* loop variables */
   unsigned plane=0; /* Unicode plane number, 0 to 0x16 */
   unsigned page;  /* unicode page (256 bytes wide) */
   unsigned unichar; /* unicode character */
   int pagecount[256] = {256 * 0};
   int onepage=0; /* set to one if printing character grid for one page */
   int pageno=0; /* page number selected if only examining one page */
   int html=0;   /* =0: print plain text; =1: print HTML */
   int links=0;  /* =1: print HTML links; =0: don't print links */
   void mkftable();  /* make (print) flipped HTML table */

   size_t strlen();

   if (argc > 1 && argv[1][0] == '-') {  /* Parse option */
      plane = 0;
      for (i = 1; i < argc; i++) {
         switch (argv[i][1]) {
            case 'p':  /* specified -p<hexpage> -- use given page number */
               sscanf (&argv[1][2], "%x", &pageno);
               if (pageno >= 0 && pageno <= 255) onepage = 1;
               break;
            case 'h':  /* print HTML table instead of text table */
               html = 1;
               break;
            case 'l':  /* print hyperlinks in HTML table */
               links = 1;
               html = 1;
               break;
            case 'P':  /* Plane number specified */
               plane = atoi(&argv[1][2]);
               break;
         }
      }
   }
   /*
      Initialize pagecount to account for noncharacters.
   */
   if (!onepage && plane==0) {
      pagecount[0xfd] = 32;  /* for U+FDD0..U+FDEF */
   }
   pagecount[0xff] = 2;   /* for U+nnFFFE, U+nnFFFF */
   /*
      Read one line at a time from input.  The format is:

         <hexpos>:<hexbitmap>

      where <hexpos> is the hexadecimal Unicode character position
      in the range 00..FF and <hexbitmap> is the sequence of hexadecimal
      digits of the character, laid out in a grid from left to right,
      top to bottom.  The character is assumed to be 16 rows of variable
      width.
   */
   while (fgets (inbuf, MAXBUF-1, stdin) != NULL) {
      sscanf (inbuf, "%X", &unichar);
      page = unichar >> 8;
      if (onepage) { /* only increment counter if this is page we want */
         if (page == pageno) { /* character is in the page we want */
            pagecount[unichar & 0xff]++; /* mark character as covered */
         }
      }
      else { /* counting all characters in all pages */
         if (plane == 0) {
            /* Don't add in noncharacters (U+FDD0..U+FDEF, U+FFFE, U+FFFF) */
            if (unichar < 0xfdd0 || (unichar > 0xfdef && unichar < 0xfffe))
               pagecount[page]++;
         }
         else {
            if ((page >> 8) == plane) { /* code point is in desired plane */
               pagecount[page & 0xFF]++;
            }
         }
      }
   }
   if (html) {
      mkftable (plane, pagecount, links);
   }
   else {  /* Otherwise, print plain text table */
      if (plane > 0) fprintf (stdout, "  ");
      fprintf (stdout,
         "   0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F\n");
      for (i=0; i<0x10; i++) {
         fprintf (stdout,"%02X%X ", plane, i); /* row header */
         for (j=0; j<0x10; j++) {
            if (onepage) {
               if (pagecount[i*16+j])
                  fprintf (stdout," *  ");
               else
                  fprintf (stdout," .  ");
            }
            else {
               fprintf (stdout, "%3X ", pagecount[i*16+j]);
            }
         }
         fprintf (stdout,"\n");
      }
   
   }
   exit (0);
}


/*
   mkftable - function to create an HTML table to show PNG files
              in a 16 by 16 grid.
*/

void
mkftable (unsigned plane, int pagecount[256], int links)
{
   int i, j;
   int count;
   unsigned bgcolor;
   
   printf ("<html>\n");
   printf ("<body>\n");
   printf ("<table border=\"3\" align=\"center\">\n");
   printf ("  <tr><th colspan=\"16\" bgcolor=\"#ffcc80\">");
   printf ("GNU Unifont Glyphs<br>with Page Coverage for Plane %d<br>(Green=100%%, Red=0%%)</th></tr>\n", plane);
   for (i = 0x0; i <= 0xF; i++) {
      printf ("  <tr>\n");
      for (j = 0x0; j <= 0xF; j++) {
         count = pagecount[ (i << 4) | j ];
         
         /* print link in cell if links == 1 */
         if (plane != 0 || (i < 0xd || (i == 0xd && j < 0x8) || (i == 0xf && j > 0x8))) {
            /* background color is light green if completely done */
            if (count == 0x100) bgcolor = 0xccffcc;
            /* otherwise background is a shade of yellow to orange to red */
            else bgcolor = 0xff0000 | (count << 8) | (count >> 1);
            printf ("    <td bgcolor=\"#%06X\">", bgcolor);
            if (plane == 0)
               printf ("<a href=\"png/plane%02X/uni%02X%X%X.png\">%X%X</a>", plane, plane, i, j, i, j);
            else
               printf ("<a href=\"png/plane%02X/uni%02X%X%X.png\">%02X%X%X</a>", plane, plane, i, j, plane, i, j);
            printf ("</td>\n");
         }
         else if (i == 0xd) {
            if (j == 0x8) {
               printf ("    <td align=\"center\" colspan=\"8\" bgcolor=\"#cccccc\">");
               printf ("<b>Surrogate Pairs</b>");
               printf ("</td>\n");
            }  /* otherwise don't print anything more columns in this row */
         }
         else if (i == 0xe) {
            if (j == 0x0) {
               printf ("    <td align=\"center\" colspan=\"16\" bgcolor=\"#cccccc\">");
               printf ("<b>Private Use Area</b>");
               printf ("</td>\n");
            }  /* otherwise don't print any more columns in this row */
         }
         else if (i == 0xf) {
            if (j == 0x0) {
               printf ("    <td align=\"center\" colspan=\"9\" bgcolor=\"#cccccc\">");
               printf ("<b>Private Use Area</b>");
               printf ("</td>\n");
            }
         }
      }
      printf ("  </tr>\n");
   }
   printf ("</table>\n");
   printf ("</body>\n");
   printf ("</html>\n");

   return;
}
