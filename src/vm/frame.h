#ifndef VM_FRAME_H
#define VM_FRAME_H

#include "userprog/pagedir.h"
#include "vm/swap.h"
#include "threads/thread.h"
#include "threads/palloc.h"
#include <list.h>
#include "vm/page.h"

//frame table entry
struct fte{
  void *paddr;
  struct spte *page;
  struct thread *holder;
  struct list_elem elem;
};

void ftable_init();
void *falloc_get_page(enum palloc_flags, struct spte *page);
void falloc_free_page(void *paddr);
struct fte* ftable_find(void*);
void evict();
struct fte* get_page_to_evict();
struct fte* next_clock_hand();
void fte_destroy_by_holder(struct thread *t);

#endif
