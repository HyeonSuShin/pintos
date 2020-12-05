#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <hash.h>
#include <stdbool.h>

struct page{
  void *vaddr;
  size_t offset;
  size_t size;
  size_t zero_size;
  bool writable;
  struct file *file;
  int type;
  struct hash_elem hash_elem;
};

unsigned page_hash_func(struct hash_elem *elem, void *aux);
bool page_less_func(const struct hash_elem *a, const struct hash_elem *b, void *aux);

#endif
