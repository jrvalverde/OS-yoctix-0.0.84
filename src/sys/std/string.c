/*
 *  Copyright (C) 1999 by Anders Gavare.  All rights reserved.
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

/*
 *  string.c  --  string functions
 *
 *  TODO:
 *	Move to arch/ instead (?), or at least optimize...
 *
 *  History:
 *	18 Oct 1999	first version: memset(), memcpy()
 *	21 Oct 1999	adding strlen()
 *	20 Dec 1999	adding strncpy(), strlcpy(), strcmp(), strncmp()
 */


#include <string.h>
#include <sys/std.h>
#include <sys/defs.h>


void *memset (void *b, int c, size_t len)
  {
    unsigned char *p;
    size_t i;

    p = (unsigned char *) b;
    for (i=0; i<len; i++)
	p[i] = c;
    return b;
  }



void *memcpy (void *dst, void *src, size_t len)
  {
    unsigned char *a = (unsigned char *) dst;
    unsigned char *b = (unsigned char *) src;
    size_t i;

    if (len==0)
	return dst;

    if (a < b)
      {
	for (i=0; i<len; i++)
		a[i] = b[i];
      }
    else
      {
	for (i=len-1; i>0; i--)
		a[i] = b[i];
	a[0]=b[0];
      }

    return dst;
  }



size_t strlen (char *s)
  {
    size_t count = 0;
    while (s[0])
	s++, count++;
    return count;
  }



char *strncpy (char *dst, char *src, size_t len)
  {
    int i=0;

    if (!dst || !src)
      {
	printk ("strncpy: NULL ptr");
	return NULL;
      }

    while (src[i] && i<len)
	dst[i]=src[i], i++;

    if (i<len && src[i]==0)
	dst[i]=0;

    return dst;
  }



size_t strlcpy (char *dst, char *src, size_t len)
  {
    int i = 0;

    if (!dst || !src)
      {
	printk ("strlcpy: NULL ptr");
	return 0;
      }

    while (src[i] && i < len)
	dst[i]=src[i], i++;

    if (i==len)
	i--;
    dst[i]=0;

    return strlen(src);
  }



int strcmp (unsigned char *s1, unsigned char *s2)
  {
    int i = 0;

    if (!s1 || !s2)
      {
	printk ("strcmp: NULL ptr");
	return 1;
      }

    while (!(s1[i]==0 && s2[i]==0))
      {
	if (s1[i] < s2[i])	return -1;
	if (s1[i] > s2[i])	return 1;
	i++;
      }

    return 0;
  }



int strncmp (unsigned char *s1, unsigned char *s2, int len)
  {
    int i = 0;

    if (!s1 || !s2)
      {
	printk ("strncmp: NULL ptr");
	return 1;
      }

    if (len < 0)
      {
	printk ("strncmp: len = %i, invalid", len);
	return 1;
      }

    while (!(s1[i]==0 && s2[i]==0) && i<len)
      {
	if (s1[i] < s2[i])	return -1;
	if (s1[i] > s2[i])	return 1;
	i++;
      }

    return 0;
  }


