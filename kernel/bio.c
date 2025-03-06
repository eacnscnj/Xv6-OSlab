// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

//==========================================
int
hash(int dev, int blockno)
{
  return ((dev * 100 + blockno) % BUKETSIZE);
}
//==========================================

struct {
  struct spinlock lock;
  struct buf buf[NBUF];
  //=======================================
  struct spinlock bcache_lock[BUKETSIZE];
  //========================================

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head[BUKETSIZE];
} bcache;

void
binit(void)
{
  struct buf *b;

  initlock(&bcache.lock, "bcache");
  for(int i=0;i<BUKETSIZE;i++){
    initlock(&bcache.bcache_lock[i], "bcache_lock");
  }
  for(int i=0;i<BUKETSIZE;i++){
    bcache.head[i].prev = &bcache.head[i];
    bcache.head[i].next = &bcache.head[i];
  }

  // Create linked list of buffers
  //bcache.head.prev = &bcache.head;
  //bcache.head.next = &bcache.head;
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = bcache.head[0].next;
    b->prev = &bcache.head[0];
    initsleeplock(&b->lock, "buffer");
    bcache.head[0].next->prev = b;
    bcache.head[0].next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  struct buf *marke = 0;
  int idx = hash(dev, blockno);
  int min_ticks = 0;

  acquire(&bcache.bcache_lock[idx]);

  // Is the block already cached?
  for(b = bcache.head[idx].next; b != &bcache.head[idx]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.bcache_lock[idx]);
      acquiresleep(&b->lock);
      return b;
    }
  }
  release(&bcache.bcache_lock[idx]);

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  acquire(&bcache.lock);
  acquire(&bcache.bcache_lock[idx]);
  //find from current bucket
  for(b = bcache.head[idx].next; b != &bcache.head[idx]; b = b->next){
    if(b->dev == dev && b->blockno == blockno) {
      b->refcnt++;
      release(&bcache.bcache_lock[idx]);
      release(&bcache.lock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  //find a LRU block from current bucket
  for(b = bcache.head[idx].next; b != &bcache.head[idx]; b = b->next){
    if(b->refcnt == 0 && (marke == 0 || b->recent_time < min_ticks)) {
      min_ticks = b->recent_time;
      marke = b;
    }
  }
  if(marke != 0){
    marke->dev = dev;
    marke->blockno = blockno;
    marke->valid = 0;
    marke->refcnt++;
    release(&bcache.bcache_lock[idx]);
    release(&bcache.lock);
    acquiresleep(&marke->lock);
    return marke;
  }
  // panic("here1");
  //find from other bucket
  for(int i = 0; i < BUKETSIZE; i++){
    if(i == idx)continue;

    acquire(&bcache.bcache_lock[i]);
    for(b = bcache.head[i].next; b != &bcache.head[i]; b = b->next){
      if(b->refcnt == 0 && (marke == 0 || b->recent_time < min_ticks)) {
        min_ticks = b->recent_time;
        marke = b;
      }
    }
    if(marke){
      marke->dev = dev;
      marke->blockno = blockno;
      marke->valid = 0;
      marke->refcnt++;
      //don't forget to change the list
      marke->next->prev = marke->prev;
      marke->prev->next = marke->next;
      release(&bcache.bcache_lock[i]);
      //add to the head of the list
      marke->next = bcache.head[idx].next;
      marke->prev = &bcache.head[idx];
      bcache.head[idx].next->prev = marke;
      bcache.head[idx].next = marke;
      release(&bcache.bcache_lock[idx]);
      release(&bcache.lock);
      acquiresleep(&marke->lock);
      //panic("here");
      return marke;
    }
    release(&bcache.bcache_lock[i]);
  }
  //panic("here");
  release(&bcache.bcache_lock[idx]);
  release(&bcache.lock);
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  int idx = hash(b->dev, b->blockno);

  acquire(&bcache.bcache_lock[idx]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    // b->next->prev = b->prev;
    // b->prev->next = b->next;
    // b->next = bcache.head.next;
    // b->prev = &bcache.head;
    // bcache.head.next->prev = b;
    // bcache.head.next = b;
    // ticks recording the time til system start
    b->recent_time=ticks;
  }
  
  release(&bcache.bcache_lock[idx]);
}

void
bpin(struct buf *b) {
  int idx = hash(b->dev, b->blockno);
  acquire(&bcache.bcache_lock[idx]);
  b->refcnt++;
  release(&bcache.bcache_lock[idx]);
}

void
bunpin(struct buf *b) {
  int idx = hash(b->dev, b->blockno);
  acquire(&bcache.bcache_lock[idx]);
  b->refcnt--;
  release(&bcache.bcache_lock[idx]);
}


