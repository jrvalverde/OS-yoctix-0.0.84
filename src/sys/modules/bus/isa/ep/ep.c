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
 *  modules/bus/isa/ep/ep.c  --  3C509B (and possibly others...)
 *
 *	Written with help from various other drivers (FreeBSD, OpenBSD,
 *	Linux).
 *
 *  History:
 *	29 Nov 2000	test
 *	1 Dec 2000	more testing
 */



#include "../../../../config.h"
#include <string.h>
#include <stdio.h>
#include <sys/std.h>
#include <sys/module.h>
#include <sys/ports.h>
#include <sys/proc.h>
#include <sys/interrupts.h>
#include <sys/arch/i386/pio.h>
#include <sys/modules/bus/isa/isa.h>
#include <sys/modules/bus/isa/elink3reg.h>
#include <sys/modules/bus/isa/elink3var.h>
#include <sys/modules/net/if_arp.h>


#define	MAX_CARDS	20

struct ep_card
      {
	int	detected;
	byte	ethaddr [6];
	int	iobase;
	int	irq;
	int	connector;		/*  10=UTP, 5=AUI, 2=BNC  */
	int	id;
	int	prod_id;
	struct module *m;
      };

struct ep_card ep_card [MAX_CARDS];

int ep_total_nr_of_cards = 0;



int ep_poll();


void ep_irqhandler ()
  {
    u_int16_t status;
    int iobase;

    printk ("ep_irqhandler()");

    iobase = ep_card[0].iobase;

    while (1)
      {
printk ("A");

	outw (iobase + EP_COMMAND, C_INTR_LATCH);
	status = inw (iobase + EP_STATUS);

	if ((status & (S_TX_COMPLETE | S_TX_AVAIL |
	    S_RX_COMPLETE | S_CARD_FAILURE)) == 0)
	  break;

printk ("B");

	outw (iobase + EP_COMMAND, ACK_INTR | status);

	if (status & S_RX_COMPLETE)
	  ep_poll (0);

	/*  TODO:  S_TX_AVAIL, S_CARD_FAILURE, S_TX_COMPLETE  */
      }

    wakeup (ep_irqhandler);
  }



void ep__send_id_seq (int p)
  {
    int i, d = 255;

    for (i=0; i<255; i++)
      {
	outb (p, d);
	d <<= 1;
	if (d & 256)
	  d ^= 0xcf;
      }
  }



int ep__eeprom_read (int p, int ofs)
  {
    int i, d=0;

    outb (p, 0x80 + ofs);
    kusleep (1000);
    for (i=0; i<16; i++)
	d = (d << 1) | (inw (p) & 1);
    return d;
  }



int ep__eeprom_ready (int base)
  {
    int i;

    for (i=0; i<5000; i++)
      {
	if ((inw ((base) + EP_W0_EEPROM_COMMAND) & EEPROM_BUSY) == 0)
	  return 1;
      }

    printk ("ep: eeprom not ready base=0x%Y", base);
    return 0;
  }



int ep__get_eeprom_data (int base, int ofs)
  {
    /*
     *	Get a 16 bit word from the EEPROM.
     */

    if (!ep__eeprom_ready (base))
      return -1;

    outw (base + EP_W0_EEPROM_COMMAND, READ_EEPROM | ofs);

    if (!ep__eeprom_ready (base))
      return -1;

    return inw (base + EP_W0_EEPROM_DATA);
  }



