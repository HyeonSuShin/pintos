#include "page.h"
#include "threads/malloc.h"

static hash_hash_func page_hash_func;
static hash_less_func page_less_func;
static void sptable_insert(struct hash *spt, struct spte *entry);

void sptable_init(struct hash *spt){
  hash_init(spt, page_hash_func, page_less_func, NULL);
}

void sptable_delete(struct hash *spt, struct spte *entry){
  hash_delete(spt, &entry->hash_elem);
  free(entry);
}

struct spte* make_spt_entry(struct hash *spt, struct file *file, off_t ofs, uint8_t *upage, uint32_t read_bytes, uint32_t zero_bytes, bool writable, int type){
  struct spte *entry = (struct spte*) malloc(sizeof(struct spte));
  entry->file = file;
  entry->ofs = ofs;
  entry->vaddr = upage;
  entry->read_bytes = read_bytes;
  entry->zero_bytes = zero_bytes;
  entry->writable = writable;
  entry->type = type;
  sptable_insert(spt, entry);
  return entry;
}

static unsigned page_hash_func (const struct hash_elem *elem, void *aux){
  struct spte *p = hash_entry(elem, struct spte, hash_elem);

  return hash_bytes(&p->vaddr, sizeof(p->vaddr));
}

static bool page_less_func (const struct hash_elem *a, const struct hash_elem *b, void *aux){
  void *addr1 = hash_entry(a, struct spte, hash_elem)->vaddr;
  void *addr2 = hash_entry(b, struct spte, hash_elem)->vaddr;

  return addr1 < addr2;
}

static void sptable_insert(struct hash *spt, struct spte *entry){
  hash_insert(spt, &entry->hash_elem);
}
