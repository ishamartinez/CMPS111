// Isha Aggarwal

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
#include <syscall-nr.h>
#include <list.h>
//#include <syscall.h>
#include "devices/shutdown.h"
#include "devices/input.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "filesys/inode.h"
#include "filesys/directory.h"
#include "threads/palloc.h"
#include "threads/malloc.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/syscall.h"
#include "userprog/process.h"
#include "userprog/umem.h"

static void syscall_handler(struct intr_frame *);

static void write_handler(struct intr_frame *);
static void exit_handler(struct intr_frame *);
static void create_handler(struct intr_frame *); 
static void open_handler(struct intr_frame *); 
static void read_handler(struct intr_frame *);

static void close_handler(struct intr_frame *);

void
syscall_init (void)
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  
}

static void
syscall_handler(struct intr_frame *f)
{
  int syscall;
  ASSERT( sizeof(syscall) == 4 ); // assuming x86

  // The system call number is in the 32-bit word at the caller's stack pointer.
  umem_read(f->esp, &syscall, sizeof(syscall));

  // Store the stack pointer esp, which is needed in the page fault handler.
  // Do NOT remove this line
  thread_current()->current_esp = f->esp;

  switch (syscall) {
  case SYS_HALT: 
    shutdown_power_off();
    break;

  case SYS_EXIT: 
    exit_handler(f);
    break;
      
  case SYS_WRITE: 
    write_handler(f);
    break;
    
  case SYS_CREATE: 
    create_handler(f);
    break;
    
  case SYS_OPEN: 
    open_handler(f);
    break;
    
  case SYS_READ: 
    read_handler(f);
    break;
    
  case SYS_CLOSE: 
    close_handler(f);
    break;
    
  default:
    printf("[ERROR] system call %d is unimplemented!\n", syscall);
    thread_exit();
    break;
  }
}



void sys_exit(int status) 
{
  printf("%s: exit(%d)\n", thread_current()->name, status);
  thread_exit();
}

static void exit_handler(struct intr_frame *f) 
{
  int exitcode;
  umem_read(f->esp + 4, &exitcode, sizeof(exitcode));

  sys_exit(exitcode);
}

/*
 * BUFFER+0 and BUFFER+size should be valid user adresses
 */
static uint32_t sys_write(int fd, const void *buffer, unsigned size)
{
  umem_check((const uint8_t*) buffer);
  umem_check((const uint8_t*) buffer + size - 1);

  int ret = -1;

  if (fd == 1) { // write to stdout
    putbuf(buffer, size);
    ret = size;
  }

  return (uint32_t) ret;
}

static void write_handler(struct intr_frame *f)
{
    int fd;
    const void *buffer;
    unsigned size;

    umem_read(f->esp + 4, &fd, sizeof(fd));
    umem_read(f->esp + 8, &buffer, sizeof(buffer));
    umem_read(f->esp + 12, &size, sizeof(size));
 
    f->eax = sys_write(fd, buffer, size);
}

static void create_handler(struct intr_frame *f)
{
   // bool isFile;
    char *cmdString = *(char **) (f->esp+4);
    unsigned sizeCount = *(int*) (f->esp +8);
    f->eax = filesys_create(cmdString, sizeCount, false);
}



static void open_handler(struct intr_frame *f)
{
    int fileDescp = -1;
    char *cmdString;// = *(char **)(f->esp +4);
     umem_read(f->esp + 4, &cmdString, sizeof(cmdString));
     
    struct file *file = filesys_open(cmdString); 
    if ( file == NULL)
        f->eax = fileDescp;
    else
    {
        struct thread *current = thread_current(); 
        fileDescp = current->nextDesc++; 
        //list_push_front(&current->fileDesc, &fileD)
        f->eax = fileDescp; 
    
    }
}


static void read_handler(struct intr_frame *f)
{
   // struct file *file= filesys_open(*(char**)(f->esp+4));
    
    int fd;// = (int)(f->esp + 4);
    void *buffer;// = *(char **)(f->esp + 8);
    unsigned size;// = *(unsigned *)(f->esp + 12);
    
    umem_read(f->esp + 4, &fd, sizeof(fd));
    umem_read(f->esp + 8, &buffer, sizeof(buffer));
    umem_read(f->esp + 12, &size, sizeof(size));
    
    umem_check((const uint8_t*) buffer);
    umem_check((const uint8_t*) buffer + size - 1);
    //struct file_elem *file_elem = get_file_elem(fd);
    
    if(get_file_elem(fd) == NULL)
        f->eax = -1;
    else{
        struct file_elem *file_elem = get_file_elem(fd);
        f->eax = file_read(file_elem->file, buffer, size);
    }

    
}

struct file_elem* get_file_elem(int fd){
    
    struct list_elem *e;
    struct list *file_list = &thread_current()->fileDesc; //something like that
    
    for(e = list_begin(file_list); e != list_end(file_list); e = list_next(e)){
        struct file_elem *current = list_entry(e, struct file_elem, elem);
        //printf("\n current->fd: %d\n fd: %d", current->fd, fd);
        if(current->fd == fd)
            return current;
    }
    return 0;// NULL;
}

static void close_handler(struct intr_frame *f)
{

    
    if(get_file_elem(*(int *)(f->esp + 4)) != NULL){
        printf("\nhello\n");
    }
}