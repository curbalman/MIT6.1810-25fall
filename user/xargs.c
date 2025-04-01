#include "kernel/param.h"
#include "user/uutils.h"

#define MAXLINE 1024

void parseargs(int argc, char *argv[], char *s);

int
main(int argc, char *argv[])
{   
    assert(argc >= 2, "Usage: xargs command initial-arguments\n");
    char s[MAXLINE], *execargv[MAXARG], *execcmd;
    int argindex;

    execcmd = argv[1];
    // copy initial args to execargv[]
    //printf("***Building initial argument: start***\n");
    for(argindex = 0; argindex < argc-1; argindex++)
    {
        execargv[argindex] = argv[argindex+1];
        //printf("execargv[%d] = argv[%d] = %s\n", argindex, argindex+1, argv[argindex+1]);
    }
    //printf("***Building initial argument: finish***\n");
    // argindex现在是initial args的数量
    for(memset(s, '\0', MAXLINE); getline(s, MAXLINE, 0) > 0; memset(s, '\0', MAXLINE))
    {
        if (s[0] == '\n')   continue;   // 忽略空行
        // printf("s[MAXLINE-2] = \"%c\"(%d)\n", s[MAXLINE-2], s[MAXLINE-2]);
        assert((s[MAXLINE-2]=='\0') || (s[MAXLINE-2]=='\n'), "xargs: input line too long\n");
        memset(execargv + argindex, 0, MAXARG - argindex);  // execargv清零
        parseargs(MAXARG - argindex - 1, execargv + argindex, s);   // execargv应该以零元素结尾，故减一
        if (fork() == 0)
        {
            //printf("%s -%s--%s--%s-\n", execargv[0], execargv[1], execargv[2], execargv[3]);
            exec(execcmd, execargv);
            fprintf(2, "xargs: %s: No such file\n", execcmd);
            exit(1);
        }
        else
        {
            wait(0);
        }
        
    }
    exit(0);
}

void
parseargs(int argc, char *argv[], char *s)
{
    int i;

    //printf("***Parsing args: start***\n");
    s = sskip(s, 0);
    for (i = 0; (*s) && (i < argc); i++)
    {
        argv[i] = strsep(&s, " \t\r\n\v\f");
        //printf("token: \"%s\"(%d)\n", argv[i], (int)argv[i][0]);
        //printf("before skip: -%s-\n", s);
        s = sskip(s, 0);    // 跳过连续的空白
        //printf("after skip: -%s-\n", s);
        
    }
    //printf("\n***Parsing args: stop***\n");
}
