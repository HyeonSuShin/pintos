#include "vm/frame.h"
#include "threads/malloc.h"

static struct list ftable;
static struct lock frame_lock;

static void ftable_insert(struct ft_entry*);
static void ftable_delete(struct ft_entry*);
static struct ft_entry* ftable_find(void*);
static void make_ft_entry(void *paddr, struct page *page);

void ftable_init(){
  list_init(&ftable);
  lock_init(&frame_lock);
}

void *falloc_get_page(enum palloc_flags flag, struct page *page){
  lock_acquire(&frame_lock);
  void *paddr = palloc_get_page(flag);
  make_ft_entry(paddr, page);
  lock_release(&frame_lock);
  return paddr;
}

void falloc_free_page(void *paddr){
  lock_acquire(&frame_lock);
  struct ft_entry *entry = ftable_find(paddr);
  palloc_free_page(paddr);
  ftable_delete(entry);
  lock_release(&frame_lock);
}

static void ftable_insert(struct ft_entry* entry){
  list_push_back(&ftable, &entry->elem);
}

static void ftable_delete(struct ft_entry* entry){
  list_remove(&entry->elem);
}

static struct ft_entry* ftable_find(void* key){
  for (struct list_elem *e = list_begin(&ftable);e!=list_end(&ftable);e=list_next(e)){
    if (list_entry(e, struct ft_entry, elem)->paddr == key){
      return list_entry(e, struct ft_entry, elem);
    }
  }
  return NULL;
}

static void make_ft_entry(void *paddr, struct page *page){
  struct ft_entry *entry = (struct ft_entry *)malloc(sizeof(struct ft_entry));
  entry->paddr = paddr;
  entry->page = page;
  ftable_insert(entry);
}
