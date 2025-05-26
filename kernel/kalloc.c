// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "proc.h"
#include "defs.h"

void freepages();
void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

struct spinlock page_lock;
struct page pages[PHYSTOP/PGSIZE];  // virtual pages
struct page *page_lru_head;         // mapped virtual page
char *swap_space_bitmap;
int num_free_pages; // of swap space
int num_lru_pages;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  initlock(&page_lock, "page");
  freepages();
  freerange(end, (void*)PHYSTOP);

  swap_space_bitmap = kalloc();
  memset((char*)swap_space_bitmap, 0, PGSIZE); // 0으로 초기화
  num_free_pages = PGSIZE;
  num_lru_pages = 0;
}

void
freepages()
{
  int i;
  for (i = 0; i < PHYSTOP / PGSIZE; i++) {
    pages[i].next = 0; // unused
    pages[i].prev = 0;
  }
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void*
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);
  
  if(!r) {
    acquire(&page_lock);
    if(num_lru_pages > 0 && num_free_pages > 0)
      r = swapout();
    else
      printf("out of memory\n"); // OOM
    release(&page_lock);
  }

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}

// Requires the ```page_lock``` to be held before calling this function.
// Returns a pointer that the kernel can use.
void*
swapout(void)
{
  pte_t *pte;
  struct run *r;      // physical address
  struct page *vctm;  // victim page
  
  // clock algorithm (select victim)
  for(;;){
    vctm = page_lru_head;

    pte = walk(vctm->pagetable, (uint64)vctm->vaddr, 0); // 읽기 접근
    if((*pte & PTE_A) == 0) break;

    *pte &= ~PTE_A; // clear a PTE_A bit
    sfence_vma();   // TLB flush

    page_lru_head = page_lru_head->next;
  }

  r = (struct run *)walkaddr(vctm->pagetable, (uint64)vctm->vaddr);

  // swap to swap space
  int i; // offset
  for (i = 0; i < PGSIZE && swap_space_bitmap[i] != 0; i++);
  if (i == PGSIZE)
    panic("swapout: no space left");
  
  swap_space_bitmap[i] = 1;
  num_free_pages--;
  release(&page_lock);

  swapwrite(vctm->pagetable, (uint64)vctm->vaddr, i); // lock 없이 수행해야

  *pte = (i << 10) | PTE_FLAGS(*pte); // set PPN to offset // race condition 발생 가능성 존재
  *pte &= ~PTE_V;                     // clear a PTE_V bit
  sfence_vma();                       // TLB flush

  remove_lru(vctm->pagetable, (uint64)vctm->vaddr);
  acquire(&page_lock);

  return r;
}

// Handle a page fault.
void
swapin(uint64 va)
{
  char *pa = kalloc();
  if(pa == 0) return; // OOM (예외 처리 X)

  struct proc *p = myproc();
  pte_t *pte = walk(p->pagetable, va, 0);
  int i = *pte >> 10; // offset

  *pte = ((uint64)pa << 10) | PTE_FLAGS(*pte); // set PPN to physical address
  *pte |= PTE_V;                               // set a PTE_V bit
  sfence_vma();                                // TLB flush

  swapread(va, i);
  swap_space_bitmap_clear(i);

  if (*pte & PTE_U)
    append_lru(p->pagetable, va);
}

void
swap_space_bitmap_clear(int i)
{
  acquire(&page_lock);
  swap_space_bitmap[i] = 0;
  num_free_pages++;
  release(&page_lock);
}

void
append_lru(pagetable_t pagetable, uint64 va)
{
  acquire(&page_lock);
  int i;
  for (i = 0; i < PHYSTOP/PGSIZE && pages[i].next != 0; i++);
  pages[i].pagetable = pagetable;
  pages[i].vaddr = (char*)va;

  if(num_lru_pages > 0) {
    pages[i].next = page_lru_head;
    pages[i].prev = page_lru_head->prev;
    page_lru_head->prev->next = &pages[i];
    page_lru_head->prev = &pages[i];
  } else {
    pages[i].next = &pages[i];
    pages[i].prev = &pages[i];
    page_lru_head = &pages[i];
  }
  num_lru_pages++;
  release(&page_lock);
}

void
remove_lru(pagetable_t pagetable, uint64 va)
{
  acquire(&page_lock);
  if (num_lru_pages == 0) {
    release(&page_lock);
    return;
  }

  struct page *temp = page_lru_head;
  do{
    if(temp->pagetable == pagetable && temp->vaddr == (char*)va) {
      if(num_lru_pages == 1)
        temp->next = 0; // free(temp)
      else {
        temp->next->prev = temp->prev;
        temp->prev->next = temp->next;
        if (temp == page_lru_head) // remove head
          page_lru_head = temp->next;
        temp->next = 0; // free(temp)
      }
      num_lru_pages--;
      break;
    }

    temp = temp->next;
  } while(page_lru_head != temp);
  release(&page_lock);
}