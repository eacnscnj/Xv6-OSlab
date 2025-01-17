#include "kernel/types.h"
#include "kernel/param.h"
#include "user/user.h"
#include "kernel/sysinfo.h"

int main(int argc, char *argv[]) {
  struct sysinfo info;
  if(argc != 1) {
    fprintf(2, "Usage: sysinfo\n");
    exit(1);
  }
  sysinfo(&info);
  printf("free space:%d, used process num:%d\n", info.freemem,info.nproc);
  exit(0);
}