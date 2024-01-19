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
 *  modules/dev/null/null.c
 *
 *	The /dev/null driver
 *
 *	TODO:  actually write this module. The only reason it's here
 *	is to make "null" show up in devfs, but it doesn't do anything yet.
 *
 *  History:
 *	14 Jan 2000	first version
 */



#include "../../../config.h"
#include <sys/std.h>
#include <string.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/module.h>



struct module *null_m;
struct device *null_d;



int null_open ()
  {
    null_d->refcount ++;

    return 0;
  }


int null_close ()
  {
    null_d->refcount --;

    return 0;
  }


int null_read ()
  {

    return ENODEV;
  }


int null_write ()
  {

    return ENODEV;
  }



void null_init (int arg)
  {
    null_m = module_register ("virtual", 0, "null", "Null device driver");

    if (!null_m)
	return;

    null_d = device_alloc ("null", "null", DEVICETYPE_CHAR, 0666, 0, 0, 0);
    if (!null_d)
      {
        module_unregister (null_m);
        return;
      }
  
    null_d->open  = null_open;
    null_d->close = null_close;
    null_d->read  = null_read;
    null_d->write = null_write;

    if (device_register (null_d))
      {
        printk ("null: Problem registering null device");
        device_free (null_d);
        module_unregister (null_m);
        return;
      }
  }


