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
 *  modules/fs/msdosfs/msdosfs.c  --  MS-DOS filesystem driver
 *
 *  History:
 *	20 Jan 2000	first version
 *	18 Feb 2000	fixed bug in _next_cluster(), adding _read()
 */



#include "../../../config.h"
#include <string.h>
#include <stdio.h>
#include <sys/std.h>
#include <sys/stat.h>
#include <sys/malloc.h>
#include <sys/errno.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/module.h>
#include <sys/device.h>
#include <sys/modules/fs/msdosfs.h>



struct module *msdosfs_m;
struct filesystem *msdosfs_fs;



inode_t msdosfs_next_cluster (struct mountinstance *mi, inode_t cluster)
  {
    /*  Find the "next cluster" number in the first FAT.
	Return 0 if there are no more clusters...  */

    struct msdosfs_superblock *fssb = (struct msdosfs_superblock *)
	mi->superblock->fs_superblock;
    int fatsector;
    int nr_of_entries;
    int ofs;
    int res;
    byte buf [512];
    int new_cluster;

    fatsector = fssb->nr_of_reserved_sectors;
    nr_of_entries = (int)mi->superblock->nr_of_blocks / fssb->sectors_per_cluster;
    if (cluster > nr_of_entries)
      {
	printk ("msdosfs_next_cluster: cluster number %i too high (max = %i)",
		cluster, nr_of_entries);
	return 0;
      }

    if (fssb->bits_per_fatentry == 12)
      {
	ofs = 3*cluster;
	ofs /= 2;

	res = block_read (mi, (daddr_t) (fatsector+ofs/512), (daddr_t) 1, buf);
	if (res)
{
printk ("  _next_cluster: block_read returned %i", res);
	  return 0;
}

	new_cluster = buf[ofs&511];

	/*  Special weird stuff if the word is split onto two sectors:  */
	if ((ofs & 511) == 511)
	  {
	    res = block_read (mi, (daddr_t) (fatsector+(ofs+1)/512), (daddr_t) 1, buf);
	    if (res)
{
printk ("  _next_cluster: block_read (B) returned %i", res);
		return 0;
}
	    new_cluster = new_cluster + buf[0]*256;
	  }
	else
	  new_cluster = new_cluster + buf[(ofs&511)+1]*256;

	if (cluster & 1)
	  new_cluster >>= 4;
	else
	  new_cluster &= 0xfff;

	if (new_cluster >= 0xff0)
		return 0;

	return (inode_t) new_cluster;
      }
    else
	printk ("msdosfs_next_cluster: unsupported bits_per_fatentry (%i)",
		fssb->bits_per_fatentry);

    return 0;
  }



int msdosfs_cluster_to_sector (struct mountinstance *mi, int cluster)
  {
    /*  Returns the absolute sector number for a cluster number...
	Note that since a cluster can be more than one sector, you may have
	to read more than one sector.  */

    int s;
    struct msdosfs_superblock *fssb = (struct msdosfs_superblock *)
	mi->superblock->fs_superblock;

    s = fssb->first_data_sector + fssb->sectors_per_cluster * (cluster-2);

/*
printk ("[ cluster %i ==> sector %i ]", cluster, s);
*/
    return s;

    /*  the -2 stuff is because the two first entries in a FAT are reserved...  */
  }



