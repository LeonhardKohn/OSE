#include "types.h"
#include "riscv.h"
#include "hardware.h"


extern int main(void);
extern void ex(void);
extern void printstring(char *s);
extern int rb_write(char c);
extern int rb_read(char *c);

/*
 * - __attribute__: weißt einer Struktur bestimmte Eigenschaften zu, in diesem Fall dem Stack des Kernels
 * - aligned(16): Die Adresse in der sich der Kernelstack befindet, sollte durch 16 teilbar sein. 
 * - Hier wird der Kernelstack definiert mit einer Größe von 16 * 4096 = 65536 Bytes 
 */
__attribute__ ((aligned (16))) char kernelstack[4096];

/*
 * Funktionsdeklaration
 */
void printhex(uint64);
void putachar(char);
void copyprog(int);

/*
 * - PCB: Definition des Program-Control-Blocks, zum speichern der Zwischenzustände
 * - uart: Die Struktur UART wird definiert (beinhaltet Register, um Werte zu printen bzw. auszulesen), welches sich an Adresse 0x10000000 befindet
 */
volatile struct uart* uart0 = (volatile struct uart *)0x10000000;
PCB pcb[2];
// Aktueller Prozess
int current_process = 0;

uartlock lock[1];
// Intervallgröße für einen Timer-Interrupt
extern int timer_interval;

// Diese Funtkion überträgt einen String auf den Bildschirm / Konsole.
void printstring(char *s) {
    while (*s) {     			// as long as the character is not null
        putachar(*s); 			// output the character
        s++;         			// and progress to the next character
    }
}

// Diese Funktion überträgt einen Character auf den Bildschirm / Konsole.
void putachar(char c) {
    while ((uart0->LSR & (1<<5)) == 0); // do nothing - wait until bit 5 of LSR = 1
    uart0->THR = c; 			// then write the character
}

// Diese Funktion liest einen Character von der Tastatur.
char readachar(void) {
    char c;
    rb_read(&c);
    return c; 			// then read the character
}

// Memory protection wird umgestellt, damit ein Userprozess nicht auf andere Userprozesse zugreifen darf 
void config_pmp(void){
    if (current_process == 0){
        w_pmpcfg0(0x00000f0000);
    }else{
        w_pmpcfg0(0x000f000000);
    } 
}

// Diese Funktion speichert den aktuellen Status des laufenden Prozesses, um zu wechseln
void yield(uint64 pc, uint64 s){
    pcb[current_process].sp = s;
    pcb[current_process].pc = pc;

    current_process++;
    if (current_process > 1) current_process = 0;
    
    if (pcb[current_process].blocked == 0){
       config_pmp();
    }else{
       current_process++;
       if (current_process > 1) current_process = 0;
    }
}

// Diese Funktion ist für das beenden von Programmen notwendig. Hier nur Anfangszustand herstellen.
void exiit(void){
    pcb[0].pc = 0x80100000;
    pcb[0].sp = 0x80102000;
    pcb[1].pc = 0x80200000;
    pcb[1].sp = 0x80202000; 
    
    current_process++;
    if (current_process > 1) current_process = 0;

    config_pmp();
}

void close_uart(void){
   if (lock->lockvalue == 0){
      lock->lockvalue = 1;
      lock->process = current_process;
   }else{
      printstring("The UART is locked by another process!\n");
   }
}

void open_uart(void){
   lock->lockvalue = 0;
}

// Diese Funktion ändert die Obergrenze für den Timer-Interrupt
void interrupt_handler(stackframe **s, uint64 *pc){
    if ((r_mcause() & 255) == 11){
       uint32 irq = *(uint32*)PLIC_CLAIM;
       switch(irq){
	  case UART_IRQ:
	      ;
	      uint8_t uart_irq = uart0->IIR;
	      char c = uart0->RBR;
          rb_write(c);
          if (pcb[0].blocked == 1){
              pcb[0].blocked = 0;
	      }else if (pcb[1].blocked == 1){
              pcb[1].blocked = 0;
	      }	      
	      break;
	  default:
	      printstring("Unknown Interupt\n");
	      break;
       }
       *(uint32*)PLIC_COMPLETE = irq;
       *pc = *pc - 4;
    }else if ((r_mcause() & 255) == 7){
       //printstring("INTERRUPT\n");
       *(uint64*)CLINT_MTIMECMP(0) = *(uint64*)CLINT_MTIME + timer_interval;
       *pc = *pc - 4;   
       yield(*pc, (uint64) s);
       printhex(current_process);
       *pc = pcb[current_process].pc;
       *s = (stackframe*) pcb[current_process].sp;
    }else{
       printstring("Unknown Interrupt\n");
    }
}

