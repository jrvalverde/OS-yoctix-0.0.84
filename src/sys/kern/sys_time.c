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
 *  kern/sys_time.c  --  time related syscalls
 *
 *
 *  History:
 *	24 Jul 2000	sys_gettimeofday()
 */


#include "../config.h"
#include <string.h>
#include <sys/std.h>
#include <sys/proc.h>
#include <sys/errno.h>
#include <sys/syscalls.h>
#include <sys/time.h>
#include <sys/vm.h>


extern size_t userland_startaddr;
extern volatile struct timespec system_time;



int sys_gettimeofday (ret_t *res, struct proc *p, struct timeval *tp,
	void *timezoneptr)
  {
    /*
     *	sys_gettimeofday ()
     *	-------------------
     *
     *	Return the contents of the system_time variable.
     */

    int return_tp=1, return_tz=1;

    if (!res || !p)
	return EINVAL;

    if (!tp)
      return_tp = 0;

    if (!timezoneptr)
      return_tz = 0;

    if (!p->sc_params_ok)
      {
	if (return_tp)
	  if (!vm_prot_accessible (p, tp, sizeof(struct timeval),
				VMREGION_WRITABLE))
	    return EFAULT;

/*  TODO: timezone size!!! */
/*
	if (return_tz)
	  if (!vm_prot_accessible (p, timezoneptr, sizeof(int),
				VMREGION_WRITABLE))
	    return EFAULT;
*/
	p->sc_params_ok = 1;
      }


    if (return_tp)
      {
	tp = (struct timeval *) ((byte *)tp + userland_startaddr);
	tp->tv_sec = system_time.tv_sec;
	tp->tv_usec = system_time.tv_nsec / 1000;
      }

/*  TODO: handle the timezone stuff; at least it should be zeroed  */

    return 0;
  }