void ep_reset_card (int cardnr)
  {
    int i;
    int iobase = ep_card[cardnr].iobase;

    outw (iobase + EP_COMMAND, RX_DISABLE);
    outw (iobase + EP_COMMAND, RX_DISCARD_TOP_PACK);
    while (inw (iobase + EP_STATUS) & S_COMMAND_IN_PROGRESS)  ;
    outw (iobase + EP_COMMAND, TX_DISABLE);
    outw (iobase + EP_COMMAND, STOP_TRANSCEIVER);
    outw (iobase + EP_COMMAND, RX_RESET);
    outw (iobase + EP_COMMAND, TX_RESET);
    outw (iobase + EP_COMMAND, C_INTR_LATCH);
    outw (iobase + EP_COMMAND, SET_RD_0_MASK);
    outw (iobase + EP_COMMAND, SET_INTR_MASK);
    outw (iobase + EP_COMMAND, SET_RX_FILTER);

    while (inw (iobase + EP_STATUS) & S_COMMAND_IN_PROGRESS)  ;
    GO_WINDOW(iobase,0);

    /* Disable the card */
    outw (iobase + EP_W0_CONFIG_CTRL, 0);

    /* Configure IRQ to none */
    outw (iobase + EP_W0_RESOURCE_CFG, SET_IRQ(0));
/*
    outw (iobase + EP_W0_RESOURCE_CFG, SET_IRQ(10));
*/

    /* Enable the card */
    outw (iobase + EP_W0_CONFIG_CTRL, ENABLE_DRQ_IRQ);

    GO_WINDOW(iobase,2);

    /* Reload the ether_addr. */
    for (i=0; i<6; i++)
	outb (iobase + EP_W2_ADDR_0 + i,
		ep_card[cardnr].ethaddr[i]);

    outw (iobase + EP_COMMAND, RX_RESET);
    outw (iobase + EP_COMMAND, TX_RESET);

    /* Window 1 is operating window */
    GO_WINDOW(iobase,1);
    for (i=0; i<31; i++)
	inb (iobase + EP_W1_TX_STATUS);

    outw (iobase + EP_COMMAND, SET_RD_0_MASK | S_5_INTS);
    outw (iobase + EP_COMMAND, SET_INTR_MASK | S_5_INTS);
    outw (iobase + EP_COMMAND, SET_RX_FILTER | FIL_INDIVIDUAL |
	FIL_BRDCST);

    /* get rid of stray intr's */
    outw (iobase + EP_COMMAND, ACK_INTR | 0xff);

    /*  Configure depending on connector type:  */
    switch (ep_card[cardnr].connector)
      {
	/*  BNC:  */
	case 2:
		outw (iobase + EP_COMMAND, START_TRANSCEIVER);
		kusleep (1000);
		break;
	/*  UTP:  */
	case 10:
		GO_WINDOW(iobase,4);
		outw (iobase + EP_W4_MEDIA_TYPE, ENABLE_UTP);
		GO_WINDOW(iobase,1);
	default:
		/*  No config needed???  */
      }

    /*  Start tranciever and receiver:  */
    outw (iobase + EP_COMMAND, RX_ENABLE);
/*
    outw (iobase + EP_COMMAND, TX_ENABLE);
*/

    /*  Set early threshold for minimal packet length  */
    outw (iobase + EP_COMMAND, SET_RX_EARLY_THRESH | 64);
    outw (iobase + EP_COMMAND, SET_TX_START_THRESH | 16);
  }



int ep_poll (int cardnr)
  {
    int packetlen;
    byte packet [2000];
    unsigned short type = 0;
/*    struct ether_header *eh; */
    short status;
    register short rx_fifo;
    int iobase;

    iobase = ep_card[cardnr].iobase;

/*
    cst = inw (iobase + EP_STATUS);
    if ((cst & 0x1fff) == 0)
      return 0;

    if ((cst & (S_RX_COMPLETE|S_RX_EARLY)) == 0)
      {
	* acknowledge  everything *
	outw (iobase + EP_COMMAND, ACK_INTR| (cst & S_5_INTS));
	outw (iobase + EP_COMMAND, C_INTR_LATCH);

	return 0;
      }
*/

    status = inw (iobase + EP_W1_RX_STATUS);
    if (status & ERR_RX)
      {
	outw (iobase + EP_COMMAND, RX_DISCARD_TOP_PACK);
	return 0;
      }

    rx_fifo = status & RX_BYTES_MASK;
    if (rx_fifo==0)
      return 0;

    /*  Read packet:  */
    insw (iobase + EP_W1_RX_PIO_RD_1, packet, rx_fifo / 2);
    if (rx_fifo & 1)
      packet[rx_fifo-1] = inb (iobase + EP_W1_RX_PIO_RD_1);

    packetlen = rx_fifo;

    while (1)
      {
	status = inw (iobase + EP_W1_RX_STATUS);
	rx_fifo = status & RX_BYTES_MASK;

	if (rx_fifo > 0)
	  {
	    insw (iobase + EP_W1_RX_PIO_RD_1, packet+packetlen, rx_fifo / 2);

	    if (rx_fifo & 1)
		packet[packetlen+rx_fifo-1] = inb (iobase + EP_W1_RX_PIO_RD_1);
	    packetlen += rx_fifo;
/*
	    printk ("  rx_fifo = 0x%Y", rx_fifo);
*/
	  }

	if ((status & ERR_INCOMPLETE) == 0)
	    break;

	kusleep (1000);
      }

    /* acknowledge reception of packet */
    outw (iobase + EP_COMMAND, RX_DISCARD_TOP_PACK);
    while (inw (iobase + EP_STATUS) & S_COMMAND_IN_PROGRESS);

/*
printk ("%s: %y:%y:%y:%y:%y:%y %y:%y:%y:%y:%y:%y %y%y %y%y %y%y%y%y %y%y%y%y %y%y%y%y",
	ep_card[cardnr].m->shortname,
	packet[0], packet[1], packet[2], packet[3],
	packet[4], packet[5], packet[6], packet[7],
	packet[8], packet[9], packet[10], packet[11],
	packet[12], packet[13], packet[14], packet[15],
	packet[17], packet[18], packet[19], packet[20],
	packet[21], packet[22], packet[23], packet[24],
	packet[25], packet[26], packet[27], packet[28]);
*/

    type = (packet[12]<<8) | packet[13];

    /*  Type 0800 = IP, 0806 = ARP  */

    if (type == 0x0806)
      {
	struct arphdr *ah;

	ah = (struct arphdr *) ((byte *)packet + 14);

/*  These need to be htons():ed!  */
/*
	printk ("  ARP:  ar_hrd = 0x%Y", ah->ar_hrd);
	printk ("        ar_pro = 0x%Y", ah->ar_pro);
	printk ("        ar_hln =   0x%y", ah->ar_hln);
	printk ("        ar_pln =   0x%y", ah->ar_pln);
*/
	printk ("        ar_op  = 0x%Y   addr = %i.%i.%i.%i -> %i.%i.%i.%i",
		ah->ar_op,
		packet[28], packet[29], packet[30], packet[31],
		packet[38], packet[39], packet[40], packet[41]);
      }


/*
        if (type == ARP) {
                struct arprequest *arpreq;
                unsigned long reqip;
                arpreq = (struct arprequest *)&packet[ETHER_HDR_LEN];
#ifdef EDEBUG
                printf("(ARP %I->%I)",ntohl(*(int*)arpreq->sipaddr),
                    ntohl(*(int*)arpreq->tipaddr));
#endif
                convert_ipaddr(&reqip, arpreq->tipaddr);
                if ((ntohs(arpreq->opcode) == ARP_REQUEST) &&
                    (reqip == arptable[ARP_CLIENT].ipaddr)) {
                        arpreq->opcode = htons(ARP_REPLY);
                        bcopy(arpreq->sipaddr, arpreq->tipaddr, 4);
                        bcopy(arpreq->shwaddr, arpreq->thwaddr, 6);
                        bcopy(arptable[ARP_CLIENT].node, arpreq->shwaddr, 6);
                        convert_ipaddr(arpreq->sipaddr, &reqip);
                        eth_transmit(arpreq->thwaddr, ARP, sizeof(struct arprequest),
                            arpreq);
                        return(0);
                }
        } else if(type==IP) {
                struct iphdr *iph;
                iph = (struct iphdr *)&packet[ETHER_HDR_LEN];
#ifdef EDEBUG
                printf("(IP %I-%d->%I)",ntohl(*(int*)iph->src),
                    ntohs(iph->protocol),ntohl(*(int*)iph->dest));
#endif
        }
*/
    return 1;
  }



