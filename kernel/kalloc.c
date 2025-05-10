// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "fs.h"
#include "file.h"
#include "kalloc.h"
#include "proc.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
  uint freemem;
} kmem;

struct mmap_area mmap_area[NMMAP];

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  kmem.freemem = 0;

  struct mmap_area *m;
  for (m = mmap_area; m < mmap_area + NMMAP; m++)
    m->length = 0;

  freerange(end, (void*)PHYSTOP);
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
  kmem.freemem++;
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
  if(r) {
    kmem.freelist = r->next;
    kmem.freemem--;
  }
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}

uint64
meminfo(void)
{
  struct run *r;
  uint64 freemem = 0;

  acquire(&kmem.lock);
  r = kmem.freelist;
  while (r) {
    freemem++;
    r = r->next;
  }
  release(&kmem.lock);

  return freemem * 4096;
}

// if option = 0, find empty mmap_area
// if option = 1, fine corresponding mmap_area
struct mmap_area*
find_mmap_area(uint64 addr, int option)
{
  struct proc *p = myproc();
  struct mmap_area *m;
  for (m = mmap_area; m < mmap_area + NMMAP; m++) {
    if (option == 0 && m->length == 0) break;
    if (option == 1 && m->length != 0 &&
      m->addr <= addr && addr < m->addr + m->length &&
      m->p == p)
      break;
  }

  if (m == mmap_area + NMMAP) return 0;
  else return m;
}

void
copy_mmap_area(struct proc *p, struct proc *np)
{
  struct mmap_area *m, *nm;
  for (m = mmap_area; m < mmap_area + NMMAP; m++) {
    if (m->length != 0 && m->p == p) {
      nm = find_mmap_area(m->addr, 0);

      nm->addr = m->addr;
      nm->length = m->length;
      nm->prot = m->prot;
      nm->flags = m->flags;
      nm->f = m->f;
      nm->offset = m->offset;
      nm->p = np;


      pte_t *pte;
      uint64 pa, mem;
      for (int l = 0; l < m->length; l += PGSIZE) {
        // virtual memory is mapped in the parent process
        if (walkaddr(p->pagetable, MMAPBASE + m->addr + l) != 0) {
          pte = walk(p->pagetable, MMAPBASE + m->addr + l, 0);
          pa = PTE2PA(*pte);

          mem = (uint64)kalloc();
          memmove((void *)mem, (void *)pa, PGSIZE);
          mappages(
            np->pagetable, 
            MMAPBASE + nm->addr + l, 
            PGSIZE, 
            mem, 
            (nm->prot & PROT_READ ? PTE_R : 0) | (nm->prot & PROT_WRITE ? PTE_W : 0) | PTE_U
          );
        }
      }
    }
  }
}

void 
clear_mmap_area(struct proc *p)
{
  int length;
  uint64 addr;
  struct mmap_area *m;
  for (m = mmap_area; m < mmap_area + NMMAP; m++) {
    if (m->length != 0 && m->p == p) {
      addr = m->addr;
      length = m->length;

      // free physical memory and page table
      for (int l = 0; l < length; l += PGSIZE)
        if (walkaddr(p->pagetable, MMAPBASE + addr + l) != 0)
          uvmunmap(p->pagetable, MMAPBASE + addr + l, 1, 1);

      m->length = 0;
    }
  }
}

void
mmappage(uint64 addr, int length, int prot, int flags, struct file *f, int offset, struct proc *p)
{
  for (int l = 0; l < length; l += PGSIZE) {
    uint64 pa = (uint64)kalloc();

    if (flags & MAP_ANONYMOUS)
      memset((void *)pa, 0, PGSIZE);
    else {
      ilock(f->ip);
      readi(f->ip, 0, pa, offset+l*PGSIZE, PGSIZE);
      iunlock(f->ip);
    }

    acquire(&p->lock);
    mappages(
      p->pagetable, 
      MMAPBASE + addr + l, 
      PGSIZE, 
      pa, 
      (prot & PROT_READ ? PTE_R : 0) | (prot & PROT_WRITE ? PTE_W : 0) | PTE_U
    );
    release(&p->lock);
  }
}

uint64
mmap(uint64 addr, int length, int prot, int flags, int fd, int offset)
{
  if (addr % PGSIZE != 0 || length % PGSIZE != 0 ||
    ((flags & MAP_ANONYMOUS) && (fd != -1 || offset != 0)) || 
    (!(flags & MAP_ANONYMOUS) && fd == -1))
    return 0;


  struct proc *p = myproc();
  struct file *f = 0;

  if (!(flags & MAP_ANONYMOUS)) {
    acquire(&p->lock);
    f = p->ofile[fd];
    release(&p->lock);

    // check that the prot of file and the prot param are same
    int file_prot = (f->readable ? 0x1 : 0) | (f->writable ? 0x2 : 0);
    if (prot != file_prot)
      return 0;
  }

  // record mmap_area
  struct mmap_area *m = find_mmap_area(addr, 0); // empty mmap_area
  if (m == 0) panic("mmap: shortage");

  m->addr = addr;
  m->length = length;
  m->prot = prot;
  m->flags = flags;
  m->f = f;
  m->offset = offset;
  m->p = p;

  if (flags & MAP_POPULATE)
    mmappage(addr, length, prot, flags, f, offset, p);

  return MMAPBASE + addr;
}

int
munmap(uint64 addr)
{
  int length;
  struct proc *p;
  struct mmap_area *m = find_mmap_area(addr, 1); // corresponding mmap_area
  if (m == 0) return -1;

  length = m->length;
  p = m->p;

  // free physical memory and page table
  acquire(&p->lock);
  for (int l = 0; l < length; l += PGSIZE) {
    if (walkaddr(p->pagetable, MMAPBASE + addr + l) != 0)
      uvmunmap(p->pagetable, MMAPBASE + addr + l, 1, 1);
  }
  release(&p->lock);

  m->length = 0;
  return 1;
}

int
freemem(void)
{
  uint64 freemem;

  acquire(&kmem.lock);
  freemem = kmem.freemem;
  release(&kmem.lock);

  return freemem;
}