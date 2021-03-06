#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include <stdbool.h>

typedef int pid_t;
typedef int mapid_t;

void syscall_init (void);

bool check_address (void *addr);

void sys_halt (void);
void sys_exit(int status);
pid_t sys_exec(const char *cmd_line);
int sys_wait (pid_t pid);
bool sys_create (const char *file, unsigned initial_size);
bool sys_remove (const char *file);
int sys_open (const char *file);
int sys_filesize (int fd);
int sys_read (int fd, void *buffer, unsigned size);
int sys_write (int fd, const void *buffer, unsigned size);
void sys_seek (int fd, unsigned position);
unsigned sys_tell (int fd);
void sys_close (int fd);
mapid_t sys_mmap(int fd, void *addr);
void sys_munmap(mapid_t mapid);

#endif /* userprog/syscall.h */
