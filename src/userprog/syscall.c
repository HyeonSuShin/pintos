#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "userprog/process.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "devices/shutdown.h"
#include "devices/input.h"
#include "filesys/filesys.h"
#include "userprog/pagedir.h"
#include "threads/vaddr.h"

static void syscall_handler (struct intr_frame *);
struct lock file_lock;

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init(&file_lock);
}

void get_argument(void *esp, int *arg, int count){
  for(int i = 0; i < count; i ++){
    if(!check_address(esp + 4*i)) 
    {
      // printf("get ar\n");
      sys_exit(-1);
    }
    arg[i] = *(int *)(esp + 4*i);
  }
}

static void
syscall_handler (struct intr_frame *f) 
{
  // if address is invalid, exit program
  if (!check_address(f->esp))
  {
    // printf("cehc ar\n");
    sys_exit(-1);
  }

  // argument_stack
  switch (*(int*)f->esp)
  {
    int argv[3];
    case SYS_HALT:  
      sys_halt();
      break;
    case SYS_EXIT:
      get_argument(f->esp + 4, &argv[0], 1);
      // printf("name : %s, code : %d\n\n", thread_name(), (int)argv[0]);
      sys_exit((int)argv[0]);
      break;
    case SYS_EXEC:
      get_argument(f->esp + 4, &argv[0], 1);
      if(!check_address((char*)argv[0])) sys_exit(-1);
      f->eax = sys_exec((const char*)argv[0]);
      break;
    case SYS_WAIT:
      get_argument(f->esp + 4, &argv[0], 1);
      f->eax = sys_wait((pid_t)argv[0]);
      break;
    case SYS_CREATE:
      get_argument(f->esp + 4, &argv[0], 2);
      if(!check_address((char*)argv[0])) sys_exit(-1);
      f->eax = sys_create((const char*)argv[0], (unsigned)argv[1]);
      break;
    case SYS_REMOVE:
      get_argument(f->esp + 4, &argv[0], 1);
      if(!check_address((char*)argv[0])) sys_exit(-1);
      f->eax = sys_remove((const char*) argv[0]);
      break;
    case SYS_OPEN:
      get_argument(f->esp + 4, &argv[0], 1);
      if(!check_address((char*)argv[0])) sys_exit(-1);
      f->eax = sys_open((const char*) argv[0]);
      break;
    case SYS_FILESIZE:
      get_argument(f->esp + 4, &argv[0], 1);
      f->eax = sys_filesize((int)argv[0]);
      break;
    case SYS_READ:
      get_argument(f->esp + 4, &argv[0], 3);
      if(!check_address((void*)argv[1])) sys_exit(-1);
      f->eax = sys_read((int)argv[0], (void*)argv[1], (unsigned)argv[2]);
      break;
    case SYS_WRITE:
      get_argument(f->esp + 4, &argv[0], 3);
      if(!check_address((void*)argv[1])) sys_exit(-1);
      f->eax = sys_write((int)argv[0], (const void*)argv[1], (unsigned)argv[2]);
      break;
    case SYS_SEEK:
      get_argument(f->esp + 4, &argv[0], 2);
      sys_seek((int)argv[0], (unsigned)argv[1]);
      break;
    case SYS_TELL:
      get_argument(f->esp + 4, &argv[0], 1);
      f->eax = sys_tell((int)argv[0]);
      break;
    case SYS_CLOSE:
      get_argument(f->esp + 4, &argv[0], 1);
      sys_close((int)argv[0]);
      break;
    default:
      // printf("default\n");
      sys_exit(-1);
  }
}

bool check_address (void *addr)
{
  if (addr >= 0x8048000 && addr < 0xc0000000 && addr != 0)
    return true;
  return false;
}

void sys_halt (void)
{
  shutdown_power_off();
}

void sys_exit(int status)
{
  thread_current()->pcb->exit_status = status;
  printf("%s: exit(%d)\n", thread_current()->name, status);
  thread_exit();
}

pid_t sys_exec(const char *cmd_line)
{
  pid_t pid;
  struct PCB *child;
  pid = process_execute(cmd_line);
  if (pid == -1){
    return -1;
  }
  child = thread_get_child(pid);
  if (!child){
    return -1;
  }
  if (child && !child->load_success){
    return -1;
  }
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
  
  if (!file){
    // printf("nofile\n");
    sys_exit(-1);
  }
  file_ptr = filesys_open(file);
  if (!file_ptr)
  {
    return -1;
  }
  fd = add_file(file_ptr);

  return fd;
}

int sys_filesize (int fd)
{
  struct thread *cur = thread_current();
  struct file *file = cur->pcb->fd_table[fd];

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
  lock_acquire(&file_lock);

  if (fd < 0 || fd == 1 || fd >= cur->pcb->next_fd)
  {
    lock_release(&file_lock);
    return -1;
  }

  // STDIN, store keyboard input in buffer
  if (fd == 0)
  {
    for (int i = 0; i < size; i++){
      *((char*)buffer++) = input_getc();
    }
    lock_release(&file_lock);
    return size;
  }

  // not STDIN, store file in buffer
  file = cur->pcb->fd_table[fd];  
  // if can't find file, return -1
  if (!file)
  {
    lock_release(&file_lock);
    return -1;
  }

  size = file_read(file, buffer, size);
  lock_release(&file_lock);

  return size;
}

int sys_write (int fd, const void *buffer, unsigned size)
{
  struct thread *cur = thread_current();
  struct file *file;
  lock_acquire(&file_lock);
  
  if (fd == 0 || fd >= cur->pcb->next_fd)
  {
    lock_release(&file_lock);
    return -1;
  }

  // STDOUT, write to console
  if (fd == 1)
  {
    putbuf(buffer, size);
    lock_release(&file_lock);
    return size;
  }
  // not STDOUT, write to file
  file = cur->pcb->fd_table[fd];  
  // if can't find file, return -1
  if (!file)
  {
    lock_release(&file_lock);
    return -1;
  }
  size = file_write(file, buffer, size);
  lock_release(&file_lock);

  return size;
}

void sys_seek (int fd, unsigned position)
{
  struct thread *cur = thread_current();
  struct file *file; 

  file = cur->pcb->fd_table[fd];
  if (!file) return;

  file_seek(file, position);
}

unsigned sys_tell (int fd)
{
  struct file *file;

  file = thread_current()->pcb->fd_table[fd];
  ASSERT(!file);

  return file_tell(file);
}

void sys_close (int fd)
{
  struct file *file;
  struct thread *cur = thread_current();

  if (fd >= cur->pcb->next_fd || fd<=1){
    // printf("?????????\n");
    sys_exit(-1);
  }
  
  file = cur->pcb->fd_table[fd];
  ASSERT(file);

  file_close(file);
  cur->pcb->fd_table[fd] = NULL;
  for(int i = fd; i < cur->pcb->next_fd; i++){
    cur->pcb->fd_table[i] = cur->pcb->fd_table[i + 1];
  }
  cur->pcb->next_fd--;
}