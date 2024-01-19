/*
 * MEGATRIVIAL kernel-burner
 *
 * Writes biosboot + yoktix to fd1c
 */

#include <stdio.h>
main()
{
FILE *fout, *fin;
int len;
unsigned char buf[500000];

printf ("opening /dev/fd1c\n");
fout = fopen ("/dev/fd1c", "w");
if (!fout)
  { perror("fd1c"); exit (1); }

printf ("opening biosboot\n");
fin = fopen ("biosboot", "r");
if (!fin)
  { perror("biosboot"); exit (1); }

fseek (fin, 32, SEEK_SET);

fread (buf, 1, 512, fin);
fwrite (buf, 1, 512, fout);

fclose (fin);

printf ("opening yoctix\n");
fin = fopen ("yoctix", "r");
if (!fin)
  { perror("yoctix"); exit (1); }

len = fread (buf, 1, 500000, fin);
fseek (fout, 18*2*512*74, 0);
fwrite (buf, 1, len, fout);
fclose (fin);

fclose (fout);
}
