#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"
#include "userprog/syscall.h"
#include "vm/frame.h"

typedef int tid_t;

struct PCB{
    pid_t pid;
    struct semaphore load;
    struct semaphore wait;
    int exit_status;
    struct thread *parent;
    struct list_elem child_elem;
    bool load_success;
    bool die;
    struct file **fd_table; // close all when process exit
    struct file *load_file;
    int next_fd;
};

struct tempPCB{
    struct PCB* _pcb;
    char *cmd;
};

static void CMD2FileName(char *);
int find_argc(char *);
char** make_argv(char *);
void argument_stack(char **, int, void **esp);
tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);

int add_file(struct file *file);

#endif /* userprog/process.h */
