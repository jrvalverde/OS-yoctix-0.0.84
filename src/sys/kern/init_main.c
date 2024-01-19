/*
 *  Copyright (C) 1999,2000,2001 by Anders Gavare.  All rights reserved.
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
 *  kern/init_main.c  --  "Where the kernel begins"
 *
 *	init_main() is called by machine dependant code as soon as the stack
 *	is setup and we are running in a "flat" or "raw" mode (physical memory
 *	= virtual memory).
 *
 *	What init_main() should do:
 *
 *		*  Initialize a "simple console" and print boot message
 *		*  Call machine dependant machdep_main_startup()
 *			(on i386: set up GDT, IDT etc)
 *		*  Initialize machine independant resources
 *		*  Initialize machine dependant resources
 *			(on i386: indentify CPU, BIOS etc)
 *		*  Initialize statically linked modules
 *		*  Create process 1
 *		*  Mount the root filesystem
 *		*  Execute /sbin/init
 *
 *
 *  History:
 *	18 Oct 1999	first version
 *	21 Oct 1999	minor changes
 *	22 Oct 1999	minor changes (refreshing)
 *	24 Nov 1999	should create process 1 manually
 *	20 Dec 1999	calls module_init()
 *	27 Dec 1999	calls proc_init(), vm_init(), vfs_init()
 *	28 Dec 1999	calls timer_init()
 *	13 Jan 2000	trying to get tasks to run
 *	14 Jan 2000	calls device_init()
 *	26 Jul 2000	creates proc1 before mounting root
 *	19 Oct 2000	opens /dev/console instead of /dev/ttyC0
 */


#include "../config.h"
#include <stdio.h>
#include <string.h>
#include <sys/os.h>
#include <sys/std.h>
#include <sys/md/machdep.h>
#include <sys/console.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/module.h>
#include <sys/proc.h>
#include <sys/md/proc.h>
#include <sys/vm.h>
#include <sys/vfs.h>
#include <sys/syscalls.h>
#include <sys/ports.h>
#include <sys/interrupts.h>
#include <sys/dma.h>
#include <sys/timer.h>
#include <sys/time.h>
#include <fcntl.h>


extern volatile struct timespec system_time;
extern volatile struct proc *curproc;
extern size_t userland_startaddr;
extern char *compile_info, *compile_generation;

struct proc *proc1;
ret_t tmp_retval;
int res;
char nodename [MAX_NODENAME+1];

char *init_path = "/sbin/init";

char *init_args [2] =
      {  "init", NULL  };

char *init_env [4] =
      {
	"HOSTTYPE=" OS_ARCH,
	"OSTYPE=yoctix",
	"MACHTYPE=" OS_MACHTYPE,
	NULL
      };



int init_main ()
  {
    /*
     *	Initialize a simple console (we want to show debug
     *	messages while we initialize things...)
     *
     *	... and print boot message
     */

    console_init ();
    printk (OS_NAME"/"OS_ARCH" "OS_VERSION" %s %s",
		compile_generation, compile_info);


    /*
     *	Call machine dependant machdep_main_startup()
     */

    machdep_main_startup ();


    /*
     *	Initialize machine independant resources
     */

    strlcpy (nodename, "noname", MAX_NODENAME+1);

    malloc_init ();		/*  The kernel memory allocator		*/

    device_init();		/*  Device support			*/
    module_init();		/*  Module support			*/
    proc_init();		/*  Process handling			*/
    vm_init();			/*  Virtual Memory System		*/
    syscall_init();		/*  Syscall/exec handling		*/

    ports_init();		/*  I/O ports registry			*/
    irq_init();			/*  Hardware interrupt handlers reg.	*/
    dma_init();			/*  DMA registry			*/

    timer_init();		/*  Timers				*/
    vfs_init();			/*  Virtual File System			*/

    interrupts (ENABLE);


    /*
     *	Initialize machine dependant resources
     */

    machdep_res_init ();

    interrupts (DISABLE);
    srandom ((system_time.tv_sec << 32) + system_time.tv_nsec);
    interrupts (ENABLE);


    /*
     *	Initialize statically linked modules
     */

    staticmodules_init ();


    /*
     *	Create process 1
     *	(Only pids ranging from PID_MIN are represented in the pidbitmap,
     *	so we don't add proc 1 to the map.)
     */

    proc1 = proc_alloc ();
    proc1->pid = 1;
    proc1->ppid = 0;
    proc1->next = proc1;
    proc1->prev = proc1;


    /*
     *	Mount the root filesystem
     */

    vfs_mount (proc1, "wd0d", "/", "ffs", VFSMOUNT_RW | VFSMOUNT_ASYNC);
    vfs_mount (proc1, "", "/dev", "devfs", VFSMOUNT_NODEV);
    vfs_mount (proc1, "", "/proc", "procfs", VFSMOUNT_NODEV);
    vfs_mount (proc1, "wd0a", "/wd0a", "ffs", 0);

/*
    vfs_mount (proc1, "fd0", "/", "ffs", VFSMOUNT_RW | VFSMOUNT_ASYNC);
    vfs_mount (proc1, "", "/dev", "devfs", VFSMOUNT_NODEV);
    vfs_mount (proc1, "", "/proc", "procfs", VFSMOUNT_NODEV);
    vfs_mount (proc1, "wd0a", "/mnt", "ffs", 0);
*/

    /*
     *	Execute /sbin/init
     *
     *	First we call sys_open() to open the console tty. We will get file
     *	descriptors number 0, 1, and 2, becase the process doesn't have any
     *	other open descriptors yet.
     */

    proc1->sc_params_ok = 1;

    res = sys_open (&tmp_retval, proc1, "/dev/console" - userland_startaddr, O_RDONLY, 0);
    if (res)
      panic ("could not open /dev/console for reading, error %i", res);

    res = sys_open (&tmp_retval, proc1, "/dev/console" - userland_startaddr, O_WRONLY, 0);
    if (res)
      panic ("could not open /dev/console for writing, error %i", res);

    res = sys_open (&tmp_retval, proc1, "/dev/console" - userland_startaddr, O_WRONLY, 0);
    if (res)
      panic ("could not open /dev/console for writing, error %i", res);

    res = sys_execve (&tmp_retval, proc1,
	(char *)  ((size_t)init_path - userland_startaddr),
	(char **) ((size_t)init_args - userland_startaddr),
	(char **) ((size_t)init_env  - userland_startaddr) );

    panic ("could not execute /sbin/init, result code %i", res);


    /*  not reached  */
    return 0;
  }

