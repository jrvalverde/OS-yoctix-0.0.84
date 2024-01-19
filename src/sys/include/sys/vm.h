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
 *  sys/vm.h  --  the virtual memory system
 */

#ifndef	__SYS__VM_H
#define	__SYS__VM_H


#include <sys/defs.h>
#include <sys/md/vm.h>
#include <sys/md/machdep.h>		/*  for PAGESIZE  */


/*  Maybe these macros should be in sys/defs.h instead?  */

#define	round_down_to_page(x)	((size_t)x & ~(PAGESIZE-1))
#define	round_up_to_page(x)	((((size_t)x-1) | (PAGESIZE-1)) + 1)



/*
 *  The vm_object struct
 *  --------------------
 *
 *  A virtual memory object can be one of the following:
 *
 *	(o)  A memory-mapped file (read-only, copy-on-write, or write-back)
 *	(o)  Anonymous (for example bss data and stack)
 *	(o)  Shadow objects (with reference to another object)
 *
 *  A virtual memory object can be shared by several processes. The refcount
 *  tells how many processes actually share the object.
 *
 *  A shadow object may have at most two referers (other vm_objects), and their
 *  addresses should be stored in shadow_ref1 and shadow_ref2. This is because
 *  we want to be able to free modified pages when processes die etc.
 */

struct vm_object
      {
	/*
	 *  If this is a shadow object, then this is a pointer to
	 *  the object we're shadowing:
	 */
	struct vm_object	*next;

	/*  Object lock:  */
	struct lockstruct	lock;

	u_int16_t		type;		/*  object type  */
	u_int16_t		flags;		/*  additional flags  */
	ref_t			refcount;	/*  reference count  */

	/*  If this object is a shadow object, then these are the two shadow objects
	    referencing it (ie after a fork). (There can not be more than two.)
	    Set these to NULL if there are no shadow objects refering this object.  */
	struct vm_object	*shadow_ref1;
	struct vm_object	*shadow_ref2;

	/*  If this is a file object, then this is the vnode we're representing:  */
	struct vnode		*vnode;

	/*  Pointer to the vm_object's pagein function:  */
	void *			(*pagein)();		/*  *errno, vmregion, vmobject, linearaddr  */

	/*  Index into the MCB array to the first page in the chain:  */
	size_t			page_chain_start;
      };

/*  Object types:  */
#define	VM_OBJECT_FILE			1
#define	VM_OBJECT_ANONYMOUS		2
#define	VM_OBJECT_SHADOW		3

/*  Object flags (ORed):  */
#define	VM_OBJECT_EXECUTABLEFILE	1


/*
 *  vm_region
 *  ---------
 *
 *  Each process has a chain of these. Each one represents a region of
 *  virtual memory, such as the TEXT, DATA, SHARED LIBRARY regions and
 *  a few others.
 *
 *  Regions are NOT shared between processes, but the vm_objects they
 *  point to are often shared.
 */

struct vm_region
      {
	struct vm_region	*next;
	struct vm_region	*prev;

	size_t			start_addr;	/*  for example: 0x30000  */
	size_t			end_addr;	/*  for example: 0x3ffff  */

	u_int32_t		type;

	struct vm_object	*source;	/*  data source (vm_object)  */
	off_t			srcoffset;	/*  offset into source,
						    nr of bytes  */
      };


#define	VMREGION_READABLE		1
#define	VMREGION_WRITABLE		2
#define	VMREGION_EXECUTABLE		4
#define	VMREGION_COW			8
#define	VMREGION_STACK			16



/*
 *  vm functions:
 */

void vm_init ();

int vm_region_attach (struct proc *p, struct vm_object *obj, off_t objoffset,
        size_t start_addr, size_t end_addr, u_int32_t type);
int vm_region_detach (struct proc *p, size_t start_addr);
void *vm_region_findfree (struct proc *p, size_t len);

struct vm_object *vm_object_create (int type);
int vm_object_freepages (struct vm_object *obj, size_t firstpage, size_t lastpage);
int vm_object_free (struct vm_object *obj);
int vm_object_combine (struct vm_object *subobj, struct vm_object *newobj);

int vm_fork (struct proc *p, struct proc *child_proc);

void vm_fault (struct proc *p, size_t virtualaddr, int action);

#define	VM_PAGEFAULT_READ		1
#define	VM_PAGEFAULT_WRITE		2
#define	VM_PAGEFAULT_NOTPRESENT		4

int vm_prot_accessible (struct proc *p, void *addr, size_t len, int accessflag);
size_t vm_prot_maxmemrange (struct proc *p, void *addr, int accessflag);
size_t vm_prot_stringlen (struct proc *p, byte *addr, size_t buflen);


#endif	/*  __SYS__VM_H  */

