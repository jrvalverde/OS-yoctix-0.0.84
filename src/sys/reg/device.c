/*
 *  Copyright (C) 2000,2001 by Anders Gavare.  All rights reserved.
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
 *  reg/device.c  --  device registry
 *
 *	Devices are registered by their device drivers. For example, the
 *	i386 console driver "pccon" may wish to register devices called
 *	"ttyC0", "ttyC1", and so on.  A floppy driver could register
 *	"fd0", "fd1" etc.
 *
 *	The device structs that are placed in the chain begining with
 *	first_device are also used by the "devfs" virtual file system.
 *
 *
 *	device_init()
 *		Called from kern/init_main.c
 *
 *	device_alloc()
 *		Allocate and (partly) fill-in data in a device struct
 *
 *	device_register()
 *		Register a device
 *
 *	device_unregister()
 *		Unregister a device
 *
 *	device_dump()
 *		Debug dump of all registered devices
 *
 *  History:
 *	14 Jan 2000	first version
 *	12 Dec 2000	device_alloc needs to be called before device_register
 */


#include "../config.h"
#include <stdio.h>
#include <string.h>
#include <sys/std.h>
#include <sys/malloc.h>
#include <sys/interrupts.h>
#include <sys/device.h>
#include <sys/errno.h>



struct device *first_device = NULL;
extern struct timespec system_time;



void device_init ()
  {
    /*  Nothing.  */
  }



int device_register (struct device *newd)
  {
    /*
     *  device_register ()
     *	------------------
     *
     *	Try to register a device by adding it to the first_device chain.
     *
     *	Returns 0 on success, errno on error.
     *
     *	TODO: when lots of devices need to be registered, then this
     *		routine will need to be rewritten... hash tables?
     */

    int oldints;
    struct device *d;
    dev_t nr = 0;

    if (!newd)
      return EINVAL;
    if (!newd->name)
      return EINVAL;

    oldints = interrupts (DISABLE);

    /*  Is there already a device registered with this name?  */
    d = first_device;
    if (d)
      nr = d->vfs_dev;

    while (d)
      {
	if (!strcmp(d->name, newd->name))
	  {
	    interrupts (oldints);
	    return EEXIST;
	  }

	if (d->vfs_dev > nr)
	  nr = d->vfs_dev;

	d = d->next;
      }

    /*  TODO:  use the lowest available device number  */
    newd->vfs_dev = nr + 1;

    /*  Add newd to the device list:  */
    newd->next = first_device;
    first_device = newd;

    /*  Also, interrupts should be disabled to read system_time atomically:  */
    newd->vfs_atime = system_time;
    newd->vfs_ctime = newd->vfs_atime;
    newd->vfs_mtime = newd->vfs_atime;

    /*  Return successfully:  */
    interrupts (oldints);

    return 0;
  }



int device_free (struct device *d)
  {
    /*
     *  device_free ()
     *	--------------
     *
     *	Undo anything that device_alloc() does.
     *	Return 0 on success, errno on error.
     */

    if (!d)
      return EINVAL;

    if (d->name)
      free (d->name);

    free (d);

    return 0;
  }



struct device *device_alloc (char *name, const char *owner, u_int32_t type,
	mode_t vfs_mode, uid_t vfs_uid, gid_t vfs_gid, size_t bsize)
  {
    /*
     *  device_alloc ()
     *	---------------
     *
     *	Try to register a device by adding it to the first_device chain.
     *	A device number (if any) should be included in the name.
     *
     *	Returns a pointer to the device structure on success, NULL on failure.
     *	The caller needs to set open/read/write/ioctl etc. in the device
     *	struct and then call device_register().
     */

    struct device *d;

    if (!name || !owner)
      return NULL;

    d = (struct device *) malloc (sizeof(struct device));
    if (!d)
      return NULL;

    memset (d, 0, sizeof(struct device));
    d->next	= NULL;

    d->name	= malloc (strlen(name)+1);
    if (!d->name)
      return NULL;
    memcpy (d->name, name, strlen(name)+1);

    d->type	= type;
    d->owner	= owner;
    d->refcount	= 0;
    d->vfs_mode	= vfs_mode;
    d->vfs_uid	= vfs_uid;
    d->vfs_gid	= vfs_gid;
    d->bsize    = bsize;

    return d;
  }



int device_unregister (char *name)
  {
    /*
     *	device_unregister ()
     *	--------------------
     *
     *	Try to unregister device 'name' by removing it from the
     *	device list.  Fails if refcount > 0.
     *
     *	Returns 0 on success, errno on failure.
     */

    struct device *d, *tmp, *prev;
    int oldints;

    if (!name)
	return EINVAL;

    oldints = interrupts (DISABLE);

    /*  Find 'name' in the device list:  */
    d = first_device;
    tmp = NULL;
    prev = NULL;

    while (d)
      {
	if (!strcmp(d->name, name))
	  { tmp = d;  d = NULL; }

	if (d)
	  {
	    prev = d;
	    d = d->next;
	  }
      }

    if (tmp)
      {
	/*  Try to remove tmp from the device list:  */
	if (tmp->refcount > 0)
	  {
	    interrupts (oldints);
	    return EBUSY;
	  }

	if (tmp->refcount < 0)
	  panic ("device_unregister: device '%s'->refcount=%i !!!",
		tmp->name, tmp->refcount);

	if (!prev)
	  first_device = tmp->next;
	else
	  prev->next = tmp->next;

	free (tmp->name);
	free (tmp);
      }
    else
      return ENXIO;	/*  Device not found. (TODO: find better errno)  */

    interrupts (oldints);
    return 0;
  }



void device_dump ()
  {
    struct device *d;
    int oldints;

    oldints = interrupts (DISABLE);

    printk ("device_dump():");
    d = first_device;
    while (d)
      {
	printk ("  '%s' (owner '%s') uid=%i gid=%i mode=%i type=%i refcount=%i",
		d->name, d->owner, d->vfs_uid, d->vfs_gid, d->vfs_mode, d->type, d->refcount);
	d = d->next;
      }

    interrupts (oldints);
  }

