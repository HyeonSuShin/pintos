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
  thread_current()->esp = f->esp;
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
    case SYS_MMAP:
      get_argument(f->esp + 4, &argv[0], 2);
      f->eax = sys_mmap((int)argv[0], (void*)argv[1]);
      break;
    case SYS_MUNMAP:
      get_argument(f->esp + 4, &argv[0], 1);
      sys_munmap((int)argv[0]);
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
  
  lock_acquire(&file_lock);
  if (!file){
    lock_release(&file_lock);
    sys_exit(-1);
  }
  file_ptr = filesys_open(file);
  if (!file_ptr)
  {
    lock_release(&file_lock);
    return -1;
  }
  fd = add_file(file_ptr);

  lock_release(&file_lock);
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

mapid_t sys_mmap(int fd, void *addr){
  struct thread *cur = thread_current();
  struct file *file = cur->pcb->fd_table[fd];
  if(file == NULL){
    return -1;
  }
  if((int)addr % PGSIZE != 0 || !addr){
    return -1;
  }

  struct file *fork_file = file_reopen(file);
  if(fork_file == NULL){
    return -1;
  }

  struct mmap_file *mfile = mmap_init(cur->mmapid++, fork_file, addr);
  if(mfile == NULL){
    return -1;
  }

  return mfile->mmapid;
}

void sys_munmap(mapid_t mapid){
  struct thread *cur = thread_current();
  struct list_elem *e;
  struct mmap_file *mfile;
  void *vaddr;
  if(mapid >= cur->mmapid){
    return;
  }
  for(e = list_begin(&cur->mmap_list); e != list_end(&cur->mmap_list); e = list_next(e)){
    mfile = list_entry(e, struct mmap_file, mmap_elem);
    if(mfile->mmapid == mapid){
      break;
    }
  }
  if(e == list_end(&cur->mmap_list)){
    return;
  }

  vaddr = mfile->vaddr;

  lock_acquire(&file_lock);

  for(off_t ofs = 0; ofs < file_length(mfile->file); ofs += PGSIZE){
    struct spte *entry = sptable_find(vaddr);
    if(entry->type == PAGE_ANON){
      if(!page_fault_handler(vaddr)){
        sys_exit(-1);
      }
    }
    if(entry->type == PAGE_FRAME){
      void *kaddr = pagedir_get_page(cur->pagedir, vaddr);
      if (pagedir_is_dirty(cur->pagedir, vaddr)){
        file_write_at(entry->file, kaddr, entry->read_bytes, entry->ofs);
      }
      falloc_free_page(kaddr);
    }
    sptable_delete(&cur->spt, entry);
    vaddr += PGSIZE;
  }
  list_remove(e);

  lock_release(&file_lock);
}
