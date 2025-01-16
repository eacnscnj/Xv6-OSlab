#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]){
    int parentPipe[2],childPipe[2];
    pipe(parentPipe);
    pipe(childPipe);

    int pid = fork();
    if (pid == 0){//child
        char buf[10];
        read(parentPipe[0],buf,10);
        printf("%d: received %s\n",getpid(),buf);
        close(parentPipe[0]);

        write(childPipe[1],"pong",10);
        close(childPipe[1]);

        exit(0);
    } else {//parent
        write(parentPipe[1],"ping",10);
        close(parentPipe[1]);
        char buf[10];

        read(childPipe[0],buf,10);
        printf("%d: received %s\n",getpid(),buf);
        close(childPipe[0]);
        wait(0);
    }
    exit(0);
}