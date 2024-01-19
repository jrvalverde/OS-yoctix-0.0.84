/*
 *  config.h  --  Yoctix kernel configuration
 *  -----------------------------------------
 *
 *  Edit this file before compiling the kernel. You should also make the
 *  following symbolic links:
 *
 *	include/sys/md	-->  include/sys/arch/XXX
 *	md		-->  arch/XXX
 *
 *  where XXX is your architecture, for example i386.
 *
 *  NOTE:  There are two other configuration files of interest! There should
 *  be a file called "md/config.h" which contains machine dependant config
 *  options, and "modules/CONFIG" which lists the modules that will be
 *  statically linked into the kernel image.
 */


#include "md/config.h"


/*
 *  debuglevel
 *  ----------
 *
 *  0 = off, 1 = minimal, 2+ = more
 *  -1 disables printk() except for the first line of text
 */

#define	DEBUGLEVEL		3


/*
 *  malloc() out-of-memory behaviour
 *  --------------------------------
 *
 *  If malloc() ever is unable to return a pointer to the requested amount
 *  of memory to a caller, should we panic() then or should we return NULL?
 */

/*  #define	MALLOC_PANIC  */


/*
 *  Built-in kernel debugger
 *  ------------------------
 */

#define	KDB
#define	KDB_ON_PANIC

