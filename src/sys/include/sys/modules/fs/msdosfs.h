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
 *  sys/modules/fs/msdosfs.h  --  msdosfs stuff
 */

#ifndef	__SYS__MODULES__FS__MSDOSFS_H
#define	__SYS__MODULES__FS__MSDOSFS_H


#include <sys/defs.h>


struct msdosfs_superblock
      {
	u_int32_t	nr_of_reserved_sectors;		/*  Offset to the first FAT  */

	u_int32_t	nr_of_fats;
	u_int32_t	sectors_per_fat;
	u_int32_t	bits_per_fatentry;

	u_int32_t	first_rootdir_sector;
	u_int32_t	nr_of_rootdir_entries;

	u_int32_t	first_data_sector;
	u_int32_t	sectors_per_cluster;

	mode_t		vfs_modemask;
	uid_t		vfs_uid;
	gid_t		vfs_gid;
      };


#endif	/*  __SYS__MODULES__FS__MSDOSFS_H  */

