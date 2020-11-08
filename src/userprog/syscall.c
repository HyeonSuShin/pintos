#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "devices/shutdown.h"
#include "filesys/filesys.h"
#include "userprog/process.h"

static void syscall_handler (struct intr_frame *);
struct lock *file_lock;

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
  thread_current()->exit_status = status;
  printf("%s: exit(%d)\n", thread_current()->name, status);
  process_exit();
  thread_exit();
}

pid_t sys_exec(const char *cmd_line)
{
  pid_t pid;
  struct thread *child;

  pid = process_execute(cmd_line);
  if (pid == -1)
    return -1;
  
  child = thread_get_child(pid);
  if (!child)
    return -1;
  
  sema_down(child->load); //sema_up in start_process 
  // if load fail, thread exit and return -1

  return pid;
}

int sys_wait (pid_t pid)
{
  return process_wait(pid);
}

bool sys_create (const char *file, unsigned initial_size)
{
  return filesys_create(file, initial_size);
}

bool sys_remove (const char *file)
{
  return filesys_remove(file);
}

int sys_open (const char *file)
{
  struct file *file_ptr;
  int fd;

  lock_acquire(file_lock);
  
  file_ptr = filesys_open(file);
  if (!file_ptr)
    return -1;
  fd = add_file(file_ptr);

  lock_release(file_lock);

  return fd;
}