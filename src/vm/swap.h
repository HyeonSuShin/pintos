#ifndef VM_SWAP_H
#define VM_SWAP_H

#include <bitmap.h>
#include "vm/frame.h"
#include "threads/synch.h"
#include "threads/vaddr.h"
#include "userprog/syscall.h"
#include "devices/block.h"

void swap_init();

void swap_in(size_t idx, void *kaddr);

void swap_out(void *kaddr);

void swap_clear(size_t idx);

#endif
