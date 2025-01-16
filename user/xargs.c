#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"
//根据 Unix 约定，命令的名称应该放在 argv[0] 位置。
//因此，newArgv[0] 需要保存 argv[1]，即 xargs 执行的命令名。
// 从标准输入读取一行数据并将其解析为多个参数，返回解析后的参数个数
int dynamicRead(char *new_argv[32], int curr_argc) {
    char buf[1024];  // 用于存储从标准输入读取的数据
    int n = 0;       // 当前读取的字节数
    // 循环读取每个字符，直到读取到换行符或者最大缓冲区大小
    while (read(0, buf + n, 1)) {
        // 如果读取的数据过长，退出
        if (n == 1023) {
            fprintf(2, "argument is too long\n");
            exit(1);
        }
        // 如果读取到换行符，则结束读取
        if (buf[n] == '\n') {
            break;
        }
        n++;  // 增加已读取字节数
    }
    buf[n] = 0;  // 在字符串末尾添加终止符

    // 如果没有读取到任何数据，则返回0
    if (n == 0) return 0;

    int offset = 0;
    // 解析缓冲区中的字符串，将每个单词或参数放入 new_argv 中
    while (offset < n) {
        new_argv[curr_argc++] = buf + offset;  // 将当前参数的起始位置保存到 new_argv 中
        // 跳过当前单词的字符
        while (buf[offset] != ' ' && offset < n) {
            offset++;
        }
        // 跳过空格
        while (buf[offset] == ' ' && offset < n) {
            buf[offset++] = 0;  // 将空格替换为字符串终止符
        }
    }
    return curr_argc;  // 返回当前已解析的参数个数
}
int main(int argc, char const *argv[]) {
    // 检查传入的参数是否正确
    if (argc <= 1) {
        fprintf(2, "Usage: xargs command (arg ...)\n");
        exit(1);
    }
    char *cmd = malloc(strlen(argv[1])+1);  // 获取命令名
    strcpy(cmd, argv[1]);
    char *newArgv[MAXARG] = {0};  // 保存解析后的参数
    int argcNum = 0;  // 保存解析后的参数个数

    for(int i = 1; i < argc; i++){
        newArgv[i-1] = malloc(strlen(argv[i]) + 1);
        strcpy(newArgv[i-1] , argv[i]);
    }
    while((argcNum = dynamicRead(newArgv,argc - 1))!=0){
        newArgv[argcNum] = 0;
        if(fork() == 0){
            exec(cmd, newArgv);
            fprintf(2,"xargs: %s failed\n", cmd);
            exit(1);
        }
        else{
            wait(0);
        }
    }

    
    exit(0);  // 结束程序
}