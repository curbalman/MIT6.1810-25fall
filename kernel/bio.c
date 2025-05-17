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

//struct spinlock bcachelock;
struct bucket bcache[NBUCKET];

static uint
hash(unsigned int x) {
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = (x >> 16) ^ x;
    return x % NBUCKET;
}

void
binit(void)
{
  //initlock(&bcachelock, "bcache");
  for(int i = 0; i < NBUCKET; ++i) {
    struct bucket *bkt = &(bcache[i]);
    initlock(&(bkt->lock), "bcache.bucket");
    for(int j = 0; j < BKTSIZE; ++j) {
      initsleeplock(&(bkt->buf[j].lock), "buffer");
      bkt->buf[j].bktlk = &(bkt->lock);
    }
  }
}

// 限制总的buf数量为NBUF
// return 1 if buf is unused
static int
bktunused(int ibkt, int ibuf)
{
  // 不使用BKTSIZE*NBUCKET-NBUF个桶的最后一个buf
  if( ibkt < (BKTSIZE*NBUCKET-NBUF) && ibuf == BKTSIZE-1 )
    return 1;
  else
    return 0;
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  const int mybkt = hash(blockno);
  int ibkt;

  push_off();

  // Is the block already cached?
  // 从自己的桶开始搜索，如果自己的桶没有再搜索别人的桶
  ibkt = mybkt;
  do {
    acquire( &(bcache[ibkt].lock) );
    for(int i = 0; i < BKTSIZE; ++i) {
      if( bktunused(ibkt, i) ) continue;
      b = &(bcache[ibkt].buf[i]);
      if(b->dev == dev && b->blockno == blockno){
        b->refcnt++;
        release( &(bcache[ibkt].lock) );
        pop_off();
        acquiresleep(&b->lock);
        return b;
      }
    }
    release( &(bcache[ibkt].lock) );
  } while( (ibkt = (ibkt + 1) % NBUCKET) != mybkt );

  // BUG: 其他CPU可能也bget同一个blockno，且两个CPU同时运行到这里，会造成重复分配
  // 可以加一个驱逐锁，拿到锁之后再检查一遍blockno有没有缓存
  // 见https://blog.miigon.net/posts/s081-lab8-locks/#%E6%96%B0%E7%9A%84%E9%97%AE%E9%A2%98%E9%87%8A%E6%94%BE%E8%87%AA%E8%BA%AB%E6%A1%B6%E9%94%81%E5%8F%AF%E8%83%BD%E4%BD%BF%E5%BE%97%E5%90%8C-blockno-%E9%87%8D%E5%A4%8D%E9%A9%B1%E9%80%90%E4%B8%8E%E5%88%86%E9%85%8D

  // Not cached.
  // 从自己桶(mybkt)的下一个桶开始搜索
  for(ibkt = (mybkt + 1) % NBUCKET; ibkt != mybkt;
      ibkt = (ibkt + 1) % NBUCKET) {
    acquire( &(bcache[ibkt].lock) );
    for(int i = 0; i < BKTSIZE; ++i) {
      if( bktunused(ibkt, i) ) continue;
      b = &(bcache[ibkt].buf[i]);
      if(b->refcnt == 0) {
        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;
        release( &(bcache[ibkt].lock) );
        pop_off();
        acquiresleep(&b->lock);
        return b;
      }
    }
    release( &(bcache[ibkt].lock) );
  }
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
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  push_off();
  acquire(b->bktlk);
  b->refcnt--;
  release(b->bktlk);
  pop_off();
}

void
bpin(struct buf *b) {
  push_off();
  acquire(b->bktlk);
  b->refcnt++;
  release(b->bktlk);
  pop_off();
}

void
bunpin(struct buf *b) {
  push_off();
  acquire(b->bktlk);
  b->refcnt--;
  release(b->bktlk);
  pop_off();
}


