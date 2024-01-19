
int wdc_partition__open (struct device *dev)
  {
    return 0;
  }



int wdc_partition__close (struct device *dev)
  {
    return 0;
  }



int wdc_partition__read (struct device *dev, daddr_t blocknr,
	daddr_t nrofblocks, byte *buf, struct proc *p)
  {
    return wdc_read (
	((struct wdc_partitiondev *) dev->dspec) -> rawdev,
	blocknr + ((struct wdc_partitiondev *) dev->dspec) -> start_offset,
	nrofblocks, buf, p);
  }



int wdc_register_partition (struct device *rawdev,
	daddr_t start, daddr_t size, char partitionname)
  {
    /*
     *	This function registers a device with a name consisting of the name
     *	of the raw device plus a partition letter.
     *
     *	TODO:  if the name was already in use, maybe the partition name
     *	should be automatically increased here?
     */

    char tmpbuf[16];
    struct device *dev;

    if (!rawdev || !partitionname)
      return EINVAL;

    snprintf (tmpbuf, sizeof(tmpbuf), "%s%c", rawdev->name,
	partitionname);

/*
    printk ("wdc_register_partition: '%s', start=%i size=%i",
	tmpbuf, (int)start, (int)size);
*/

    dev = device_alloc (tmpbuf,
	wdc_m->shortname, DEVICETYPE_BLOCK, 0640, 0,0,
	rawdev->bsize);

    if (dev)
      {
	dev->open    = wdc_partition__open;
	dev->close   = wdc_partition__close;
/*	dev->write   = wdc_partition__write; */
	dev->read    = wdc_partition__read;
/*	dev->seek    = wdc_partition__seek; */
/*	dev->ioctl   = wdc_partition__ioctl; */
/*	dev->readtip = wdc_partition__readtip; */

	if (device_register (dev))
	  {
	    printk ("wdc_register_partition: could not register %s",
		tmpbuf);
	    device_free (dev);
	  }
      }

    ((struct wdc_partitiondev *) dev->dspec) =
	(struct wdc_partitiondev *) malloc (sizeof(struct wdc_partitiondev));
    if (! ((struct wdc_partitiondev *) dev->dspec) )
      {
	device_unregister (dev->name);
	device_free (dev);
	return ENOMEM;
      }

    ((struct wdc_partitiondev *) dev->dspec) -> rawdev = rawdev;
    ((struct wdc_partitiondev *) dev->dspec) -> start_offset = start;
    ((struct wdc_partitiondev *) dev->dspec) -> size = size;

    return 0;
  }



int wdc_read_mbr (struct device *dev)
  {
    /*
     *	Read the MBR of a raw device and register devices for each PC
     *	partition:
     *
     *	1)  If the PC partition contains a (valid) BSD disklabel, then register
     *	    BSD partitions a,b,c, ... (unless another disklabel had been found
     *	    earlier, in which case letters >= i will be used)
     *
     *	2)  If the PC partition is non-disklabeled, then add it as letter >= i.
     *
     *	TODO:  Handle "extended" partitions... (?)
     */

    int res;
    int i, j;
    byte *mbr;
    byte *disklabel_buf;
    struct disklabel *dl;
    int found_disklabel = 0;
    char cur_partition_letter = 'i';
    daddr_t start_sector, size;
    struct dos_partition *dp;


    if (!dev)
      return EINVAL;

    mbr = (byte *) malloc (dev->bsize);
    if (!mbr)
      return ENOMEM;

    disklabel_buf = (byte *) malloc (dev->bsize);
    if (!disklabel_buf)
      {
	free (mbr);
	return ENOMEM;
      }

    /*  Read the MBR:  */
    res = wdc_read (dev, DOSBBSECTOR, 1, mbr, NULL);
    if (res)
      {
	free (mbr);
	free (disklabel_buf);
	return EIO;
      }

/*
    printk ("MBR dump for '%s':", dev->name);
    printk ("  #: BF Start    Ty End      StartLBA Size");

    for (i=0; i<4; i++)
      {
	printk ("  %i: %y %y,%y,%y %y %y,%y,%y %x %x",
	  i, buf[446+i*16+0], buf[446+i*16+1], buf[446+i*16+2], buf[446+i*16+3],
	  buf[446+i*16+4], buf[446+i*16+5], buf[446+i*16+6], buf[446+i*16+7],
	  *((int *)&buf[446+i*16+8]),
	  *((int *)&buf[446+i*16+12]));
      }
*/


    dp = (struct dos_partition *) (mbr + DOSPARTOFF);

    for (i=0; i<NDOSPART; i++)
      switch (dp[i].dp_typ)		/*  PC partition type  */
	{
	  case 0:
		break;

	  case DOSPTYP_EXTEND:
	  case DOSPTYP_EXTENDL:
		printk ("wdc_read_mbr: %s contains extended partitions (TODO)",
			dev->name);
		break;

	  default:
		start_sector	= dp[i].dp_start;
		size		= dp[i].dp_size;

		/*  Check for a BSD disklabel:  */
		res = wdc_read (dev, start_sector + LABELSECTOR, 1,
				disklabel_buf, NULL);
		if (res)
		  {
		    printk ("wdc_read_mbr: read error from '%s'", dev->name);
		    break;
		  }
		dl = (struct disklabel *) (disklabel_buf + LABELOFFSET);
		if (dl->d_magic == DISKMAGIC)
		  {
		    if (found_disklabel)
		      printk ("wdc_read_mbr: warning: multiple BSD disklabels found");
		    for (j=0; j<MAXPARTITIONS; j++)
		      if (dl->d_partitions[j].p_size > 0)
			{
			  /*  Add this (BSD) partition.  */
			  if (found_disklabel)
			    wdc_register_partition (dev,
				dl->d_partitions[j].p_offset,
				dl->d_partitions[j].p_size,
				cur_partition_letter ++);
			  else
			    wdc_register_partition (dev,
				dl->d_partitions[j].p_offset,
				dl->d_partitions[j].p_size,
				'a' + j);
			}
		    found_disklabel = 1;
		  }
		else
		  {
		    /*  Add this (PC) partition:  */
			    wdc_register_partition (dev,
				start_sector, size, cur_partition_letter ++);
		  }
	}

    free (mbr);
    free (disklabel_buf);

    return 0;
  }

