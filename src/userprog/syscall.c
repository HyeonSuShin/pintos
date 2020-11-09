#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "userprog/process.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "devices/shutdown.h"
#include "devices/input.h"
#include "filesys/filesys.h"

static void syscall_handler (struct intr_frame *);
struct lock *file_lock;

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init(file_lock);
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  printf ("system call!\n");
  thread_exit ();
}

bool check_address (void *addr)
{
  if (addr >= 0x8048000 && addr <= 0xc0000000)
    return true;
  return false;
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

int sys_filesize (int fd)
{
  struct thread *cur = thread_current();
  struct file *file = cur->fd_table[fd];

  // if file doesn't exist, return -1
  if (!file)
    return -1;
  
  // return file size
  return file_length(file);
}

int sys_read (int fd, void *buffer, unsigned size)
{
  struct thread *cur = thread_current();
  struct file *file;
  lock_acquire(file_lock);

  // STDIN, store keyboard input in buffer
  if (fd == 0)
  {
    for (int i = 0; i < size; i++)
      *((char*)buffer++) = input_getc();
    lock_release(file_lock);
    return size;
  }

  // not STDIN, store file in buffer
  file = cur->fd_table[fd];  
  // if can't find file, return -1
  if (!file)
  {
    lock_release(file_lock);
    return -1;
  }

  size = file_read(file, buffer, size);
  lock_release(file_lock);
  return size;
}

int sys_write (int fd, const void *buffer, unsigned size)
{
  struct thread *cur = thread_current();
  struct file *file;
  lock_acquire(file_lock);

  // STDOUT, write to console
  if (fd == 1)
  {
    putbuf(buffer, size);
    lock_release(file_lock);
    return size;
  }

  // not STDOUT, write to file
  file = cur->fd_table[fd];  
  // if can't find file, return -1
  if (!file)
  {
    lock_release(file_lock);
    return -1;
  }

  size = file_write(file, buffer, size);
  lock_release(file_lock);
  return size;
}

void sys_seek (int fd, unsigned position)
{
  struct thread *cur = thread_current();
  struct file *file; 

  file = cur->fd_table[fd];
  if (!file) return;

  file_seek(file, position);
}

unsigned sys_tell (int fd)
{
  struct file *file;

  file = thread_current()->fd_table[fd];
  ASSERT(!file);

  return file_tell(file);
}

void sys_close (int fd)
{
  struct file *file;
  
  file = thread_current()->fd_table[fd];
  ASSERT(!file);

  file_close(file);
}