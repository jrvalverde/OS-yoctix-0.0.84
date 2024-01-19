/*
 *  Copyright (C) 1999,2000 by Anders Gavare.  All rights reserved.
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
 *  reg/module.c  --  module handler
 *
 *	Modules, both statically linked and dynamically loaded, must register
 *	themselves by calling module_register().
 *
 *
 *	module_init()
 *		Initialize the module handling system
 *
 *	module_register_raw()
 *		Register a module (parent given by address)
 *
 *	module_register()
 *		Register a module (parent given by name)
 *
 *	module_unregister()
 *		Tries to unregister a module
 *
 *	module_nametobuf()
 *		Puts a string such as "module0 at parent0" in
 *		a buffer provided by the caller.
 *
 *	module_dump()
 *		Dump a tree of all current modules to the console
 *
 *  History:
 *	20 Dec 1999	first version
 *	6 Jan 2000	autonumbering of numbered modules
 */


#include "../config.h"
#include <sys/module.h>
#include <sys/std.h>
#include <sys/malloc.h>
#include <string.h>
#include <stdio.h>
#include <sys/interrupts.h>



struct module *root_module = NULL;


struct module *module_register__Findmodulebyname (char *, struct module *);


void module_init ()
  {
    /*
     *	Initialize the module handler:
     *
     *	       root
     *		|
     *		+--  virtual  (Virtual modules)
     *		|
     *		\--  mainbus  (Hardware dependant modules)
     *
     *	Note that 'mainbus' will actually be called 'mainbus0'.
     */

    module_register_raw (NULL, MODULETYPE_SYSTEM | MODULETYPE_BUILTIN,
	"root", "root");

    if (!root_module)
	panic ("module_init() could not register the module handler");

    module_register ("root", MODULETYPE_SYSTEM | MODULETYPE_BUILTIN,
	"virtual", "Virtual modules");

    module_register ("root", MODULETYPE_SYSTEM | MODULETYPE_BUILTIN
	| MODULETYPE_NUMBERED, "mainbus", "Hardware dependant modules");
  }



struct module *module_register_raw (struct module *parent, int type, char *shortname, char *name)
  {
    /*
     *	Register a new module (called 'shortname') at the parent pointed
     *	to by 'parent', by doing the following:
     *
     *	1) Allocate memory for a new module struct
     *	2) Fill in the data
     *	3) Add it to the parent's children-chain
     */

    struct module *p, *tmp;
    int nr, found;
    char namebuf [MODULE_SHORTNAMELEN+20];
    int oldints;

    oldints = interrupts (DISABLE);

    if (parent == NULL  &&  root_module != NULL)
      {
	interrupts (oldints);
	printk ("module_register_raw(): trying to re-register root module");
	return NULL;
      }

    p = (struct module *) malloc (sizeof(struct module));
    if (!p)
      {
	interrupts (oldints);
	printk ("module_register_raw(): out of kernel memory");
	return NULL;
      }

    /*  Fill in the data:  */
    p->type = type;
    p->next = NULL;
    p->parent = parent;
    p->children = NULL;
    strlcpy (p->shortname, shortname, MODULE_SHORTNAMELEN+1);
    strlcpy (p->name, name, MODULE_LONGNAMELEN+1);


    /*
     *	If the module should be numbered, then let's find the first instance
     *	of this shortname which is not yet in use.  For example, if fdc0 was in
     *	use and we're trying to register fdc, but fdc1 was free, then we call
     *	this module fdc1 even if there are fdc2, fdc3 or whatever...
     */

    if (type & MODULETYPE_NUMBERED)
      {
	nr = 0;
	found = -1;
	while (found == -1)
	  {
	    snprintf (namebuf, MODULE_SHORTNAMELEN+20, "%s%i", shortname, nr);
	    if (module_register__Findmodulebyname (namebuf, root_module))
		nr++;
	    else
		found = nr;
	  }
	strlcpy (p->shortname, namebuf, MODULE_SHORTNAMELEN+1);
      }


    /*  Add to the parent's children-chain:  (add last)  */
    if (parent == NULL)
	root_module = p;	/*  the root module is the first one we register  */
    else
      {
	if (parent->children == NULL)
		parent->children = p;
	else
	  {
	    /*  find the last child of the parent, and add our selves:  */
	    tmp = parent->children;
	    while (tmp->next)
		tmp = tmp->next;
	    tmp->next = p;
	  }
      }

    interrupts (oldints);
    return p;
  }



