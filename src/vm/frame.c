#include "vm/frame.h"
#include "threads/malloc.h"

static struct list ft;
static struct lock frame_lock;

static void ftable_insert(struct fte*);
static void ftable_delete(struct fte*);
static struct fte* ftable_find(void*);
static void make_ft_entry(void *paddr, struct spte *page);

void ftable_init(){
  list_init(&ft);
  lock_init(&frame_lock);
}

void *falloc_get_page(enum palloc_flags flag, struct spte *page){
  lock_acquire(&frame_lock);
  void *paddr = palloc_get_page(flag);
  make_ft_entry(paddr, page);
  lock_release(&frame_lock);
  return paddr;
}

void falloc_free_page(void *paddr){
  lock_acquire(&frame_lock);
  struct fte *entry = ftable_find(paddr);
  palloc_free_page(paddr);
  ftable_delete(entry);
  lock_release(&frame_lock);
}

static void ftable_insert(struct fte* entry){
  list_push_back(&ft, &entry->elem);
}

static void ftable_delete(struct fte* entry){
  list_remove(&entry->elem);
}

static struct fte* ftable_find(void* key){
  for (struct list_elem *e = list_begin(&ft);e!=list_end(&ft);e=list_next(e)){
    if (list_entry(e, struct fte, elem)->paddr == key){
      return list_entry(e, struct fte, elem);
    }
  }
  return NULL;
}

static void make_ft_entry(void *paddr, struct spte *page){
  struct fte *entry = (struct fte *)malloc(sizeof(struct fte));
  entry->paddr = paddr;
  entry->page = page;
  ftable_insert(entry);
}
