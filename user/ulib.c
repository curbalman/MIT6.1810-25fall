#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

//
// wrapper so that it's OK if main() does not call exit().
//
void
start()
{
  extern int main();
  main();
  exit(0);
}

char*
strcpy(char *s, const char *t)
{
  char *os;

  os = s;
  while((*s++ = *t++) != 0)
    ;
  return os;
}

int
strcmp(const char *p, const char *q)
{
  while(*p && *p == *q)
    p++, q++;
  return (uchar)*p - (uchar)*q;
}

uint
strlen(const char *s)
{
  int n;

  for(n = 0; s[n]; n++)
    ;
  return n;
}

void*
memset(void *dst, int c, uint n)
{
  char *cdst = (char *) dst;
  int i;
  for(i = 0; i < n; i++){
    cdst[i] = c;
  }
  return dst;
}


char*
strchr(const char *s, char c)
{
  for(; *s; s++)
    if(*s == c)
      return (char*)s;
  return 0;
}

char*
gets(char *buf, int max)
{
  int i, cc;
  char c;

  for(i=0; i+1 < max; ){
    cc = read(0, &c, 1);
    if(cc < 1)
      break;
    buf[i++] = c;
    if(c == '\n' || c == '\r')
      break;
  }
  buf[i] = '\0';
  return buf;
}

int
stat(const char *n, struct stat *st)
{
  int fd;
  int r;

  fd = open(n, O_RDONLY);
  if(fd < 0)
    return -1;
  r = fstat(fd, st);
  close(fd);
  return r;
}

int
atoi(const char *s)
{
  int n;

  n = 0;
  while('0' <= *s && *s <= '9')
    n = n*10 + *s++ - '0';
  return n;
}

void*
memmove(void *vdst, const void *vsrc, int n)
{
  char *dst;
  const char *src;

  dst = vdst;
  src = vsrc;
  if (src > dst) {
    while(n-- > 0)
      *dst++ = *src++;
  } else {
    dst += n;
    src += n;
    while(n-- > 0)
      *--dst = *--src;
  }
  return vdst;
}

int
memcmp(const void *s1, const void *s2, uint n)
{
  const char *p1 = s1, *p2 = s2;
  while (n-- > 0) {
    if (*p1 != *p2) {
      return *p1 - *p2;
    }
    p1++;
    p2++;
  }
  return 0;
}

void *
memcpy(void *dst, const void *src, uint n)
{
  return memmove(dst, src, n);
}

char *
fgets(char *s, int size, int fd)
{
  char *p;

  p = s;
  size--;   // make room for '\0'
  for( ; --size >= 0; p++)
  {
    if ((Read(fd, p, 1) == 0) && (p == s))  break; // EOF
    if ( (*p == '\n') || (*p == '\r') )   break;
  }
  *(++p) = '\0';
  return s;
}

int
getline(char *s, int size, int fd)
{
  char *p;

  p = fgets(s, size, fd);
  return p ? strlen(p): -1;
}

char *
strsep(char **str, const char *delim)
{
    char *old;
    
    if (*str == 0)  return 0;
    old = *str;
    for( ; **str; (*str)++)
    {
        if (strchr(delim, **str))
        {
            **str = 0;
            (*str)++;
            return old;
        }
    }
    // 没有找到分隔符
    *str = 0;
    return old;
}


int
iswhitespace(char c)
{
  static const char whitespace[] = " \t\r\n\v\f";
  return strchr(whitespace, c) != 0;
}

// sskip 依赖于iswhitespace()输入'\0'返回0
char *
sskip(const char *start, const char *end)
{
  //printf("Calling sskip(\"%s\")\n", end);
  const char *p;

  p = start;
  /* 如果c='\0', iswhitespace(c)返回零，
   * 故 (c)&&iswhitespace(c) 中第一项是多余的*/
  // 从前往后搜索，直到字符串结尾
  if (end == 0)
  {
    for( ; iswhitespace(*p); p++)
      // printf("skip: \"%s\"\n", p+1);
      ;
  }
  else  // end != 0
  {
    if (start < end)
    {
        for( ; (p < end) && iswhitespace(*p); p++)
        // printf("skip: \"%s\"\n", p+1);
        ;
    }
    else if (start > end)
    {
        // printf("reverse\n");
        p = start - 1;
        for( ; (p >= end) && iswhitespace(*p); p--)
        // printf("skip: \"%s\"\n", p-1)
        ;
        if (p <= end)   p = end;    // 如果全部是空格，p此时等于end-1
    }
  }
  // printf("resulting string is \"%s\"\n", p);
  return (char *)p;
}









// wapper for syscall

void
assert(int cond, char *msg)
{
  if (!cond)
  {
    msg = (msg ? msg : "assertion failed\n");
    write(2, msg, strlen(msg));
    exit(-1);
  }
}

int
Pipe(int p[2])
{
  int r;

  assert(p != 0, "Pipe: null argument\n");
  r = pipe(p);
  // pipe returns 0 if success, -1 if fail
  assert(r >= 0, "Pipe:error\n");
  return r;
}

int
Write(int fd, const void *buf, int cnt)
{
  int realcnt;

  assert(!( (cnt != 0) && (buf == 0) ), "Write: null argument buf");
  realcnt = write(fd, buf, cnt);
  assert(realcnt >= 0, "Write: failed\n");
  return realcnt;
}

int
Read(int fd, void *buf, int cnt)
{
  int realcnt;

  assert(!( (cnt != 0) && (buf == 0) ), "Read: null argument buf");
  realcnt = read(fd, buf, cnt);
  assert(realcnt >= 0, "Read: failed\n");
  return realcnt;
}

int
Writebyte(int fd, const void *buf, int cnt)
{
  int realcnt;

  realcnt = Write(fd, buf, cnt);
  assert(cnt == realcnt, "Writebyte: short count\n");
  return realcnt;
}

int
Readbyte(int fd, void *buf, int cnt)
{
  int realcnt;

  realcnt = Read(fd, buf, cnt);
  if (realcnt == 0)   // EOF
  {
    return 0;
  }
  assert(cnt == realcnt, "Readbyte: short count\n");
  return realcnt;
}

int
Fork(void)
{
  int pid;

  pid = fork();
  assert(pid >= 0, "Fork: failed\n");
  return pid;
}

int
Close(int fd)
{
  // close returns 0 if success, -1 if failed
  assert(close(fd) >= 0, "Close: failed\n");
  return 1;
}