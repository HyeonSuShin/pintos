#include "vm/swap.h"

static struct lock swap_lock;
static struct bitmap *swap_bitmap;
static struct block *swap_block;

#define SECTOR_NUM (PGSIZE/BLOCK_SECTOR_SIZE)

void swap_init(){
  lock_init(&swap_lock);
  swap_block = block_get_role(BLOCK_SWAP);
  swap_bitmap = bitmap_create(block_size(swap_block) / SECTOR_NUM);
}

void swap_in(size_t idx, void *kaddr){
  printf("before idx = %d, kaddr = %p\n", idx, kaddr);
  int i;
  if(idx > block_size(swap_block) / SECTOR_NUM || idx < 0){
    sys_exit(-1);
  }

  lock_acquire(&swap_lock);
  for(i = 0; i < SECTOR_NUM; i++){
    block_read(swap_block, idx * SECTOR_NUM + i, kaddr + (i * BLOCK_SECTOR_SIZE));
  }

  if(!bitmap_test(swap_bitmap, idx)){
    lock_release(&swap_lock);
    sys_exit(-1);
  }
  bitmap_set(swap_bitmap, idx, false);
  lock_release(&swap_lock);
  printf("end\n");
  for (i = 0; i < 10; i ++)
    printf("%x ", *(char *)(kaddr + i));
  printf("\n");
  return;
}

void swap_out(void *kaddr){
  int i;
  lock_acquire(&swap_lock);
  struct spte *entry = ftable_find(kaddr)->page;
  if(!entry){
    lock_release(&swap_lock);
    return;
  }
  size_t idx = bitmap_scan_and_flip(swap_bitmap, 0, 1, false);
  printf("idx = %d\n", idx);
  for(i = 0; i < SECTOR_NUM; i++){
    block_write(swap_block, idx * SECTOR_NUM + i, kaddr + (i * BLOCK_SECTOR_SIZE));
  }
  entry->idx = idx;
  lock_release(&swap_lock);
  for (i = 0; i < 10; i ++)
    printf("%x ", *(char *)(kaddr + i));
  printf("\n");
}

void swap_clear(size_t idx){

}