int msdosfs_read_superblock (struct mountinstance *mi)
  {
    int res;
    byte buf[512];
    struct msdosfs_superblock  *msdossb;

    mi->superblock->blocksize = 512;

    res = block_read (mi, (daddr_t) 0, (daddr_t) 1, buf);
    if (res)
	return res;

    if ((buf[0xb]+buf[0xc]*256)!=512)
      {
	printk ("msdosfs_read_superblock(): cannot handle sector size %i",
		buf[0xb]+buf[0xc]*256);
	return ENODEV;
      }

/*  TODO:  how/where is msdossb freed???  */
    msdossb = (struct msdosfs_superblock *) malloc (sizeof(struct msdosfs_superblock));
    if (!msdossb)
	return ENOMEM;

    msdossb->nr_of_reserved_sectors = buf[0xe]+buf[0xf]*256;
    msdossb->nr_of_fats = buf[0x10];
    msdossb->sectors_per_fat = buf[0x16]+buf[0x17]*256;
    msdossb->first_rootdir_sector = msdossb->sectors_per_fat * msdossb->nr_of_fats
		+ msdossb->nr_of_reserved_sectors;
    msdossb->nr_of_rootdir_entries = buf[0x11]+buf[0x12]*256;
    msdossb->first_data_sector = msdossb->first_rootdir_sector
		+ msdossb->nr_of_rootdir_entries / 16;
    msdossb->sectors_per_cluster = buf[0xd];

    if ((buf[0x13]+buf[0x14]*256)>0)
	mi->superblock->nr_of_blocks =
	  buf[0x13]+buf[0x14]*256 -			/*  standard "nr of sectors"  */
	  (buf[0x16]+buf[0x17]*256)*buf[0x10] -		/*  sec/fat * nrofFATs  */
	  (buf[0xe]+buf[0xf]*256) -			/*  reserved sectors  */
	  (buf[0x11]+buf[0x12]*256)/16			/*  nr of rootdir sectors  */
	  ;
    else
	mi->superblock->nr_of_blocks =
	  buf[0x20]+buf[0x21]*256+buf[0x22]*65536+buf[0x23]*16777216 -  /*  extended "nr of sectors"  */
	  (buf[0x16]+buf[0x17]*256)*buf[0x10] -		/*  sec/fat * nrofFATs  */
	  (buf[0xe]+buf[0xf]*256) -			/*  reserved sectors  */
	  (buf[0x11]+buf[0x12]*256)/16			/*  nr of rootdir sectors  */
	  ;

    msdossb->vfs_uid = 0;
    msdossb->vfs_gid = 0;
    msdossb->vfs_modemask = 022;
    msdossb->bits_per_fatentry = 12;	/*  TODO  */

    mi->superblock->root_inode = 1;
    mi->superblock->fs_superblock = msdossb;

    return 0;
  }



int msdosfs_read (struct vnode *v, off_t offset, byte *buffer, off_t length, off_t *transfered)
  {
    /*
     *	Try to read data from a file.
     *	Return 0 on success, errno on failure. Set the variable pointed to by
     *	transfered to the number of bytes actually transfered.
     *
     *	I know, this is The Mega-stupid Way of doing this, but it should
     *	work until I have time to implement an optimized version:
     *
     *		Traverse the cluster chain until we are far enough into the
     *		file. Read one cluster at a time (checking for EOF too) and
     *		go to the next cluster until either EOF is found or we have
     *		read 'length' bytes.
     */

    struct msdosfs_superblock *fssb;
    int bytes_per_cluster;
    u_int32_t cur_cluster;
    u_int32_t cur_pos;	/*  TODO:  large file support?  */
    byte *slow_tmpbuf;
    int res;
    u_int32_t cur_sector;
    u_int32_t cplen, cpofs;
    u_int32_t total_to_read = length;

    if (!v || !buffer || length<1)
	return EINVAL;
/*
printk ("vnode %x, offset %i, buffer %x, length %i, transfered %x",
	v,(int)offset,buffer,(int)length,transfered);
*/
    *transfered = 0;

    fssb = (struct msdosfs_superblock *) v->mi->superblock->fs_superblock;
    bytes_per_cluster = fssb->sectors_per_cluster * v->mi->superblock->blocksize;

    cur_cluster = v->ss.st_ino;
    cur_pos = 0;

    /*  Traverse the cluster chain until we are at the first cluster
	which should actually be read:  */
/*printk ("beginning cluster = %i", cur_cluster);*/
    while (offset >= cur_pos+bytes_per_cluster && cur_pos < v->ss.st_size)
      {
	cur_cluster = msdosfs_next_cluster (v->mi, cur_cluster);
/*printk ("  (next cluster = %i, offset = %i)", cur_cluster, cur_pos);*/
	if (cur_cluster == 0)
	  return 0;
	cur_pos += bytes_per_cluster;
      }

    if (cur_pos >= v->ss.st_size)
	return 0;

    slow_tmpbuf = (byte *) malloc (bytes_per_cluster);
    if (!slow_tmpbuf)
	return ENOMEM;

    /*  Transfer all the data:  */
    while (*transfered < total_to_read)
      {
	/*  Read one cluster into slow_tmpbuf:  */
	cur_sector = msdosfs_cluster_to_sector (v->mi, cur_cluster);
	res = block_read (v->mi, (daddr_t) (cur_sector), (daddr_t)
		fssb->sectors_per_cluster, slow_tmpbuf);
	if (res)
	  {
	    free (slow_tmpbuf);
	    return res;
	  }

	/*  The data we want is at offset 'offset-cur_pos' in the slow_tmpbuf, and
		is of length 'length' (or at most to the end of the cluster):  */
	cpofs = offset-cur_pos;
	cplen = length>=(bytes_per_cluster-cpofs)?  bytes_per_cluster-cpofs : length;

/*
printk ("  offset=%i cur_pos=%i length=%i bytes_per_cluster=%i cpofs=%i cplen=%i",
	(int)offset, (int)cur_pos, (int)length, (int)bytes_per_cluster,
	(int)cpofs,(int)cplen);
*/

	memcpy (buffer, slow_tmpbuf+cpofs, cplen);
	buffer += cplen;
	length -= cplen;
	offset += cplen;
	(*transfered) += cplen;
	if (cur_pos >= v->ss.st_size)
	  {
	    free (slow_tmpbuf);
	    (*transfered) -= (cur_pos - v->ss.st_size);
	    return 0;
	  }
	if (*transfered >= total_to_read)
	  {
	    free (slow_tmpbuf);
	    return 0;
	  }

	/*  Advance to the next cluster:  */
	cur_cluster = msdosfs_next_cluster (v->mi, cur_cluster);
	if (cur_cluster == 0)
	  {
	    free (slow_tmpbuf);
	    return 0;
	  }
	cur_pos += bytes_per_cluster;
      }

    free (slow_tmpbuf);
    return 0;
  }



