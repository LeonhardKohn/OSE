#include "types.h"

// TODO 3.1: add a data structure for the user stack
__attribute__ ((aligned (16))) char userstack[4096];

uint64 syscall(uint64 nr, uint64 param) {
    uint64 retval;
    asm volatile("mv a7, %0" : : "r" (nr));
    asm volatile("mv a0, %0" : : "r" (param));
    asm volatile("ecall");
    asm volatile("mv %0, a0" : "=r" (retval) );
    return retval;
}

int main(void) {
    /*
    char values [] = {'0','1','2','3','4','5','6','7','8','9'};
    while(1){
        int count = 0;
        while (count<10)
        {
            syscall(1,"Prozess 2 befindet sich im schritt: ");
            syscall(2, values[count]);
            syscall(1,"\n");
            //uint64 test = *(char*) 0x8010000; //kleiner Test für die Memory Protection
            //syscall(1,test);
            count++;
        }
        syscall(1,"YIELD!!!\n");
        syscall(23,0);
        
    }
    */

   while (1)
   {
    #if 1
    char c;
    for(int i=0;i<100000000;i++);
    syscall(10,0);
    c = syscall(3,0);
    syscall(2,'2');
    syscall(2,c);
    syscall(11,0);
    //syscall(23,0);
    #endif
    //syscall(2,'2');
    //for(int i=0;i<100000000;i++);    
   }
   

    
    
}
