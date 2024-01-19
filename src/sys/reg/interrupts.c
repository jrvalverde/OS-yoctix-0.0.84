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
 *  reg/interrupts.c  --  machine independant interrupt handler
 *
 *	Modules which need to service interrupts should register with
 *	irq_register().  The irq_dispatcher() is called from machine
 *	dependant assembly language code each time a hardware interrupt
 *	occurs.
 *
 *	irq_init()
 *		Called from kern/init_main.c, initializes the machine
 *		independant interrupt dispatcher.
 *
 *	irq_dispatcher()
 *		Calls the correct irq handler.
 *
 *	irq_register()
 *		Register an irq handler (also known as an interrupt
 *		service routine, ISR).
 *
 *	irq_unregister()
 *		Unregister an irq handler.
 *
 *	interrupts()
 *		Turns on or off interrupts (should be used only in
 *		critical code sections).
 *
 *
 *  History:
 *	5 Jan 2000	first version, irq_dispatcher()
 *	28 May 2000	removing some obsolete code which was only
 *			used during kernel initialisation
 */


#include "../config.h"
#include <stdio.h>
#include <sys/std.h>
#include <sys/interrupts.h>
#include <sys/proc.h>



extern volatile int need_to_pswitch;


void		*irq_handler [MAX_IRQS];
char		*irq_handler_name [MAX_IRQS];
u_int64_t	irq_statistics [MAX_IRQS];

int		interrupts_state = DISABLE;



void irq_init ()
  {
    int i;

    for (i=0; i<MAX_IRQS; i++)
      {
	irq_handler [i] = NULL;
	irq_handler_name [i] = NULL;
	irq_statistics [i] = 0;
      }
  }



void irq_dispatcher (int irqnr)
  {
    /*
     *	irq_dispatcher()  --  The general (hardware) interrupt dispatcher
     *
     *	Whenever a "module" registers itself, it should also register any
     *	hardware interrupt service routines it has. This function is called
     *	from low-level machine dependant code with irqnr set to the ID number
     *	of the interrupt. Getting a more detailed identification of what caused
     *	the interrupt is left to the interrupt service routine, the "handler"
     *	function.
     *
     *	Since handling of an interrupt may cause one or more sleeping processes
     *	to be awaken, we check the need_to_pswitch variable to see if a process
     *	switch should take place, before returning to the interrupted process.
     *
     *	TODO:  ack. of the interrupts need to be handled by the handler()  ???
     *		right now they're handled by md code in md/interrupts.S, which
     *		acks interrupts AFTER irq_dispatcher() has returned, but it
     *		will obviously not do so if we do pswitch()....   needs to be
     *		fixed!
     */

    void (*handler)();

    irq_statistics [irqnr] ++;

    handler = (void *) irq_handler [irqnr];
    if (!handler)
      {
	printk ("irq_dispatcher(): irq %i not registered", irqnr);
	return;
      }

    handler ();

    if (need_to_pswitch)
	pswitch ();
  }



int irq_register (int irqnr, void *handler, char *handlername)
  {
    /*
     *	irq_register ()
     *	---------------
     *
     *	Try to register an interrupt handler.  Interrupts should be disabled
     *	temporarily while irq_handler[] and irq_handler_name[] are being
     *	updated.  The irq_statistics entry is set to 0. This is because if
     *	for example a module using an interrupt is unregistered (unloaded)
     *	so that another module may use that interrupt, then the count should
     *	not include interrupts that occured while the first module was
     *	handling the interrupt.
     *
     *	Returns 1 on success, 0 on failure.
     */

    int oldints;

    if (irqnr < 0 || irqnr >= MAX_IRQS)
	return 0;

    oldints = interrupts (DISABLE);

    /*  irqnr already in use?  */
    if (irq_handler[irqnr])
      {
	interrupts (oldints);
	return 0;
      }

    irq_handler [irqnr]      = handler;
    irq_handler_name [irqnr] = handlername;
    irq_statistics [irqnr]   = 0;

    interrupts (oldints);
    return 1;
  }



int irq_unregister (int irqnr)
  {
    /*
     *	irq_unregister ()
     *	-----------------
     *
     *	Unregister an interrupt handler.
     *  Returns 1 on success, 0 on failure.
     */

    int oldints;

    if (irqnr < 0 || irqnr >= MAX_IRQS)
	return 0;

    oldints = interrupts (DISABLE);

    if (!irq_handler[irqnr])
      {
	interrupts (oldints);
	return 0;
      }

    irq_handler [irqnr] = 0;
    irq_handler_name [irqnr] = NULL;

    interrupts (oldints);
    return 1;
  }



int interrupts (int state)
  {
    /*
     *	Enable or disable interrupts.  Return value is the previous
     *	interrupt state.
     */

    int old_interrupts_state = machdep_oldinterrupts();

    machdep_interrupts (state);
    interrupts_state = state;

    return old_interrupts_state;
  }