// Diese Funktion gibt den Fehler auf die Konsole aus.
void exception_handler(void){
    printstring("mcause:");
    printhex(r_mcause());
    printstring("mepc:");
    printhex(r_mepc());
    printstring("mtval:");
    printhex(r_mtval());
}

// Diese Funktion printet den Hex-wert eines Parameters aus.
void printhex(uint64 x) {
  int i;
  char s[2];
  s[1] = 0;

  printstring("0x");
  for (i=60; i>=0; i-=4) {
    int d =  ((x >> i) & 0x0f);
    if (d < 10)
      s[0] = d + '0';
    else
      s[0] = d - 10 + 'a';
    printstring(s);
  }
  printstring("\n");
}


// This is the C code part of the exception handler
// "exception" is called from the assembler function "ex" in ex.S with registers saved on the stack
void exception(stackframe *s) {
  /*
   * nr		-	Die Nummer des Syscalls der ausgeführt werden soll.
   * param	-	Der Parameter eines Syscalls der gegebenenfalls verarbeitet wird.
   * retval	-	Der Rückgabewert der dem User-Prozess zurückgegeben werden soll
   * pc		-	Der aktuelle Program-Counter
  */
  uint64 nr = s->a7;
  uint64 param = s->a0;
  uint64 retval = 0;
  uint64 pc = r_mepc();

  // Exception-Handling + Interrupt Handling
  if ((r_mcause() & (1ULL<<63)) != 0){
    if ((r_mcause() & 255) == 11){
       uint32 irq = *(uint32*)PLIC_CLAIM;
       switch(irq){
	  case UART_IRQ:
	      ;
	      uint8_t uart_irq = uart0->IIR;
	      char c = uart0->RBR;
              rb_write(c);
	      //printhex(lock->process);
	      int value = lock->process;
              pcb[value].blocked = 0;	      
	      break;
	  default:
	      printstring("Unknown Interupt\n");
	      break;
       }
       *(uint32*)PLIC_COMPLETE = irq;
       pc = pc - 4;
    }else if ((r_mcause() & 255) == 7){
       //printstring("INTERRUPT\n");
       //printhex(current_process);
       *(uint64*)CLINT_MTIMECMP(0) = *(uint64*)CLINT_MTIME + timer_interval;
       pc = pc - 4;   
       yield(pc, (uint64) s);
       pc = pcb[current_process].pc;
       s = (stackframe*) pcb[current_process].sp;
    }else{
       printstring("Unknown Interrupt\n");
    }
     //interrupt_handler(s, &pc);
   }else if (r_mcause() != 8){
     exception_handler();    
     return;
  }else{
 

    // decode syscall number
    switch (nr) {
      case 1: 
    	      printstring((char *)param);
              break;
      case 2: 
	      putachar((char)param);
              break;
      case 3: 
	      if (lock->lockvalue == 1 && lock->process == current_process){
                 pcb[current_process].blocked = 1;
		 retval = readachar();
	      }else{
                 printstring("You need to lock the UART!\n");
	      }
	      break;
      case 10:
	      close_uart();
	      break;
      case 11:
	      open_uart();
	      break;
      case 23:
	      yield(pc, (uint64) s);
 	      pc = pcb[current_process].pc;
	      s = (stackframe*) pcb[current_process].sp;
	      break;
      case 42: 
              exiit();
	      pc = pcb[current_process].pc - 4;
              break;
      default: 
	      printstring("Invalid syscall: "); 
	      printhex(nr); 
	      printstring("\n");
              break;
    }

  }
  // adjust return value - we want to return to the instruction _after_ the ecall! (at address mepc+4)
  w_mepc(pc+4);

  // pass the return value back in a0
  if (r_mcause() != 8 && (r_mcause() & (1ULL<<63)) != 0){
     asm volatile("mv a0, %0" : : "r" (retval)); 
  }else{
     s->a0 = retval;
  }
  
  // this function returns to ex.S

  asm volatile("mv a1, %0" : : "r" (s));

}
