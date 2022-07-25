#include "types.h"
#include "uartlock.h"
#include "riscv.h"
#include "hardware.h"
#include "uart.h"

extern int current_process;
extern void panic(char *c);

void initlock(char *name){
  char *name = name;
  int locked = 0;
  int process = 0; 
}

// Acquire the lock.
// Loops (spins) until the lock is acquired.
void acquire(spinlock *lk){
    //asm("cli");
  
}

// Release the lock.
void release(spinlock *lk){
  //asm ("sti");

}

// Check whether this process is holding the lock.
// Interrupts must be off.
int holding(spinlock *lk){
  if (lk->locked==1)
  {
    return 1;
  }else{
    return 0;
  } 
}

void uart_open(spinlock *lk)
{
  if (holding(lk))
  {
    panic("uart is locked from Process " + *(char *)lk->name);
  }
  else
  {
    lk->locked = 1;
    lk->name = "uart";
    lk->process = current_process;
  }
}
/**
 * A process that wants to read characters from the UART
 * now has to open it before and close it afterwards.
 * Only a single process may access the UART at the same
 * time, so trying to call open_uart when another process
 * is using the UART will result in an error
 */

// TODO uart_close()
void uart_close(spinlock *lk)
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




// push_off/pop_off are like intr_off()/intr_on() except that they are matched:
// it takes two pop_off()s to undo two push_off()s.  Also, if interrupts
// are initially off, then push_off, pop_off leaves them off.






