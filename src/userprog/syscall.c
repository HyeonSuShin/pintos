#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "devices/shutdown.h"

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  printf ("system call!\n");
  thread_exit ();
}

void sys_halt (void)
{
  shutdown_power_off();
}

void sys_exit(int status)
{
  // have to add 'status' to thread structure
  //thread_current()->status = status;

  printf("%s: exit(%d)\n", thread_current()->name, status);
  thread_exit();
}

pid_t sys_exec(const char *cmd_line)
{
  char *args;
  pid_t pid;
  // function parsing cmd_line
  
  // thread_create(args[])

  return pid;
}

int sys_wait (pid_t pid)
{
  struct thread *parent;
  struct thread *child;
  // add 'child_list' to thread structure
  if (!(child = thread_get_child(pid)))
    return -1;
  
  sema_down(parent->wait); // add 'wait' semaphore to thread structures  
  list_remove(child->child_elem); // add 'child_elem'

  return child->status;
}