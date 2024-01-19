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
 *  sys/device.h  --  device registry
 */

#ifndef	__SYS__DEVICE_H
#define	__SYS__DEVICE_H


#include <sys/defs.h>
#include <sys/lock.h>
#include <sys/time.h>


struct device
      {
	struct lockstruct lock;

	/*  These require lock to be set:  */
	struct device	*next;
	ref_t		refcount;
	mode_t		vfs_mode;		/*  access permission in the file system  */
	uid_t		vfs_uid;		/*  uid  */
	gid_t		vfs_gid;		/*  gid  */
	dev_t		vfs_dev;		/*  old Unix-style device number (not really used much)  */
	struct timespec	vfs_atime;		/*  times  */
	struct timespec	vfs_ctime;
	struct timespec	vfs_mtime;
	void		*dspec;			/*  Device specific data  */

	/*  These don't require a lock:  */
	size_t		bsize;			/*  block size (hardware dependant) for block devices  */
	char		*name;			/*  name of the device  */
	const char	*owner;			/*  name of owner  */
	u_int32_t	type;			/*  device type  */

	int		(*open)();		/*  (dev)  */

	int		(*close)();		/*  (dev)  */

	int		(*read)();		/*  (dev,blocknr,nrofblocks,buf)  BLOCK  */
						/*  CHAR:  off_t *res, struct device *dev, byte *buf, off_t buflen  */

	int		(*readtip) (struct device *, daddr_t, daddr_t *, daddr_t *);
				/* (dev,blocknr,&tip,&startblock)  */

	int		(*write)();		/*  CHAR:  off_t *res, struct device *dev, byte *buf, off_t buflen  */

	int		(*seek)();

	int		(*ioctl)();
      };


#define	DEVICETYPE_CHAR			1
#define	DEVICETYPE_BLOCK		2


void device_init ();


/*  Register a device. Returns 0 on success, errno on error.  */
int device_register (struct device *newd);

struct device *device_alloc (char *name, const char *owner, u_int32_t type,
	mode_t vfs_mode, uid_t vfs_uid, gid_t vfs_gid, size_t bsize);

int device_free (struct device *d);

/*  Try to unregister a device. Returns 1 on success, 0 on failure.  */
int device_unregister (char *name);


/*  Debug dump:  */
void device_dump ();


#endif	/*  __SYS__DEVICE_H  */

