/*
 *  Copyright (C) 2000 by Anders Gavare.  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 *  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 *  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 *  OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 *  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 *  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 *  SUCH DAMAGE.
 */

#include <stdio.h>
#include <string.h>


FILE *floppy;
FILE *yoctix_image;
FILE *biosboot;

#define	MAXBR	30

struct blockread
      {
	unsigned char sec, cyl, head, nrsect;
      };

unsigned char bootsectorbuf [512];
unsigned char markstr [8];
unsigned char msdosdirentry [32];
unsigned char fatbuf [0x1200];
unsigned char imagebuf [512];
unsigned char imagebuf2 [512];
int i, markerpos, found, len, yoctixlen;
int nr_of_entries;
int cluster, newcluster;
int raw_address;
int abssector, cyl, head, sector;

struct blockread blockread [MAXBR];
int curblockread = -1;


int fdc_whichsector (int abssector, int *cyl, int *head, int *sector)
  {
    /*  Convert absolute sector number to cylinder, head, and sector (1-based)     */
    /*  abssector = (sector-1) + (nrheads * secpertracj) * cyl + secpertrack * head    */

    *sector = abssector % 18;
    (*sector) ++;
    abssector /= 18;
    *head = abssector % 2;
    *cyl = abssector / 2;

    if (*cyl >= 80)
        return 0;
    return 1;
  }



int main (int argc, char *argv[])
  {
    if (argc == 1)
      {
	printf ("usage: %s floppydev [yoctix_image]\n", argv[0]);
	printf ("where floppydev is for example /dev/fd0c (OpenBSD) or /dev/fd0b (NetBSD).\n"
		"If no yoctix_image is specified, only the bootsector is updated with contents\n"
		"from the file 'biosboot' which must be located in the directory from which\n"
		"%s is executed.\n", argv[0]);
	printf ("If yoctix_image is specified, the kernel image on the floppy is \"patched\"\n"
		"with data from yoctix_image.\n");
	exit (1);
      }

    floppy = fopen (argv[1], "r+");
    if (!floppy)
      {
	perror (argv[1]);
	exit (1);
      }

    if (argc > 2)
      {
	yoctix_image = fopen (argv[2], "r");
	if (!yoctix_image)
	  {
	    perror (argv[2]);
	    exit (1);
	  }
      }
    else
      yoctix_image = NULL;

    biosboot = fopen ("biosboot", "r");
    if (!biosboot)
      {
	perror ("biosboot");
	exit (1);
      }


    /*
     *	Read the biosboot sector into memory:
     */

    fseek (biosboot, 32, SEEK_SET);
    fread (bootsectorbuf, 1, 512, biosboot);

    /*  Find 1,2,3,4,5,6,7,8 mark:  */
    markstr[0]=1; markstr[1]=2; markstr[2]=3; markstr[3]=4;
    markstr[4]=5; markstr[5]=6; markstr[6]=7; markstr[7]=8;

    markerpos = -1;
    for (i=0; i<512-8; i++)
      if (memcmp(bootsectorbuf+i, markstr, 8) == 0)
	markerpos = i;

    if (markerpos<0)
      {
	fprintf (stderr, "%s: could not find marker in biosboot\n",
		argv[0]);
	exit (1);
      }


    /*
     */

    cluster = msdosdirentry[0x1a] + 256*msdosdirentry[0x1b];
    printf ("Starting cluster number = %i\n", cluster);

    raw_address = 0x0;

    /*  Traverse the cluster chain:  */
    while (!feof(yoctix_image))
      {
	/*  Patch using yoctix_image?  */
	if (yoctix_image)
	  {
	    printf ("ofs =%7i: ", (int)ftell(yoctix_image));
	    yoctixlen = fread (imagebuf, 1, 512, yoctix_image);

	    found = 0;
	    while (!found)
	      {
		fseek (floppy, raw_address, SEEK_SET);
		len = fread (imagebuf2, 1, 512, floppy);

		if (len!=512)
		  {
		    printf ("error reading from floppy\n");
		    exit (1);
		  }

		if (!memcmp(imagebuf,imagebuf2,yoctixlen))
		  found = 1;

		if (!found)
		  raw_address += 512;

		if (raw_address >= 1440*1024)
		  {
		    printf ("(wrap around)\n");
		    raw_address = 0x0;
		  }
	      }

/*	    fwrite (imagebuf, 1, 512, floppy);
*/
	  }

	abssector = raw_address / 512;
	fdc_whichsector (abssector, &cyl, &head, &sector);

	printf ("cluster %i => cyl %i, head %i, sector %i\n", cluster, cyl, head, sector);

	if (curblockread == -1)
	  {
	    curblockread++;
	    blockread[curblockread].sec = sector;
	    blockread[curblockread].head = head;
	    blockread[curblockread].cyl = cyl;
	    blockread[curblockread].nrsect = 1;
	  }
	else
	  {
	    /*  Is this sector following the last one?  */
	    if (sector == blockread[curblockread].sec + blockread[curblockread].nrsect
		&& head == blockread[curblockread].head &&
		cyl == blockread[curblockread].cyl)
	      {
		blockread[curblockread].nrsect++;
	      }
	    else
	      {
		curblockread++;
		blockread[curblockread].sec = sector;
		blockread[curblockread].head = head;
		blockread[curblockread].cyl = cyl;
		blockread[curblockread].nrsect = 1;
	      }
	  }

	/*  Get next cluster:  */
/*	newcluster = (cluster * 3) / 2;
	newcluster = fatbuf[newcluster] + 256*fatbuf[newcluster+1];
	if (cluster & 1)
	  newcluster = (newcluster >> 4) & 0xfff;
	else
	  newcluster &= 0xfff;
	cluster = newcluster;
*/

	raw_address += 512;
      }


    for (i=0; i<=curblockread; i++)
      {
	printf ("block %i: chs=%i,%i,%i nr=%i\n",
		i,
		blockread[i].cyl,
		blockread[i].head,
		blockread[i].sec,
		blockread[i].nrsect);

	memcpy (bootsectorbuf+markerpos+4*i, &blockread[i], 4);
      }

    bootsectorbuf[markerpos-1] = curblockread+1;


    /*  Write new bootsector:  */
    fseek (floppy, 0, SEEK_SET);
    fwrite (bootsectorbuf, 1, 512, floppy);

    fclose (floppy);
  }