int msdosfs__findfilenameinbuf (byte *buf, unsigned char *name)
  {
    /*  Scans the 512-byte sector buffer for the name 'name'.
	Returns the _offset in the sector_ to the file entry if it is
	found, -1 otherwise.  */

    int i, j,j2;
    unsigned char dosname [16];
    unsigned char c;

    /*  Check all existing non-deleted non-volumelabel entries:  */
    for (i=0; i<16; i++)
      if (buf[i*32]!=0 && buf[i*32]!=0xe5 && (buf[i*32+0xb]&8)==0)
	{
	  /*  Form the dosname from the data in buf:  */
	  memset (dosname, 0, 16);
	  j = 0;  j2 = 0;
	  while (j<11)
	    {
	      c = buf[i*32 + j];
	      if (c==0x05)
		c = 0xe5;
	      if (c>='A' && c<='Z')
		c += 32;

	      if (c!=' ')
		{
		  if (j==8)
		    dosname[j2++] = '.';
		  dosname[j2++] = c;
		  dosname[j2] = 0;
		}

	      j++;
	    }

/*
printk ("** dosname = '%s'", dosname);
*/

	  /*  Is this the name we're looking for?  */
	  if (!strcmp(dosname, name))
	    return i*32;
	}

    return -1;
  }



int msdosfs_namestat (struct mountinstance *mi, inode_t dirinode,
	char *name, struct stat *ss)
  {
    /*
     *	Scan the directory 'dirinode' for a file named 'name' and if found
     *	we fill the stat struct 'ss' with its data (creation time, uid,gid etc).
     *
     *	If 'dirinode' == 1, then we read the root directory, otherwise
     *	we read the directory beginning at cluster 'dirinode'.
     *
     *	A special case if the namestat failed: check for "." or ".."
     *	(these have to be faked in the root directory, ie if dirinode = 1)
     *
     *	Returns 0 on success, errno on failure.
     */

    struct msdosfs_superblock *fssb = (struct msdosfs_superblock *)
	mi->superblock->fs_superblock;
    daddr_t	rootdir = fssb->first_rootdir_sector;
    daddr_t	dirsector;
    int		dir_length;		/*  Nr of clusters in dir  */
    int		sectors_per_cluster;
    int		res;
    byte	buf[512];
    int		i, j;


    if (!mi || !name || !ss)
	return EINVAL;

    /*  How many clusters should we read?  */
    sectors_per_cluster = fssb->sectors_per_cluster;
    if (dirinode == 1)
	dir_length = fssb->nr_of_rootdir_entries/16;
    else
	dir_length = -1;

/*
printk ("!!! dirinode=%i dir_length=%i", (int)dirinode, dir_length);
*/

    if (dirinode == 1)
      {
	for (i = 0; i<dir_length; i++)
	  {
	    res = block_read (mi, (daddr_t) (rootdir+i), (daddr_t) 1, buf);
	    if (res)
		return res;

	    res = msdosfs__findfilenameinbuf (buf, name);
	    if (res > -1)
		goto namestat__found;
	  }

	/*  Special case for the rootdirectory only: check for "." or "..":  */
	if (!strcmp (name, ".") || !strcmp(name, ".."))
	  {
	    /*  Fake a found directory entry:  */
	    memset (buf, 0, 32);	/*  only clear first 32 bytes to save time  */
	    res = 0;			/*  offset into buf  */
	    buf[res+0x1a] = 1;		/*  inode = 1  */
	    buf[res+0xb] = 0x10;	/*  msdos flags: 0x10 = directory  */
	    goto namestat__found;
	  }

	return ENOENT;
      }


    /*  Non-root directory:  read one cluster at a time  */

    while (dirinode > 0)
      {
	dirsector = msdosfs_cluster_to_sector (mi, dirinode);
	for (j=0; j<sectors_per_cluster; j++)
	  {
	    res = block_read (mi, (daddr_t) (dirsector+j), (daddr_t) 1, buf);
	    if (res)
		return res;

	    res = msdosfs__findfilenameinbuf (buf, name);
	    if (res > -1)
		  goto namestat__found;
	  }

	/*  Find next cluster number:  */
	dirinode = msdosfs_next_cluster (mi, dirinode);
      }

    return ENOENT;


namestat__found:

    ss->st_dev = mi->device->vfs_dev;
    ss->st_ino = buf[res+0x1a]+buf[res+0x1b]*256;
    ss->st_mode = (buf[res+0xb]&0x10? S_IFDIR:S_IFREG) + (0755 & (~fssb->vfs_modemask));
    ss->st_nlink = 1;
    ss->st_uid = fssb->vfs_uid;
    ss->st_gid = fssb->vfs_gid;
    ss->st_rdev = NULL;
/*  TODO:  MSDOS -> unix date/time stuff  */
    ss->st_atime = 0;
    ss->st_atimensec = 0;
    ss->st_mtime = 0;
    ss->st_mtimensec = 0;
    ss->st_ctime = 0;
    ss->st_ctimensec = 0;
    ss->st_size = buf[res+0x1c]+buf[res+0x1d]*256+buf[res+0x1e]*65536+buf[res+0x1f]*16777216;
    if ((ss->st_mode & 0170000) == S_IFDIR)
	ss->st_size = mi->superblock->blocksize;
    ss->st_blocks = ss->st_size >> 9;	/*  assumes that blocksize = 512  */
    if ((ss->st_blocks << 9) < ss->st_size)
	ss->st_blocks ++;
    ss->st_blksize = 512;
    ss->st_flags = 0;
    ss->st_gen = 1;

    /*  Special hack: if this is a directory, and its inode==0, then it
	refers to the root directory which I want to be at inode 1.  */
    if (((ss->st_mode & 0170000) == S_IFDIR) && ss->st_ino==0)
	ss->st_ino = 1;

    return 0;
  }



