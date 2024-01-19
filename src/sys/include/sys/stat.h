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
 *  sys/stat.h  --  file stat struct
 */

#ifndef __SYS__STAT_H
#define	__SYS__STAT_H

#include <sys/defs.h>


/*
 *  A "reserved" inode number to specify the root directory. (This is not
 *  neccessarily the same as the ffs root inode, or the msdosfs root inode.)
 */

#define	ROOT_INODE	0


struct device;

struct stat
      {
	dev_t		st_dev;			/*  inode's device  */
	inode_t		st_ino;			/*  inode's number  */
	mode_t		st_mode;		/*  inode protection mode  */
	u_int32_t	st_nlink;		/*  number of hard links  */
	uid_t		st_uid;			/*  user ID of the file's owner  */
	gid_t		st_gid;			/*  group ID of the file's group  */
	struct device	*st_rdev;		/*  pointer to device (special files only)  */
	time_t		st_atime;		/*  time of last access  */
	long		st_atimensec;		/*  nsec of last access  */
	time_t		st_mtime;		/*  time of last data modification  */
	long		st_mtimensec;		/*  nsec of last data modification  */
	time_t		st_ctime;		/*  time of last file status change  */
	long		st_ctimensec;		/*  nsec of last file status change  */
	off_t		st_size;		/*  file size, in bytes  */
	daddr_t		st_blocks;		/*  blocks allocated for file  */
	u_int32_t	st_blksize;		/*  optimal blocksize for I/O  */
	u_int32_t	st_flags;		/*  user defined flags for file  */
	u_int32_t	st_gen;			/*  file generation number  */
      };


#define	S_IXOTH		0000001		/*  X for other  */
#define	S_IWOTH		0000002		/*  W for other  */
#define	S_IROTH		0000004		/*  R for other  */
#define	S_IXGRP		0000010		/*  X for group  */
#define	S_IWGRP		0000020		/*  W for group  */
#define	S_IRGRP		0000040		/*  R for group  */
#define	S_IXUSR		0000100		/*  X  */
#define	S_IWUSR		0000200		/*  W  */
#define	S_IRUSR		0000400		/*  R  */
#define	S_ISVTX		0001000		/*  save swapped text even after use  */
#define	S_ISGID		0002000		/*  set gid on exec  */
#define	S_ISUID		0004000		/*  set uid on exec  */
#define	S_IFIFO		0010000		/*  FIFO  (named pipe)  */
#define	S_IFCHR		0020000		/*  character special device  */
#define	S_IFDIR		0040000		/*  directory  */
#define	S_IFBLK		0060000		/*  block special device  */
#define	S_IFREG		0100000		/*  regular  */
#define	S_IFLNK		0120000		/*  symbolic link  */
#define	S_IFSOCK	0140000		/*  socket  */
#define	S_IFWHT		0160000		/*  whiteout  */

#define	S_ISFIFO(m)	((m&0170000)==S_IFIFO)
#define	S_ISCHR(m)	((m&0170000)==S_IFCHR)
#define	S_ISDIR(m)	((m&0170000)==S_IFDIR)
#define	S_ISBLK(m)	((m&0170000)==S_IFBLK)
#define	S_ISREG(m)	((m&0170000)==S_IFREG)
#define	S_ISLNK(m)	((m&0170000)==S_IFLNK)
#define	S_ISSOCK(m)	((m&0170000)==S_IFSOCK)
#define	S_ISWHT(m)	((m&0170000)==S_IFWHT)

#define	S_IRWXU		(S_IRUSR | S_IWUSR | S_IXUSR)
#define	S_IRWXG		(S_IRGRP | S_IWGRP | S_IXGRP)
#define	S_IRWXO		(S_IROTH | S_IWOTH | S_IXOTH)

#define	ACCESSPERMS	(S_IRWXU | S_IRWXG | S_IRWXO)	/*  0777  */


#endif	/*  __SYS__STAT_H  */
