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
 *  vfs/vfs_mount.c  --  mount a filesystem
 *
 *	vfs_mount()
 *		Mount a filesystem...
 *
 *	vfs_dumpmountinstances()
 *		Debug dump of all mountinstances
 *
 *	TODO:  vfs_unmount()
 *
 *
 *  History:
 *	20 Jan 2000	first version
 */


#include "../config.h"
#include <string.h>
#include <sys/malloc.h>
#include <sys/errno.h>
#include <sys/vfs.h>
#include <sys/std.h>


extern struct filesystem *firstfilesystem;
extern struct mountinstance *firstmountinstance;
extern struct device *first_device;



int vfs_mount (struct proc *p, char *devname, char *mountpoint, char *fstype,
	u_int32_t flags)
  {
    /*
     *	vfs_mount ()
     *	------------
     *
     *	Try to mount a filesystem:
     *
     *	  o)  Find the filesystem with the name 'fstype'.
     *	  o)  Open the device "devname" (which could be a filename if
     *	      the flag VFSMOUNT_FILE is specified).
     *	  o)  Call the filesystem's read_superblock() function.
     *	  o)  Add this mount instance to the "mounted tree".
     *
     *	Return 0 on success, errno on failure.
     */

    struct filesystem *fsptr, *found;
    struct device *adev, *fdev;
    struct mountinstance *mi, *tmpmi;
    struct superblock *sb;
    struct vnode *v = NULL;
    struct stat tmpss;
    char *tmpfilename;
    int res;


    if (!devname || !mountpoint || !fstype)
	return EINVAL;

    /*
     *	All mounts (except the first root mount) need to have
     *	a valid existing mountpoint:
     */

    if (firstmountinstance)
      {
	v = vnode_lookup (p, mountpoint, &res, "vfs_mount");
	if (!v)
	  return res;

	if (v->mounted)
	  {
	    printk ("vfs_mount: mount point '%s' already in use", mountpoint);
	    return EINVAL;
	  }
      }

    /*  Find the filesystem with the name 'fstype':  */
    fsptr = firstfilesystem;
    found = NULL;
    while (fsptr)
      {
	if (fsptr->name && !strcmp((char *)fsptr->name, fstype))
	  {  found = fsptr;  fsptr = NULL;  }

	if (fsptr)
	  fsptr = fsptr->next;
      }

    fsptr = found;
    if (!fsptr)
      {
	printk ("vfs_mount: invalid fstype: '%s'", fstype);
	return EINVAL;
      }

    if (flags & VFSMOUNT_FILE)
      {
	printk ("vfs_mount: mounting using a file is not yet implemented");
	return ENOTBLK;
      }

    if (flags & VFSMOUNT_NODEV)
      {
	adev = NULL;
      }
    else
      {
	/*  Find the device name in the device chain:  */
	adev = first_device;
	fdev = NULL;
	while (adev)
	  {
	    if (adev->name && !strcmp(adev->name, devname))
		{  fdev=adev; adev=NULL;  }

	    if (adev)
		adev = adev->next;
	  }

	if (!fdev)
	  {
	    printk ("vfs_mount: device '%s' not found", devname);
	    return ENOENT;
	  }

	adev = fdev;

	/*  Try to open the device:  */
	res = adev->open (adev, p);
	if (res)
	  {
	    printk ("vfs_mount: error #%i when opening device '%s'", res, devname);
	    return res;
	  }
      }


    sb = (struct superblock *) malloc (sizeof(struct superblock));
    if (!sb)
	return ENOMEM;
    memset (sb, 0, sizeof(struct superblock));


    /*  Create a mountinstance:  */
    mi = (struct mountinstance *) malloc (sizeof(struct mountinstance));
    if (!mi)
      {
	free (sb);
	return ENOMEM;
      }

    memset (mi, 0, sizeof(struct mountinstance));
    mi->next = NULL;
    mi->fs = fsptr;
    mi->superblock = sb;
    mi->flags = flags;
    mi->device_name = (char *) malloc (strlen(devname)+1);
    strncpy (mi->device_name, devname, strlen(devname)+1);
    mi->device = adev;
    mi->mount_point = (char *) malloc (strlen(mountpoint)+1);
    strncpy (mi->mount_point, mountpoint, strlen(mountpoint)+1);
    if (v)
      {
	mi->parent = v->mi;
	mi->mounted_at = v;
	/*  mi->parent_inode should be the inode "before" the mountpoint,
		ie if the mountpoint is /dir1/dir2 then parent_inode should
		be the inode of /dir1. How do we do this? Well, I'm too tired
		to figure out right now, so we do it the clumsy way:  */
	tmpfilename = (char *) malloc (strlen(mountpoint)+1);
	if (!tmpfilename)
	  panic ("out of memory, but this is actually a TODO thing in vfs_mount stuff...");
	memcpy (tmpfilename, mountpoint, strlen(mountpoint)+1);
	/*  remove last part:  */
	res=strlen(tmpfilename)-1;
	while (res>0 && tmpfilename[res]!='/')
	  tmpfilename[res--]=0;
#if DEBUGLEVEL>=4
	printk ("mountpoint='%s' parent='%s' inode=%i", mountpoint, tmpfilename,
	mi->mounted_at->ss.st_ino);
#endif
	res = vfs_stat (p, tmpfilename, &tmpmi, &tmpss);
	if (res)
	    panic ("vfs_mount: mountpoint exists, but not parent?!?");

	mi->parent_inode = tmpss.st_ino;
	free (tmpfilename);
      }
    else
      {
	mi->parent = NULL;
	mi->mounted_at = NULL;
      }

    res = fsptr->read_superblock (mi, p);
    if (res)
      {
	free (sb);
	if (mi->device_name)
		free (mi->device_name);
	if (mi->mount_point)
		free (mi->mount_point);
	free (mi);

	printk ("vfs_mount: error #%i when reading superblock on device '%s'", res, devname);
	return res;
      }

    /*  Add this mountinstance to the mountinstance chain:  */
    tmpmi = firstmountinstance;
    if (!tmpmi)
	firstmountinstance = mi;
    else
      {
	while (tmpmi->next)
		tmpmi = tmpmi->next;
	tmpmi->next = mi;
      }

    if (v)
      {
	v->mounted = mi;
	v->refcount ++;			/*  Should be decreased by vfs_umount()  */
	unlock (&v->lock);
      }

    if (!strcmp(mountpoint, "/") && p->rootdir_vnode==NULL)
      {
	p->rootdir_vnode = vnode_create_rootvnode (mi, p, &res, "vfs_mount");
	if (!p->rootdir_vnode)
	  panic ("vfs_mount: vnode_create_rootvnode returned %i", res);

	unlock (&p->rootdir_vnode->lock);
      }

    return 0;
  }



void vfs_dumpmountinstances()
  {
    struct mountinstance *m;

    printk ("vfs_dumpmountinstances():");
    m = firstmountinstance;

    while (m)
      {
	printk ("  '%s' at '%s' type '%s' flags=%i",
		m->mount_point, m->device_name, m->fs->name, m->flags);
	m = m->next;
      }
  }

