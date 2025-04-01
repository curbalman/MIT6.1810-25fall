#include "kernel/types.h"
// kerbal/stat.h
struct stat;

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

// umalloc.c
void* malloc(uint);
void free(void*);

// utils
void assert(int cond, char *msg);
int Writebyte(int fd, const void *buf, int cnt);    // write/read exactly cnt bytes
int Readbyte(int fd, void *buf, int cnt);
// read at most size-1 chars(including '\n')
// return s on success, and NULL when end of file occurs while no characters have been read, exit() if error.
// s is null-terminated
char *fgets(char *s, int size, int fd);
// read at most size-1 chars(including '\n')
// return the number of characters read, -1 if EOF, exit() if error
// s is null-terminated
int getline(char *s, int size, int fd);
/* Finds the first token in *str delimited by one of the bytes in string delim.
 * *str is modified.
 * If *str is null, does nothing and returns null.
 * str and delim is non-nullable.
*/
char *strsep(char **str, const char *delim) __attribute__((nonnull, returns_nonnull));
// return 0 if c is '\0'
int iswhitespace(char c);
// return ptr to the first non-space char or '\0'
// start is not nullable
// start must be null-terminated if end==0
char *sskip(const char *start, const char *end) __attribute__((nonnull(1)));


// wapper for syscall
int Pipe(int [2]);
int Write(int fd, const void *buf, int n);
int Read(int fd, void *buf, int n);
int Fork(void);
int Close(int fd);

