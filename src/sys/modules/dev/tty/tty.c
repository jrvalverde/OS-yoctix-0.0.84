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
 *  modules/dev/tty/tty.c
 *
 *	/dev/tty   --   an alias for the terminal at which the user is
 *			connected to a process
 *
 *	TODO:  actually write this module. The only reason it's here
 *	is to make "tty" show up in devfs, but it doesn't do anything yet.
 *
 *  History:
 *	29 Jul 2000	first version
 */



#include "../../../config.h"
#include <sys/std.h>
#include <string.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/proc.h>
#include <sys/module.h>



struct module *tty_m;
struct device *tty_d;



int tty_open (struct device *dev, struct proc *p)
  {
    return 0;
  }



int tty_close (struct device *dev, struct proc *p)
  {
    return 0;
  }



int tty_read (off_t *res, struct device *dev, byte *buf, off_t buflen, struct proc *p)
  {
    return EINVAL;
  }



int tty_write (off_t *res, struct device *dev, byte *buf, off_t buflen, struct proc *p)
  {
    return EINVAL;
  }



int tty_ioctl (struct device *dev, int *res, struct proc *p, unsigned long req, unsigned long arg1)
  {
    return EINVAL;
  }



void tty_init (int arg)
  {
    tty_m = module_register ("virtual", 0, "tty", "/dev/tty device driver");
    if (!tty_m)
	return;

    tty_d = device_alloc ("tty", "tty", DEVICETYPE_CHAR, 0666, 0, 0, 0);
    if (!tty_d)
      {
	module_unregister (tty_m);
	return;
      }

    tty_d->open  = tty_open;
    tty_d->close = tty_close;
    tty_d->read  = tty_read;
    tty_d->write = tty_write;

    if (device_register (tty_d))
      {
	printk ("tty: Problem registering tty device");
	device_free (tty_d);
	module_unregister (tty_m);
	return;
      }
  }


