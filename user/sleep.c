#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    char *ticks, *end;

    if(argc <= 1 || argc >= 3){
        fprintf(2, "usage: sleep number\n");
        exit(1);
    }
    ticks = argv[1];
    end = ticks + strlen(ticks);
    for (char *s = ticks; s < end; s++)
    {
        if(*s <= 0 || *s >= '9')
        {
            fprintf(2, "ticks must be a number\n");
            exit(1);
        }
    }
    sleep(atoi(ticks));
    exit(0);
}