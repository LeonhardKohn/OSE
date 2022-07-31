#include "types.h"

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
  char values[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
  while (1){    
      int counter = 0;
      while(counter < 10){
	syscall(1, (uint64) "Der 2. Prozess befindet sich in Schritt: ");
        syscall(2, (uint64) values[counter]);
	syscall(1, (uint64) "\n");
	counter++;
      }
      syscall(1, (uint64) "Prozess 2 führt jetzt yield() aus.\n");
      syscall(23, 0);
  }
  */

   while(1){
    while (!syscall(10, 0)){
      syscall(1, (uint64)"Process 2 need access to the UART!\n");
      for(int i = 0; i < 100000000; i++);
    }
    char c = syscall(3, 0);
    syscall(11, 0);
    syscall(2, c);
    //syscall(23, 0);
  }

}
