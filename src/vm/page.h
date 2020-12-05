#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <hash.h>
#include <stdbool.h>
#include "filesys/file.h"
#include "filesys/off_t.h"
#include "vm/frame.h"

#define PAGE_BIN_FILE 0
#define PAGE_FRAME    1
#define PAGE_ANON     2

//supplemental page table entry
struct spte{
  struct file *file;
  off_t ofs;
  uint32_t read_bytes;
  uint32_t zero_bytes;
  bool writable;
  void *vaddr;
  int type;
  struct hash_elem hash_elem;
};

void sptable_init(struct hash* spt);
void sptable_delete(struct hash *spt, struct spte *entry);
struct spte* make_spt_entry(struct hash *spt, struct file *file, off_t ofs, uint8_t *upage, uint32_t read_bytes, uint32_t zero_bytes, bool writable, int type);
struct spte* sptable_find(void *addr);
bool page_fault_handler(void *addr);

#endif
