#ifndef VM_FRAME_H
#define VM_FRAME_H

#include "threads/thread.h"
#include "threads/palloc.h"
#include <list.h>
#include "vm/page.h"

struct ft_entry{
  void *paddr;
  struct page *page;
  struct list_elem elem;
};

void ftable_init();
void *falloc_get_page(enum palloc_flags, struct page *page);
void falloc_free_page(void *paddr);

#endif
