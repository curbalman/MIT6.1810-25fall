#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    uchar a, b;
    int p2c[2], c2p[2];

    a = '\xff';
    b = '\xff';
    pipe(p2c);
    pipe(c2p);
    if (fork() == 0)
    {
        // close(0); dup(p2c[0]);      // file descriptor 0 refer to the read end of the pipe p2c
        // close(1); dup(c2p[1]);      // file descriptor 1 refer to the write end of the pipe c2p
        // close(p2c[0]); close(p2c[1]);
        // close(c2p[0]); close(c2p[1]);
        if (read(p2c[0], &a, sizeof(a)))
        {
            printf("%d: received ping\n", getpid());
        }
        write(c2p[1], &b, sizeof(b));
    }
    else
    {
        write(p2c[1], &a, sizeof(a));
        if (read(c2p[0], &b, sizeof(b)))
        {
            printf("%d: received pong\n", getpid());
        }
    }
    exit(0);
}