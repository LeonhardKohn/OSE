#include "types.h"
#include "uartlock.h"
#include "riscv.h"
#include "hardware.h"
#include "uart.h"

extern int current_process;
extern void panic(char *c);
uartlock lock;              // erstellt struktur

void initlock(){
  lock.locked = 0;
  lock.process = -1; 
}

// TODO Acquire the lock.
// Loops (spins) until the lock is acquired.
void acquire(){
    //asm("cli");
}
// TODO Release the lock.
void release(){
  //asm ("sti");
}

/**
 * Ein Prozess, der Zeichen aus dem UART lesen will,
 * muss ihn nun vorher öffnen und danach schließen.
 * Nur ein einziger Prozess kann gleichzeitig auf den UART zugreifen,
 * so dass der Versuch, open_uart aufzurufen, wenn ein anderer Prozess
 * den UART benutzt, zu einem Fehler führt.
 */

int close_uart(void){
   if (lock.locked == 0){         // wenn offen, dann schließe den UART
      lock.locked= 1;
      lock.process = current_process;
      return 1;                  // uart ist jetzt zu 
   }else{
      return 2;                  // uart war schon vorher zu (Fehlerbehandlung)
   }
}

void open_uart(void){
   lock.locked = 0;   
}

// Prüfen, ob dieser Prozess den lock hält.
int holding(void){
  if (lock.locked==1)
  {
    return 1;
  }else{
    return 0;
  } 
}