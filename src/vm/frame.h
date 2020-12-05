#ifndef VM_FRAME_H
#define VM_FRAME_H

#include "threads/thread.h"

struct ft_entry{
  void *key;
  void *entry;
  struct thread *owner;
};

static void ftable_init(struct hash*);
static unsigned ftable_hash(struct hash_elem*, void*);
void ftable_insert(struct hash*, struct ft_entry*);
void ftable_delete(struct hash*, struct ft_entry*);
struct ft_entry* ftable_find(struct hash*, void* key);
struct 


#endif
