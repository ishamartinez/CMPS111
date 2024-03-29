// Isha Aggarwal
// iaggarwa@ucsc.edu
// CMPS 111 Lab 2
// 12 Feb 2018


/* 
 * This file is derived from source code for the Pintos
 * instructional operating system which is itself derived
 * from the Nachos instructional operating system. The 
 * Nachos copyright notice is reproduced in full below. 
 *
 * Copyright (C) 1992-1996 The Regents of the University of California.
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for any purpose, without fee, and
 * without written agreement is hereby granted, provided that the
 * above copyright notice and the following two paragraphs appear
 * in all copies of this software.
 *
 * IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO
 * ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR
 * CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OF THIS SOFTWARE
 * AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF CALIFORNIA
 * HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS"
 * BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
 * MODIFICATIONS.
 *
 * Modifications Copyright (C) 2017 David C. Harrison. All rights reserved.
 */

#include <stdio.h>
#include <string.h>

#include "threads/semaphore.h"
#include "threads/interrupt.h"
#include "threads/thread.h"

/* 
 * Initializes semaphore SEMA to VALUE.  A semaphore is a
 * nonnegative integer along with two atomic operators for
 * manipulating it:
 *
 * - down or Dijkstra's "P": wait for the value to become positive, 
 * 	then decrement it.
 * - up or Dijkstra's "V": increment the value(and wake up one waiting 
 * 	thread, if any). 
 */
void
semaphore_init(struct semaphore *sema, unsigned value)
{
    ASSERT(sema != NULL);

    sema->value = value;
    list_init(&sema->waiters);
}

/* 
 * Down or Dijkstra's "P" operation on a semaphore.  Waits for SEMA's 
 * value to become positive and then atomically decrements it.
 *
 * This function may sleep, so it must not be called within an
 * interrupt handler.  This function may be called with
 * interrupts disabled, but if it sleeps then the next scheduled
 * thread will probably turn interrupts back on. 
 */
void
semaphore_down(struct semaphore *sema)
{
    ASSERT(sema != NULL);
    ASSERT(!intr_context());

    enum intr_level prev = intr_disable();
    while (sema->value == 0) 
    {
        donate_priority();
        list_insert_ordered(&sema->waiters, &thread_current()->elem, &is_greater_priority, NULL);
        thread_block();
    }
    sema->value--;
    intr_set_level(prev);
}

/* 
 * Down or Dijkstra's "P" operation on a semaphore, but only if the
 * semaphore is not already 0.  Returns true if the semaphore is
 * decremented, false otherwise.
 *
 * This function may be called from an interrupt handler. 
 */
bool
semaphore_try_down(struct semaphore *semaphore)
{
    ASSERT(semaphore != NULL);

    bool success = false;
    enum intr_level prev = intr_disable();
    if (semaphore->value > 0) {
        semaphore->value--;
        success = true;
    } else {
        success = false;
    }
    intr_set_level(prev);

    return success;
}

/* 
 * Up or Dijkstra's "V" operation on a semaphore.  Increments SEMA's value
 * and wakes up one thread of those waiting for SEMA, if any.
 *
 * This function may be called from an interrupt handler. 
 */
void
semaphore_up(struct semaphore *semaphore)
{
    enum intr_level prev;
    bool should_it_yield = false; 
    ASSERT(semaphore != NULL);
   
    prev = intr_disable();

    if (!list_empty(&semaphore->waiters)) 
    {
        list_sort(&semaphore->waiters, &is_greater_priority, NULL);
        struct thread *temporary = list_entry(list_pop_front(&semaphore->waiters), struct thread, elem);
        thread_unblock(temporary);
        // we saved the thread in checkThis, now unblock and determine the relative priority to current thread
        if(thread_current()->priority < temporary-> priority)
        {
            should_it_yield = true; 
        }
    }
    semaphore->value++;
    intr_set_level(prev);
    if(should_it_yield)
    {
        thread_yield();
    }
    
}