struct module *module_register__Findmodulebyname (char *name, struct module *start)
  {
    /*
     *	Returns a pointer to a module whose shortname is "name".
     *	Any unitnumber should be included in the name.
     */

    struct module *p, *res;

    if (!start)
	return NULL;

    if (!strcmp(name, start->shortname))
	return start;

    /*  Recurse through all the children:  */
    p = start->children;
    while (p)
      {
	res = module_register__Findmodulebyname (name, p);
	if (res)
		return res;

	p = p->next;
      }

    return NULL;
  }



struct module *module_register (char *parent, int type, char *shortname, char *name)
  {
    /*
     *	Register a module called 'shortname' under a parent called 'parent'.
     *
     *	If there is no root_module, it should be registered directly using
     *	module_register_raw()! (Should only be done once, in module_init().)
     *
     *	Try to find a pointer to the 'parent' module. Begin at the root
     *	module. Then use module_register_raw() to actually register the module.
     */

    struct module *p, *parent_ptr;
    int oldints;


    if (!parent || !shortname || !name)
      {
	printk ("module_register(): invalid parameters");
	return NULL;
      }

    oldints = interrupts (DISABLE);

    if (!root_module)
	panic ("module_register(): no root_module");

    parent_ptr = module_register__Findmodulebyname (parent, root_module);

    if (!parent_ptr)
      {
	interrupts (oldints);
	printk ("module_register(): trying to register %s at non-existant %s",
		shortname, parent);
	return NULL;
      }

    p = module_register_raw (parent_ptr, type, shortname, name);

    interrupts (oldints);
    return p;
  }



int module_unregister (struct module *m)
  {
    /*
     *	Try to unregister a module.  Returns 1 on success, 0 on failure.
     *	Fails when the module has children, or if m is the root module.
     */

    struct module *p, *lastp, *parent;
    int oldints;

    oldints = interrupts (DISABLE);

    if (m == NULL || m == root_module || m->children)
      {
	interrupts (oldints);
	return 0;
      }

    /*  Go though the parent's list of children:  */
    parent = m->parent;
    p = parent->children;

    if (p==m)
      {
	parent->children = m->next;
	interrupts (oldints);
	free (m);
	return 1;
      }

    lastp = p;
    p = p->next;

    while (p)
      {
	if (p==m)
	  {
	    lastp->next = m->next;
	    interrupts (oldints);
	    free (m);
	    return 1;
	  }

	lastp = p;
	p = p->next;
      }

    interrupts (oldints);
    return 0;
  }



int module_nametobuf (struct module *m, char *buf, int buflen)
  {
    /*
     *	Puts "modulename at parentname\0" in a buffer provided by
     *	the caller. Returns 0 on failure, 1 on success.
     */

    struct module *parent;
    int len;

    if (!m)
	return 0;

    parent = m->parent;
    if (!parent)
	return 0;

    len = strlen (m->shortname) + strlen(parent->shortname) + 6;

    if (buflen < len)
	return 0;

    buf[0] = 0;
    snprintf (buf, len, "%s at %s", m->shortname, parent->shortname);

    return 1;
  }



void module_dump__Internal (struct module *m, int spaces)
  {
    /*
     *	Print the name of the module m, and recurse for all children.
     */

    struct module *tmpptr;
    char *buf;
    int len, i;

    len = spaces+strlen(m->name)+strlen(m->shortname)+10;
    buf = (char *) malloc (len);
    for (i=0; i<spaces; i++)
	buf[i] = ' ';

    snprintf (buf+spaces, len-spaces-1, "%s (%s)", m->shortname, m->name);

    printk ("%s", buf);

    tmpptr = m->children;
    while (tmpptr)
      {
	module_dump__Internal (tmpptr, spaces+2);
	tmpptr = tmpptr->next;
      }

    free (buf);
  }



void module_dump ()
  {
    /*
     *	Dump a list of all modules (for debugging purposes)
     */

    struct module *m;
    int oldints;

    printk ("module_dump():");

    oldints = interrupts (DISABLE);
    m = root_module;
    if (!m)
      {
	interrupts (oldints);
	printk ("No root module.");
	return;
      }

    module_dump__Internal (m, 2);
    interrupts (oldints);
  }

