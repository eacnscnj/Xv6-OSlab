#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]){
    if(argc != 2){
        printf("Usage: sleep <number>\n");
        exit(1);
    }
    char *num = argv[1];
    int n = atoi(num);
    sleep(n);
  exit(0);
}