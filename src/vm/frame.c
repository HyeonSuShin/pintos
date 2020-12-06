#include "vm/frame.h"
#include "threads/malloc.h"

static struct list ft;
static struct lock frame_lock;
static struct fte *clock_hand;

static void ftable_insert(struct fte*);
static void ftable_delete(struct fte*);
static void make_ft_entry(void *paddr, struct spte *page);

void ftable_init(){
  list_init(&ft);
  lock_init(&frame_lock);
  clock_hand = NULL;
}

void *falloc_get_page(enum palloc_flags flag, struct spte *page){
  lock_acquire(&frame_lock);
  void *paddr = palloc_get_page(flag);
  if(!paddr){
    lock_release(&frame_lock);
    evict();
    lock_acquire(&frame_lock);
    paddr = palloc_get_page(flag);
  }
  make_ft_entry(paddr, page);
  lock_release(&frame_lock);
  return paddr;
}

void falloc_free_page(void *paddr){
  lock_acquire(&frame_lock);
  struct fte *entry = ftable_find(paddr);
  palloc_free_page(paddr);
  pagedir_clear_page(entry->holder->pagedir, entry->page->vaddr);
  ftable_delete(entry);
  lock_release(&frame_lock);
}

struct fte* ftable_find(void* key){
  for (struct list_elem *e = list_begin(&ft);e!=list_end(&ft);e=list_next(e)){
    if (list_entry(e, struct fte, elem)->paddr == key){
      return list_entry(e, struct fte, elem);
    }
  }
  return NULL;
}

static void ftable_insert(struct fte* entry){
  list_push_back(&ft, &entry->elem);
}

static void ftable_delete(struct fte* entry){
  list_remove(&entry->elem);
}

static void make_ft_entry(void *paddr, struct spte *page){
  struct fte *entry = (struct fte *)malloc(sizeof(struct fte));
  entry->paddr = paddr;
  entry->page = page;
  entry->holder = thread_current();
  ftable_insert(entry);
}

void evict(){
  lock_acquire(&frame_lock);
  struct fte *victim = get_page_to_evict();
  victim->page->type = PAGE_ANON;
  lock_release(&frame_lock);
  swap_out(victim->paddr);
  falloc_free_page(victim->paddr);
}

struct fte* get_page_to_evict(){
  struct fte *e = clock_hand;
  do{
    e = next_clock_hand();
    if(!pagedir_is_accessed(e->holder->pagedir, e->page->vaddr)){
      return e;
    }
    pagedir_set_accessed(e->holder->pagedir, e->page->vaddr, false);
  }while(true);
}

struct fte* next_clock_hand(){
  struct list_elem *e = &clock_hand->elem;
  if(clock_hand == NULL || list_next(e) == list_end(&ft)){
    e = list_begin(&ft);
  } else{
    e = list_next(e);
  }
  return list_entry(e, struct fte, elem);
}

void fte_destroy_by_holder(struct thread *t){
  lock_acquire(&frame_lock);
  for (struct list_elem *e = list_begin(&ft); e!=list_end(&ft); e=list_next(e)){
    struct fte *entry = list_entry(e, struct fte, elem);
    if (entry->holder == t) {
      ftable_delete(entry);
    }
  }
  lock_release(&frame_lock);
}
