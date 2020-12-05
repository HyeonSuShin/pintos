#ifndef VM_FRAME_H
#define VM_FRAME_H

#include "threads/thread.h"
#include "threads/palloc.h"
#include <list.h>
#include "vm/page.h"

//frame table entry
struct fte{
  void *paddr;
  struct page *page;
  struct list_elem elem;
};

void ftable_init();
void *falloc_get_page(enum palloc_flags, struct spte *page);
void falloc_free_page(void *paddr);
struct fte* ftable_find(void*);

#endif
