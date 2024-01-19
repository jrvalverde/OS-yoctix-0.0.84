/*
 *  Copyright (C) 1999 by Anders Gavare.  All rights reserved.
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
 *  sys/module.h  --  module handler
 */

#ifndef	__SYS__MODULE_H
#define	__SYS__MODULE_H

#include <sys/defs.h>


#define	MODULE_SHORTNAMELEN	15
#define	MODULE_LONGNAMELEN	79


struct module
    {
	struct module *next;				/*  Ptr to next in this chain  */
	struct module *parent;				/*  Ptr to parent module  */
	struct module *children;			/*  Ptr to first child module  */

	/*
	 *  type is the "sum" of MODULETYPE_* constants ORed together,
	 *  name is the long name of the module,
	 *  shortname is the short name. NOTE: shortname includes
	 *  the unitnumber in ascii if the module is numbered!
	 */

	int		type;
	char		name [MODULE_LONGNAMELEN+1];
	char		shortname [MODULE_SHORTNAMELEN+1];
	int		unitnumber;


	/*  Info for loadable modules:  */
	/*  TODO  */
    };


/*  Bits in module_t->type:  (should be ORed together)  */
#define	MODULETYPE_SYSTEM		1
#define	MODULETYPE_BUILTIN		2
#define	MODULETYPE_NUMBERED		4


/*  Initialize the module handler:  */
void module_init ();

/*  Add a module (specifying the parent with a module pointer...):  */
struct module *module_register_raw (struct module *parent, int type, char *shortname, char *name);

/*  Add a module:  */
struct module *module_register (char *parent, int type, char *shortname, char *name);

/*  Tries to unregister a module:  */
int module_unregister (struct module *);

/*  Print "module at parent" string to a buffer. Returns 1 on success, and buf contains
    the text...  returns 0 on failure, and buf is undefined.  */
int module_nametobuf (struct module *m, char *buf, int buflen);

/*  Debug dump of currently loaded modules:  */
void module_dump ();



/*
 *  Statically linked modules:
 */

/*  staticmodules_init() is created using information in modules/CONFIG  */
void staticmodules_init ();



/*
 *  Argument passed to XXX_init() of each module:
 *
 *	MODULE_PROBE only probes (for hardware devices etc),
 *	displays status (if any) and then exits (returns 0).
 *
 *	MODULE_INIT probes for devices etc and returns 1.
 *
 *	MODULE_FORCE_INIT should cause the module to return 1
 *	nomatter if it succeeded its probing or not.
 *
 *  XXX_init() should return 1 if the call was successful,
 *  0 otherwise. It still has to "obey" the argument, though.
 */

#define	MODULE_PROBE		1
#define	MODULE_INIT		2
#define	MODULE_FORCE_INIT	3



#endif	/*  __SYS__MODULE_H  */

