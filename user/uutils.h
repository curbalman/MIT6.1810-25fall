#include "user/user.h"

// uutils.c
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
