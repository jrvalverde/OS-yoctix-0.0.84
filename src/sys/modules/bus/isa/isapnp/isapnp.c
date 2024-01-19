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

/*
 *  modules/bus/isa/isapnp.c
 *
 *	TODO:
 *	  *  For each readport tried, ports_register() should be called
 *		(and then unregister() too, of course)
 *	  *  register addr/write ports BEFORE probing for cards!
 *
 *  History:
 *	31 Oct 2000	test, inspired by OpenBSD's /sys/dev/isa/isapnp.c
 *	10 Nov 2000	detects cards, but doesn't configure them yet
 *	14 Nov 2000	added functions to read resource data correctly
 */



#include "../../../../config.h"
#include <sys/defs.h>
#include <sys/std.h>
#include <stdio.h>
#include <string.h>
#include <sys/module.h>
#include <sys/ports.h>
#include <sys/malloc.h>
#include <sys/interrupts.h>
#include <sys/dma.h>
#include <sys/arch/i386/machdep.h>
#include <sys/arch/i386/pio.h>
#include <sys/modules/bus/isa/isa.h>
#include <sys/modules/bus/isa/isapnp.h>


#define	ISAPNP_STATE_UNUSED		0
#define	ISAPNP_STATE_DETECTED		1
#define	ISAPNP_STATE_CONFIGURED		2

struct isapnp_card
      {
	byte	state;
	byte	id [8];
	byte	csum;
	int	rdata_len;
	byte	*rdata;			/*  resource data  */
      };

struct isapnp_card isapnp_cards[ISAPNP_MAX_CARDS];


struct module	*isapnp_m;
int		isapnp_ncards = 0;
int		isapnp_readport = ISAPNP_RDDATA_MIN;


static inline void DELAY (int usec)
  {
    kusleep (usec);
  }



void isapnp_hw_init ()
  {
    /*
     *	isapnp_hw_init ()
     *	-----------------
     */

    int i;
    byte v = ISAPNP_LFSR_INIT;

    /*  First write 0's twice to enter the Wait for Key state  */
    outb (ISAPNP_ADDR, 0);
    outb (ISAPNP_ADDR, 0);

    /*  Send the 32 byte sequence to awake the logic  */
    for (i = 0; i < ISAPNP_LFSR_LENGTH; i++)
      {
	outb (ISAPNP_ADDR, v);
	v = ISAPNP_LFSR_NEXT (v);
      }
  }



byte isapnp_shift_bit ()
  {
    byte c1, c2;

    DELAY (250);
    c1 = inb (isapnp_readport);
    DELAY (250);
    c2 = inb (isapnp_readport);

    if (c1==0x55 && c2==0xaa)
      return 0x80;
    else
      return 0;
  }



void isapnp__id_to_ascii (byte *rawid, char *id)
  {
    /*
     *	Input:	rawid points to the 8 byte isapnp card id
     *	Output:	8 bytes are outputed to string at 'id'.
     */

    id[0] = ((rawid[0] >> 2) & 31)  + 64;
    id[1] = ((rawid[0] & 3) << 3) +
	    ((rawid[1] >> 5) & 7)   + 64;
    id[2] = (rawid[1] & 0x1f)       + 64;
    id[3] = "0123456789ABCDEF"[rawid[2] >> 4];
    id[4] = "0123456789ABCDEF"[rawid[2] & 15];
    id[5] = "0123456789ABCDEF"[rawid[3] >> 4];
    id[6] = "0123456789ABCDEF"[rawid[3] & 15];
    id[7] = '\0';
  }



int isapnp_waitforrdata ()
  {
    /*
     *	Wait for status bit = okay to read resource data.
     *	Returns 1 if okay, 0 on timeout.
     */

    register int t=10, i=0;

    outb (ISAPNP_ADDR, ISAPNP_STATUS);
    while (t>0)
      {
	i = inb (isapnp_readport) & 1;
	if (i)
	  return 1;

	kusleep (1000);
	t--;
      }

    return 0;
  }



int isapnp_readresourcebyte ()
  {
    /*
     *	Returns 0..255 for valid resource byte, or -1 on error.
     */

    if (!isapnp_waitforrdata ())
      {
	printk ("%s: error: time-out while waiting for resource data",
		isapnp_m->shortname);
	return -1;
      }

    outb (ISAPNP_ADDR, ISAPNP_RESOURCE_DATA);
    return inb (isapnp_readport);
  }