int msdosfs_namei (struct mountinstance *mi, inode_t dirinode, char *name, inode_t *inode)
  {
    /*
     *	Call namestat to fill in a stat structure. Grab the inode number
     *	from the st_ino field.
     *
     *	Returns 0 on success, errno on failure.
     */

    struct stat ss;
    int res;

    res = msdosfs_namestat (mi, dirinode, name, &ss);
    if (res)
	return res;

    (*inode) = ss.st_ino;
    return 0;
  }



int msdosfs_istat (struct mountinstance *mi, inode_t inode, struct stat *ss)
  {
    struct msdosfs_superblock *fssb = (struct msdosfs_superblock *)
        mi->superblock->fs_superblock;

    if (inode!=1)
      {
	printk ("msdosfs_istat(): can only istat the root directory (inode 1)");
	return EINVAL;
      }

    ss->st_dev = mi->device->vfs_dev;
    ss->st_ino = inode;
    ss->st_mode = (0777 & (~fssb->vfs_modemask)) | S_IFDIR;
    ss->st_nlink = 1;
    ss->st_uid = fssb->vfs_uid;
    ss->st_gid = fssb->vfs_gid;
    ss->st_rdev = NULL;
    ss->st_atime = 0;
    ss->st_atimensec = 0;
    ss->st_mtime = 0;
    ss->st_mtimensec = 0;
    ss->st_ctime = 0;
    ss->st_ctimensec = 0;
    ss->st_size = fssb->nr_of_rootdir_entries*32;
    ss->st_blocks = fssb->nr_of_rootdir_entries/16;
    ss->st_blksize = 512;
    ss->st_flags = 0;
    ss->st_gen = 1;
    return 0;
  }



void msdosfs_init (int arg)
  {
    msdosfs_m = module_register ("vfs", 0, "msdosfs", "MS-DOS filesystem");

    if (!msdosfs_m)
	return;

    msdosfs_fs = vfs_register ("msdosfs");
    if (!msdosfs_fs)
      {
	module_unregister (msdosfs_m);
	return;
      }

    /*  Init msdosfs_fs:  */
    msdosfs_fs->read_superblock = &msdosfs_read_superblock;
    msdosfs_fs->namei = &msdosfs_namei;
    msdosfs_fs->namestat = &msdosfs_namestat;
    msdosfs_fs->istat = &msdosfs_istat;
    msdosfs_fs->read = &msdosfs_read;

    msdosfs_fs->active = 1;
  }

