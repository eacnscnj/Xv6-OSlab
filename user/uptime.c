#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]){
    if(argc != 1){
        printf("Usage: uptime\n");
        exit(1);
    }

    int ticks = uptime();
    printf("Uptime in terms of ticks: %d\n", ticks);
    exit(0);
}