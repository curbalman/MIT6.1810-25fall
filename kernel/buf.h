#define NBUCKET 13
#define BKTSIZE ((NBUF/NBUCKET) + 1)
struct buf {
  int valid;   // has data been read from disk?
  int disk;    // does disk "own" buf?
  uint dev;
  uint blockno;
  struct sleeplock lock;
  uint refcnt;
  uchar data[BSIZE];
};

struct bucket {
  struct spinlock lock;
  // 总的buf数量大于NBUF，使用bktunused()限制
  struct buf buf[BKTSIZE];
};