int ep_probe ()
  {
    /*
     *	Probe for 3c509 cards:
     */

    int id, d, d2, cn, iobase;

    ep_total_nr_of_cards = 0;
    outb (ELINK_ID_PORT, 0xc0);

    kusleep (1000);

    for (cn=0; cn<MAX_CARDS; cn++)
      {
	outb (ELINK_ID_PORT, 0);
	outb (ELINK_ID_PORT, 0);
	ep__send_id_seq (ELINK_ID_PORT);

	id = ep__eeprom_read (ELINK_ID_PORT, EEPROM_MFG_ID);
	id = ((id & 255) << 8) + ((id >> 8) & 255);

	if (id != MFG_ID)
	  return ep_total_nr_of_cards;

	ep_card[ep_total_nr_of_cards].id = id;

	d = ep__eeprom_read (ELINK_ID_PORT, 0);
	ep_card[ep_total_nr_of_cards].ethaddr[0] = d >> 8;
	ep_card[ep_total_nr_of_cards].ethaddr[1] = d & 255;
	d = ep__eeprom_read (ELINK_ID_PORT, 1);
	ep_card[ep_total_nr_of_cards].ethaddr[2] = d >> 8;
	ep_card[ep_total_nr_of_cards].ethaddr[3] = d & 255;
	d = ep__eeprom_read (ELINK_ID_PORT, 2);
	ep_card[ep_total_nr_of_cards].ethaddr[4] = d >> 8;
	ep_card[ep_total_nr_of_cards].ethaddr[5] = d & 255;

	d = ep__eeprom_read (ELINK_ID_PORT, EEPROM_ADDR_CFG);
	iobase = (d & 0x1f) * 0x10 + 0x200;
	ep_card[ep_total_nr_of_cards].iobase = iobase;

	outb (ELINK_ID_PORT, TAG_ADAPTER + ep_total_nr_of_cards + 1);
	outb (ELINK_ID_PORT, ACTIVATE_ADAPTER_TO_CONFIG);

	GO_WINDOW (iobase, 0);
	d = ep__get_eeprom_data (iobase, EEPROM_PROD_ID);
	ep_card[ep_total_nr_of_cards].prod_id = d;

	if ((d & 0xf0ff) != 0x9050)
	  {
	    printk ("ep: card at addr 0x%Y is unknown model 0x%Y",
			iobase, d);
	    goto unknown_model;
	  }

	/*  Check for connector types:  */

	d  = inw (iobase + EP_W0_CONFIG_CTRL);
	d2 = inw (iobase + EP_W0_ADDRESS_CFG);
	d2 >>= 14;

	switch (d2)
	  {
	    case 0:
		if(d & EP_W0_CC_UTP)
		  {
		    ep_card[ep_total_nr_of_cards].connector = 10;
		    break;
		  }
		else
		  {
		    printk ("ep card %i incorrectly configured for UTP",
			ep_total_nr_of_cards);
		    goto unknown_model;
		  }

	    case 1:
		if(d & EP_W0_CC_AUI)
		  {
		    ep_card[ep_total_nr_of_cards].connector = 5;
		    break;
		  }
		else
		  {
		    printk ("ep card %i incorrectly configured for AUI",
			ep_total_nr_of_cards);
		    goto unknown_model;
		  }

	    case 3:
		if(d & EP_W0_CC_BNC)
		  {
		    ep_card[ep_total_nr_of_cards].connector = 2;
		    break;
		  }
		else
		  {
		    printk ("ep card %i incorrectly configured for BNC",
			ep_total_nr_of_cards);
		    goto unknown_model;
		  }

	    default:
		printk ("ep card %i has unknown connector type",
			ep_total_nr_of_cards);
		goto unknown_model;
	  }

	ep_reset_card (ep_total_nr_of_cards);

	ep_card[ep_total_nr_of_cards].detected = 1;
	ep_total_nr_of_cards ++;

unknown_model:
      }

    return ep_total_nr_of_cards;
  }



