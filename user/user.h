#include "kernel/types.h"
#ifdef LAB_MMAP
typedef unsigned long size_t;
typedef long int off_t;
#endif

struct stat;  // kerbal/stat.h

// system calls
int fork(void);
int exit(int) __attribute__((noreturn));
int wait(int*);
int pipe(int*);
int write(int, const void*, int);
int read(int, void*, int cnt);              // return bytes read, -1 if EOF or failure, 0 if cnt < 0
int close(int);
int kill(int);
int exec(const char*file, char** argv);
int open(const char*, int);
int mknod(const char*, short, short);
int unlink(const char*);
int fstat(int fd, struct stat*);
int link(const char*, const char*);
int mkdir(const char*);
int chdir(const char*);
int dup(int);
int getpid(void);
char* sbrk(int);
int sleep(int);
int uptime(void);
int trace(int);
#ifdef LAB_NET
int bind(uint32);
int unbind(uint32);
int send(uint32, uint32, uint32, char *, uint32);
int recv(uint32, uint32*, uint32*, char *, uint32);
#endif
#ifdef LAB_PGTBL
int ugetpid(void);
uint64 pgpte(void*);
void kpgtbl(void);
#endif

// ulib.c
int stat(const char*, struct stat*);
char* strcpy(char*, const char*);
void *memmove(void*, const void*, int);
// return null if c is 0, s is non-nullable
char* strchr(const char *s, char c) __attribute__((nonnull));
int strcmp(const char*, const char*);
void fprintf(int, const char*, ...) __attribute__ ((format (printf, 2, 3)));
void printf(const char*, ...) __attribute__ ((format (printf, 1, 2)));
char* gets(char*, int max);
// return is limited to MAX_INT, not uint
uint strlen(const char*);
// set n bytes starting from dst to (char)c
void* memset(void *dst, int c, uint n);
int atoi(const char*);
int memcmp(const void *, const void *, uint);
void *memcpy(void *, const void *, uint);
#ifdef LAB_LOCK
int statistics(void*, int);
#endif

// umalloc.c
void* malloc(uint);
void free(void*);
