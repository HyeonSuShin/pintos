#include "page.h"
#include "threads/malloc.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"

extern struct lock file_lock;

static hash_hash_func page_hash_func;
static hash_less_func page_less_func;
static void sptable_insert(struct hash *spt, struct spte *entry);
static bool install_page (void *upage, void *kpage, bool writable);

void sptable_init(struct hash *spt){
  hash_init(spt, page_hash_func, page_less_func, NULL);
}

void sptable_delete(struct hash *spt, struct spte *entry){
  hash_delete(spt, &entry->hash_elem);
  free(entry);
}

bool page_fault_handler(void *addr){
  void * page = pg_round_down(addr);
  struct spte *pt_entry = sptable_find(page);
  bool locked = lock_held_by_current_thread(&file_lock);

  if(pt_entry == NULL){
    return false;
  }

  uint8_t *kpage = falloc_get_page (PAL_USER, pt_entry);
  if (kpage == NULL){
    sptable_delete(&thread_current()->spt, pt_entry);
    return false;
  }

  if (!locked)
    lock_acquire(&file_lock);

  if (file_read_at (pt_entry->file, kpage, pt_entry->read_bytes, pt_entry->ofs) != (int) pt_entry->read_bytes){
    lock_release(&file_lock);
    sptable_delete(&thread_current()->spt, pt_entry);
    falloc_free_page (kpage);
    return false; 
  }
  memset (kpage + pt_entry->read_bytes, 0, pt_entry->zero_bytes);

  if (!locked)
    lock_release(&file_lock);

  if (!install_page (pt_entry->vaddr, kpage, pt_entry->writable)){
    sptable_delete(&thread_current()->spt, pt_entry);
    falloc_free_page (kpage);
    return false; 
  }

  pt_entry->type = PAGE_FRAME;

  return true;
}

struct spte* sptable_find(void *addr){
  struct spte temp;
  struct hash_elem *e;

  temp.vaddr = addr;
  e = hash_find(&thread_current()->spt, &temp.hash_elem);

  return e != NULL ? hash_entry(e, struct spte, hash_elem) : NULL;
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

static bool install_page (void *upage, void *kpage, bool writable){
  return (pagedir_get_page (thread_current()->pagedir, upage) == NULL
          && pagedir_set_page (thread_current()->pagedir, upage, kpage, writable));
}