int isapnp_read_one_resource (byte *buf, int *typ)
  {
    /*
     *	Reads one resource into memory pointed to by buf. If buf is NULL,
     *	then no data is stored, only read.
     *
     *	Return value:  length (in bytes) of this resource
     *	or -1 on error.
     */

    int i, tot, len, d, d2, type;


    /*  Read first byte of resource data:  */

    d = isapnp_readresourcebyte ();
    if (d<0)
      return -1;
    if (buf)
	*buf = d, buf++;

    /*  Small resource or large resource?  */
    if (d & 0x80)
      {
	d2 = isapnp_readresourcebyte ();
	if (d2<0)
	  return -1;
	if (buf)
	  *buf = d2, buf++;
	len = d2;

	d2 = isapnp_readresourcebyte ();
	if (d2<0)
	  return -1;
	if (buf)
	  *buf = d2, buf++;
	len += (d2*256);

	tot = len + 3;
	type = d & 127;
	*typ = type + 128;
#if DEBUGLEVEL>=5
	printk ("  large type %y len=%i", type, len);
#endif
      }
    else
      {
	type = (d>>3) & 15;
	len = d & 7;
	tot = len + 1;
	*typ = type;
#if DEBUGLEVEL>=5
	printk ("  small type %y len=%i", type, len);
#endif
      }

    /*  Read 'len' bytes of resource data:  */
    for (i=0; i<len; i++)
      {
	d2 = isapnp_readresourcebyte ();
	if (d2<0)
	  return -1;
	if (buf)
	  *buf = d2, buf++;
      }

    return tot;
  }



void isapnp_read_resourcedata (int cardnr)
  {
    /*
     *	Read isapnp resource data from card 'cardnr' (0-based).
     *	Store result isapnp_cards[cardnr].rdata.
     *
     *	TODO:  it's probably not good to have a max size buffer like this.
     */

    int len = 0;
    int end = 0;
    int a;
    int type;
    byte *rdata;

    rdata = (byte *) malloc (8192);
    if (!rdata)
      return;

    end = 0;
    len = 0;
    while (!end)
      {
	a = isapnp_read_one_resource (rdata+len, &type);
	if (a<0)
	  return;

	if (type==0xf)
	  end=1;

	len += a;
      }

    isapnp_cards[cardnr].rdata = (byte *) malloc (len);
    if (!isapnp_cards[cardnr].rdata)
      {
	printk ("isapnp_read_resourcedata: out of memory");
	free (rdata);
	return;
      }

    memcpy (isapnp_cards[cardnr].rdata, rdata, len);
    isapnp_cards[cardnr].rdata_len = len;
    free (rdata);
  }



int isapnp_findcard ()
  {
    /*
     *	isapnp_findcard ()
     *	------------------
     *
     *	Returns 1 if a card was found, 0 otherwise.
     */

    byte v = ISAPNP_LFSR_INIT;
    byte csum, w;
    int i, b;

    if (isapnp_ncards>=ISAPNP_MAX_CARDS)
      {
	printk ("%s: too many isapnp cards", isapnp_m->shortname);
	return 0;
      }

    outb (ISAPNP_ADDR, ISAPNP_WAKE);
    outb (ISAPNP_WRDATA, 0);

    outb (ISAPNP_ADDR, ISAPNP_SET_RD_PORT);
    outb (ISAPNP_WRDATA, isapnp_readport >> 2);
    DELAY (1000);

    outb (ISAPNP_ADDR, ISAPNP_SERIAL_ISOLATION);
    DELAY (1000);

    /*  Read the 8 bytes of the Vendor ID and Serial Number  */
    for(i = 0; i < 8; i++)
      {
	/*  Read each bit separately  */
	for (w = 0, b = 0; b < 8; b++)
	  {
	    byte neg = isapnp_shift_bit ();
	    w >>= 1;
	    w |= neg;
	    v = ISAPNP_LFSR_NEXT(v) ^ neg;
	  }

	isapnp_cards[isapnp_ncards].id[i] = w;
      }

    /*  Read the remaining checksum byte  */
    for (csum = 0, b = 0; b < 8; b++)
      {
	byte neg = isapnp_shift_bit ();
	csum >>= 1;
	csum |= neg;
      }

     isapnp_cards[isapnp_ncards].csum = csum;

     if (csum == v)
       {
	isapnp_cards[isapnp_ncards].state = ISAPNP_STATE_DETECTED;

#if DEBUGLEVEL>=4
	{
	  isapnp__id_to_ascii (isapnp_cards[isapnp_ncards].id, id);
	  printk ("  detected isapnp card %i: "%s" serial 0x%x",
		isapnp_ncards+1, id,
		*((u_int32_t *)&isapnp_cards[isapnp_ncards].id[4]) );
	}
#endif

	outb (ISAPNP_ADDR, ISAPNP_CARD_SELECT_NUM);
	outb (ISAPNP_WRDATA, isapnp_ncards);
	isapnp_read_resourcedata (isapnp_ncards);
	isapnp_ncards ++;

	return 1;
       }

     return 0;
  }



