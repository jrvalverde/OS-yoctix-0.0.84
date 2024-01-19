int wdc_whichsector (int abssector, int drive, int controller, int *cyl, int *head, int *sector)
  {
    /*  Convert absolute sector number to cylinder, head, and sector (1-based)     */
    /*  abssector = (sector-1) + (nrheads * secpertrack) * cyl + secpertrack * head    */

    int secpertrack = wdc_controller[controller].drive_id[drive][6];
    int nrheads = wdc_controller[controller].drive_id[drive][3];

/*
    printk ("wdc_whichsector: abssector == %i heads=%i, spt=%i", abssector,
		nrheads, secpertrack);
*/ 

    *sector = abssector % secpertrack;
    (*sector) ++;
    abssector /= secpertrack;
    *head = abssector % nrheads;
    *cyl = abssector / nrheads;

/*
    printk (" ==> cyl=%i head=%i sector=%i ==> new absolute = %i",
        *cyl, *head, *sector, (*sector-1)+(nrheads*secpertrack)*(*cyl)+
        secpertrack*(*head));
*/

    if (*cyl >= wdc_controller[controller].drive_id[drive][1])
        return 0;

    return 1;
  }



int wdc_read_sector (int drive, int controller, daddr_t blocknr,
	daddr_t nrofblocks, byte *buf)
  {
    int baseport, head, sector, cylinder, res;

    baseport = wdc_controller[controller].iobase_lo;
    wdc_whichsector (blocknr, drive, controller, &cylinder, &head, &sector);

    res = wdc_sendcommand (baseport, drive, head,
	cylinder, sector, nrofblocks, WDC_CMD_READ_WRETRY);

    if (res)
      return EIO;

    /*  Wait for BUSY to go away...  */
    while ((inb (baseport + WDC_PORT_STATUS) & 0x80)==0x80)
	;

    /*  Wait for DRQ (data request bit) to become active...  */
    while ((inb (baseport + WDC_PORT_STATUS) & 8)==0)
	;

    insw (baseport, buf, 512/2 * nrofblocks);
    return 0;
  }



int wdc_read (struct device *dev, daddr_t blocknr, daddr_t nrofblocks,
              byte *buf, struct proc *p)
  {
    int drive=0, controller=0, res;

    if (!dev || !buf)
      return EINVAL;

/*
    if (dev->refcount == 0)
      return EINVAL;
*/

    /*  dev->name = "wdX" where X is 0,1 on controller 0, and
		2,3 on controller 1.  */
    switch (dev->name[2])
      {
	case 0:  drive=0, controller=0; break;
	case 1:  drive=1, controller=0; break;
	case 2:  drive=0, controller=1; break;
	case 3:  drive=1, controller=1; break;
      }

    lock (&wdc_lock, (void *) "wdc_read", LOCK_BLOCKING | LOCK_RW);
    res = wdc_read_sector (drive, controller, blocknr, nrofblocks, buf);
    unlock (&wdc_lock);

    if (res)
      return EIO;

    return 0;
  }



int wdc_readtip (struct device *dev, daddr_t blocknr, daddr_t *blocks_to_read,
		daddr_t *startblock)
  {
    /*
     *	Return a tip (in blocks_to_read) specifying the number of 512-byte
     *	blocks to read in one call to wdc_read().  Set *startblock to the
     *	block nr where to start reading.
     *
     *  The "tip" is to always to read the full track (on one side of the disk).
     */

    int c,h,s;
    int drive=0, controller=0;
    daddr_t secpertrack, nrheads;



return 0;



    if (!dev || !blocks_to_read)
      return EINVAL;

/*
    if (dev->refcount < 1)
        return EINVAL;
*/

    /*  dev->name = "wdX" where X is 0,1 on controller 0, and
		2,3 on controller 1.  */
    switch (dev->name[2])
      {
	case 0:  drive=0, controller=0; break;
	case 1:  drive=1, controller=0; break;
	case 2:  drive=0, controller=1; break;
	case 3:  drive=1, controller=1; break;
      }

    /*  Get cyl, head, sector. Sector is 1-based!!!  */
    wdc_whichsector (blocknr, drive, controller, &c, &h, &s);
    secpertrack = wdc_controller[controller].drive_id[drive][6];
    nrheads = wdc_controller[controller].drive_id[drive][3];

    *blocks_to_read = secpertrack;
    *startblock = (nrheads * c + h) * secpertrack;

#if DEBUGLEVEL>=3
    printk ("  wdc readtip: %i => chs %i %i %i, blocks_to_read=%i startblock=%i",
        (int)blocknr, c,h,s, (int) *blocks_to_read, (int) *startblock);
#endif

    return 0;
  }