void ep_init (int arg)
  {
    char buf[100];
    int n, i, res, rev;
    char *st;

    memset (ep_card, 0, sizeof(ep_card));
    n = ep_probe ();

    for (i=0; i<n; i++)
     if (ep_card[i].detected)
      {
	ep_card[i].m = module_register ("isa0", MODULETYPE_NUMBERED, "ep", "EtherLink III driver");

	if (ep_card[i].m)
	  {
	    res = ports_register (ep_card[i].iobase, 16, ep_card[i].m->shortname);
	    if (!res)
	      {
/*
		printk ("conflicting ports for EtherLink card %i", i);
		goto something_bad;
*/
	      }

/*  TODO:  correct irq stuff  */

	    if (!irq_register (10, (void *)&ep_irqhandler, ep_card[i].m->shortname))
	      {
/*
		printk ("conflicting irq");
		ports_unregister (ep_card[i].iobase);
		goto something_bad;
*/
	      }


	    /*
	     *	Show nice dmesg output:
	     *
	     *	ep0 at isa0 port 0x300-0x30f irq 10: 3C509B, 00:20:af:5e:20:83, UTP
	     *
	     *	(NetBSD shows the following, maybe it is of interest?)
	     *	ep0 at isa0 port 0x300-0x30f irq 10: 3Com 3C509 Ethernet
	     *	ep0: address 00:20:af:5e:20:83, 8KB byte-wide FIFO, 5:3 Rx:Tx split
	     *	ep0: 10baseT (default 10baseT)
	     */

	    isa_module_nametobuf (ep_card[i].m, buf, 100);

	    snprintf (buf+strlen(buf), sizeof(buf)-strlen(buf), ": 3Com ");

	    if (ep_card[i].id == MFG_ID)
	      {
		rev =  (ep_card[i].prod_id & 0x0f00) >> 8;
		if (rev==5)
		  snprintf (buf+strlen(buf), 100-strlen(buf), "3C509B");
		else
		  snprintf (buf+strlen(buf), 100-strlen(buf),
			"3C509 rev %i", rev);
	      }
	    else
		snprintf (buf+strlen(buf), 100-strlen(buf),
			"unknown model 0x%Y prod_id=0x%Y",
			ep_card[i].id, ep_card[i].prod_id);

	    switch (ep_card[i].connector)
	      {
		case 10:	st = "UTP"; break;
		case 5:		st = "AUI"; break;
		case 2:		st = "BNC"; break;
		default:	st = "unknown connector type";
	      }

	    snprintf (buf+strlen(buf), sizeof(buf)-strlen(buf),
		", %y:%y:%y:%y:%y:%y, %s",
		ep_card[i].ethaddr[0], ep_card[i].ethaddr[1],
		ep_card[i].ethaddr[2], ep_card[i].ethaddr[3],
		ep_card[i].ethaddr[4], ep_card[i].ethaddr[5],
		st);

	    printk ("%s", buf);

	    /*  Done.  */
/*
while (1)
  ;
*/
/*
something_bad:
*/
	  }
	else
	  printk ("could not register Etherlink III module for card %i", i);
      }
  }