int isapnp_findall ()
  {
    /*
     *	isapnp_findall ()
     *	-----------------
     *
     *	Return the number of found isapnp cards.
     */

    int found;
    int port;

    isapnp_hw_init ();
    outb (ISAPNP_ADDR, ISAPNP_CONFIG_CONTROL);
    outb (ISAPNP_WRDATA, ISAPNP_CC_RESET_DRV);
    DELAY (2000);
    isapnp_hw_init ();
    DELAY (2000);

    port = ISAPNP_RDDATA_MIN;
    found = 0;
    isapnp_ncards = 0;

    /*
     *	TODO:  Note:  this is very slow... scanning all ports up to
     *	ISAPNP_RDDATA_MAX is the correct thing to do, but if there are no
     *	isapnp cards in the machine, then it will take several (tens of)
     *	seconds. It's probably because the delays are too long...  Temporary
     *	speed fix: only try a few ports...
     */

    while (!found && port<=ISAPNP_RDDATA_MIN+32)
      {
	isapnp_readport = port;

	found = isapnp_findcard ();
	if (!found)
	  port += 4;
      }

    if (!found)
      {
	isapnp_readport = 0;
	return 0;
      }

    while (isapnp_findcard ())   ;

    return isapnp_ncards;
  }



void isapnp__cardidstring (int cardnr, int buflen, char *buf)
  {
    int rtype, ofs=0, len, found=0;

    if (!buf || buflen<2)
      return;

    buf[0] = '\0';

    if (!isapnp_cards[cardnr].rdata)
      return;

    while (!found && ofs<isapnp_cards[cardnr].rdata_len)
      {
	rtype = isapnp_cards[cardnr].rdata[ofs++];
	if (rtype & 0x80)
	  {
	    len = (isapnp_cards[cardnr].rdata[ofs] +
		 isapnp_cards[cardnr].rdata[ofs+1] * 256);
	    ofs += 2;
	    if (rtype==128+2)
	      {
		while (buflen>1 && len>0)
		  {
		    *buf = isapnp_cards[cardnr].rdata[ofs++];
		    buf++, buflen--, len--;
		  }
		*buf = '\0';
		return;
	      }
	    /*  skip large resource:  */
	    ofs += len;
	  }
	else
	  ofs += (rtype & 7);
      }
  }



void isapnp_config_cards ()
  {
    int i;
    char id [8];
    char idstring [80];

    for (i=0; i<ISAPNP_MAX_CARDS; i++)
      if (isapnp_cards[i].state == ISAPNP_STATE_DETECTED)
	{
	  isapnp__id_to_ascii (isapnp_cards[i].id, id);
	  isapnp__cardidstring (i, 80, idstring);

	  printk ("%s card %i %s,0x%x \"%s\": not configured",
		isapnp_m->shortname,
		i+1, id, *((u_int32_t *)&isapnp_cards[i].id[4]),
		idstring);
	}
  }



void isapnp_init (int arg)
  {
    char buf[80];
    int res1, res2, res3;

    isapnp_m = module_register ("isa0", MODULETYPE_NUMBERED,
		"isapnp", "ISA Plug-and-Play");

    if (!isapnp_m)
	return;

    memset (isapnp_cards, 0, sizeof(isapnp_cards));

    isapnp_findall ();

    if (isapnp_ncards == 0 || isapnp_readport == 0)
      {
	module_unregister (isapnp_m);
	return;
      }

    res1 = ports_register (isapnp_readport, 1, isapnp_m->shortname);
    res2 = ports_register (ISAPNP_ADDR, 1, isapnp_m->shortname);
    res3 = ports_register (ISAPNP_WRDATA, 1, isapnp_m->shortname);
    if (res1+res2+res3 != 3)
      {
	if (!res1)
	  printk ("%s: could not register port 0x%Y", isapnp_m->shortname, isapnp_readport);
	if (!res2)
	  printk ("%s: could not register port 0x%Y", isapnp_m->shortname, ISAPNP_ADDR);
	if (!res3)
	  printk ("%s: could not register port 0x%Y", isapnp_m->shortname, ISAPNP_WRDATA);
	ports_unregister (isapnp_readport);
	ports_unregister (ISAPNP_ADDR);
	ports_unregister (ISAPNP_WRDATA);
	module_unregister (isapnp_m);
	return;
      }

    isa_module_nametobuf (isapnp_m, buf, 80);
    printk ("%s", buf);

    isapnp_config_cards ();
  }


