#include "types.h"
#include "uartlock.h"
#include "riscv.h"
#include "hardware.h"
#include "uart.h"

extern int current_process;
extern void panic(char *c);
uartlock lock;
#if 0
void initlock(char *name, uartlock *lock_uart){
  char *name = name;
  int locked = 0;
  int process = 0; 
}


// Acquire the lock.
// Loops (spins) until the lock is acquired.
void acquire(uartlock *lk){
    //asm("cli");
  
}

// Release the lock.
void release(uartlock *lk){
  //asm ("sti");

}

// Check whether this process is holding the lock.
// Interrupts must be off.
int holding(uartlock *lk){
  if (lk->locked==1)
  {
    return 1;
  }else{
    return 0;
  } 
}

void uart_open(uartlock *lk)
{
  if (holding(lk))
  {
    panic("uart is locked from Process " + lk->process);
  }
  else
  {
    lk->locked = 1;
    lk->process = current_process;
  }
}
#endif
/**
 * A process that wants to read characters from the UART
 * now has to open it before and close it afterwards.
 * Only a single process may access the UART at the same
 * time, so trying to call open_uart when another process
 * is using the UART will result in an error
 */
#if 0
// TODO uart_close()
void uart_close(uartlock *lk)
{
  if (lk->locked == 0)
  {
    printstring("uart is already closed");
  }
  else
  {
    lk->locked = 0;
    lk->name = "uart";
    lk->process = current_process;
  }
}

#endif


int close_uart(void){
   if (lock.locked == 0){
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


// push_off/pop_off are like intr_off()/intr_on() except that they are matched:
// it takes two pop_off()s to undo two push_off()s.  Also, if interrupts
// are initially off, then push_off, pop_off leaves them off.






