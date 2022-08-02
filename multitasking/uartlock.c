#include "types.h"
#include "uartlock.h"
#include "riscv.h"
#include "hardware.h"
#include "uart.h"

extern int current_process;
extern void panic(char *c);
uartlock lock;

void initlock(){
  lock.locked = 0;
  lock.process = -1; 
}

//TODO
// Acquire the lock.
// Loops (spins) until the lock is acquired.
void acquire(){
    //asm("cli");
  
}
//TODO
// Release the lock.
void release(){
  //asm ("sti");

}
/**
 * A process that wants to read characters from the UART
 * now has to open it before and close it afterwards.
 * Only a single process may access the UART at the same
 * time, so trying to call open_uart when another process
 * is using the UART will result in an error
 */

int close_uart(void){
   if (lock.locked == 0){ //wenn offen, dann schließe den UART
      lock.locked= 1;
      lock.process = current_process;
      return 1; //uart ist jetzt zu 
   }else{
      return 2; // uart war schon vorher zu (Fehlerbehandlung)
   }
}

void open_uart(void){
   lock.locked = 0;   
}

// Check whether this process is holding the lock.
// Interrupts must be off.
int holding(void){
  if (lock.locked==1)
  {
    return 1;
  }else{
    return 0;
  } 
}


// push_off/pop_off are like intr_off()/intr_on() except that they are matched:
// it takes two pop_off()s to undo two push_off()s.  Also, if interrupts
// are initially off, then push_off, pop_off leaves them off.






