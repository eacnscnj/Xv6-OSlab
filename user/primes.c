#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void isPrime(int pLeft){
    int passPrime = 0;
    int myPrime = 0;
    int forked = 0;
    int pRight[2];

    while(1){
        int stateNum = read(pLeft, &passPrime, sizeof(myPrime));
        if(stateNum == 0){
            close(pLeft);
            if(forked){
                close(pRight[1]);
                wait(0);
            }
            exit(0);
        }

        if(!myPrime){
            printf("prime %d\n", passPrime);
            myPrime = passPrime;
        }

        if(passPrime % myPrime != 0){
            if(!forked){
                pipe(pRight);
                forked = fork();
                if(forked){
                    close(pRight[0]);
                }else{
                    close(pLeft);
                    close(pRight[1]);
                    isPrime(pRight[0]);
                }
            }
            write(pRight[1], &passPrime, sizeof(passPrime));
        }
    }
}

int main(int argc, char *argv[]){
    if(argc != 1){
        printf("Usage: primes\n");
        exit(1);
    }
    int p[2];
    pipe(p);
    for(int i = 2; i < 35; i++){
        write(p[1], &i, sizeof(i));
    }
    close(p[1]);
    isPrime(p[0]);
    exit(0);
}