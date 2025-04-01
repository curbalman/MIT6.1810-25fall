// 这个方法不太好, 用了open
// 其他方法参考https://ttzytt.com/2022/07/xv6_lab1_record/
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"

static char *usage = "usage: find path file\n";

void find(const char *const path, const char *const target);

int
main(int argc, char *argv[])
{
    
    assert(argc == 3, usage);
    const char *const path = argv[1];
    const char *const target = argv[2];
    find(path, target);
    
    exit(0);
}

void
find(const char *const path, const char *const target)
{
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;

    if ((fd = open(path, O_RDONLY)) < 0)
    {
        printf("find: cannot open directory \"%s\"\n", (path ? path : "N/A"));
        return;
    }
    if (fstat(fd, &st) < 0)
    {
        printf("find: cannot stat directory \"%s\"\n", (path ? path : "N/A"));
        Close(fd);
        return;
    }
    
    switch(st.type)
    {
        case T_DEVICE:
        case T_FILE:
            printf("find: second argument is file\n%s", usage);
            break;
      
        case T_DIR:
            if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
                printf("find: path too long\n");
                break;
            }
            strcpy(buf, path);
            p = buf+strlen(buf);
            *p++ = '/';
            while(Read(fd, &de, sizeof(de)) == sizeof(de)){
                if(de.inum == 0
                    || strcmp(de.name, ".") == 0
                    || strcmp(de.name, "..") == 0)    continue;
                memmove(p, de.name, DIRSIZ);
                p[DIRSIZ] = 0;
                if(stat(buf, &st) < 0){
                    printf("find: cannot stat %s\n", buf);
                    continue;
                }
                switch (st.type)
                {
                    case T_DEVICE: case T_FILE:
                        if (strcmp(de.name, target) == 0)
                            printf("%s\n", buf);
                        break;
                    case T_DIR:
                        find(buf, target);
                        break;
                }
            }
            break;
    }
        close(fd);
}