/*
 *  Copyright (C) 2001 by Anders Gavare.  All rights reserved.
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
 *  sys/vm.h  --  the virtual file system
 */

#ifndef	__SYS__MOUNT_H
#define	__SYS__MOUNT_H


struct export_args
      {
	u_int32_t	ex_flags;		/*  export related flags  */
	u_int32_t	ex_root;		/*  mapping for root uid  */
/*	struct  ucred ex_anon;          *  mapping for anonymous user  */
/*        struct  sockaddr *ex_addr;      *  net address to which exported  */ 
/*        int     ex_addrlen;             *  and the net address length  */ 
/*        struct  sockaddr *ex_mask;      *  mask of valid bits in saddr  */
/*        int     ex_masklen;             *  and the smask length  */
      };


struct ufs_args
      {
	char	*fspec;			/*  mount source  */
	struct	export_args export;	/*  network export information  */
      };


#endif	/*  __SYS__MOUNT_H  */

