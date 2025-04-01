#include "kernel/types.h"
#include "user/uutils.h"

#define START 2
#define END   35


void procunit(int infd); //__attribute__((noreturn));

int
main(int argc, char *argv[])
{
    assert(END >= START, "END must >= START\n");
    int pline[2];

    Close(0);
    Pipe(pline);
    if (Fork() == 0)
    {
        Close(pline[1]);
        procunit(pline[0]);
        Close(pline[0]);
        return 0;
    }
    Close(pline[0]);    // close read end
    // Close(1);           // close stdout after procunit()
    // number feeder
    if (Fork() == 0)
    {
        // Close(0); Close(1); Close(2);
        for(int i = START; i <= END; i++)
        {
            Writebyte(pline[1], &i, sizeof(i));
        }
        // printf("numgen\n");
        return 0;
    }
    Close(pline[1]);
    wait(0);
    wait(0);
    return 0;
}

void
procunit(int infd)
{
    
    int base, pline[2], x;
    // 输出素数，如果是最右边的单元则关闭
    if (Readbyte(infd, &base, sizeof(base)))
    {
        printf("prime %d\n", base);
        //cannot Close(1) here, child needs stdout
    }
    else    // EOF
    {
        // printf("%s\n", "Last unit closed\n");
        return;
    }
    Pipe(pline);
    if(Fork() == 0)
    {
        Close(pline[1]);    // child: close write end of pipe
        procunit(pline[0]);
        Close(pline[0]);
    }
    else
    {
        // Close(1);
        Close(pline[0]);    // parent: close read end of pipe
        // printf("%d\n", -base * 100);
        while(Readbyte(infd, &x, sizeof(x)))
        {
            if (x%base == 0) continue;
            //printf("%d --(%d)-->\n", x, base);
            Writebyte(pline[1], &x, sizeof(x));     // send x to child
        }
        // printf("%d\n", -base * 1000);
        Close(pline[1]);
        wait(0);
    }
    
    return;
}

