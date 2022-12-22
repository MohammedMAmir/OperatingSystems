#include "common.h"
#include "stdlib.h" 

int factorial(int x);

int
main(int argc, char *argv[])
{
    if(argc > 2){
	    printf("Huh?\n");
    }else if(atoi(argv[1]) == 0 || atoi(argv[1]) < 1){
        printf("Huh?\n");
    }else if(atoi(argv[1]) > 12){
        printf("Overflow\n");
    }else{
        char *ptr;
        strtol(argv[1],&ptr, 10);
        if(ptr[0] == '.'){
            printf("Huh?\n");
        }else{
            printf("%d\n", factorial(atoi(argv[1])));
        }
    }
	return 0;
}

int factorial(int x){
    if(x==1){
        return 1;
    }else{
        return(x*(factorial(x-1)));
    }
}
