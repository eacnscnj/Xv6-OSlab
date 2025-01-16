#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

void find(char *path, char *filename) {
    char buf[512];
    char *p;
    int fd;
    struct dirent de;
    struct stat st;

    // 打开路径对应的文件或目录
    if((fd = open(path, 0)) < 0){
        fprintf(2, "find: cannot open %s\n", path);
        exit(1);
    }

    // 获取文件或目录的状态
    if(fstat(fd, &st) < 0){
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        exit(1);
    }

    switch (st.type){
    case T_FILE:
        fprintf(2, "find: please put path instead of filename %s\n", path);
        exit(1);
        break;
    
    case T_DIR:
        if(strlen(path) + 1 + DIRSIZ + 1 > sizeof(buf)){
            printf("find: path too long\n");
            //close(fd);
            break;
        }
        strcpy(buf, path);
        p = buf + strlen(buf);
        *p++ = '/';
        while(read(fd, &de, sizeof(de)) == sizeof(de)){
            if(de.inum == 0)  // 跳过无效目录项
                continue;
            if(strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
                continue;
            memmove(p, de.name, DIRSIZ);
            p[DIRSIZ] = 0;
            if(stat(buf, &st) < 0){
                printf("find: cannot stat %s\n", buf);
                continue;
            }
            if (st.type == T_DIR){
                find(buf, filename);
            } else if (st.type == T_FILE && strcmp(de.name, filename) == 0){
                printf("%s\n", buf);
            } 
        }
        break;
    }
    close(fd);
}

int main(int argc, char *argv[]) {
    if(argc != 3){
        fprintf(2, "Usage: find <path> <filename>\n");
        exit(1);
    }
    char *path = argv[1];
    char *target = argv[2];
    find(path, target);
    exit(1);
}