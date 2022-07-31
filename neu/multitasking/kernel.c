#include "types.h"
#include "riscv.h"
#include "hardware.h"

#include "uart.h"
#include "uartlock.h"

extern int main(void);
extern void ex(void);
extern void printstring(char *s);
extern int rb_write(char c);
extern int rb_read(char *c);
extern int is_full();
extern int is_empty();
extern int interval;
extern uartlock lock; // aus uartlock.c

/** definieren den UART und setzen ihn auf den Pointer 0x10000000 um den offset zu definiren.
 * Wichtig lernen! */
extern volatile struct uart *uart0;

/** registriert sich mindestens 16 bytes
 * char kernelstack[4096] : minimum 16 bytes (anfangsadress muss durch 16 teilbar sein)
 * registrieren uns auf dem Stack einen gewissen Speicherplatz */

// Kommentar von Christian:
/*
 * - __attribute__: weißt einer Struktur bestimmte Eigenschaften zu, in diesem Fall dem Stack des Kernels
 * - aligned(16): Die Adresse in der sich der Kernelstack befindet, sollte durch 16 teilbar sein.
 * - Hier wird der Kernelstack definiert mit einer Größe von 16 * 4096 = 65536 Bytes
 */
__attribute__((aligned(16))) char kernelstack[4096];

/** definiert die Header der Funktionen */
void printhex(uint64);
void putachar(char);
void copyprog(int);
void panic(char *);
void config_pmp();

/** schaut welchen Prozess momentan läuft */
int current_process = 0;
/** erstelle ein Array mit der größe Zwei. In diesem Array werden die beiden Prozesse gespeichert */
PCBs pcb[2];

uint64 tmpRead = 0;

//----------------------------------------------------------------------//

//-----------------syscalls-----------------//

void exception_handler()
{
  printstring("\nERROR!\n");
  printstring("r_mcause: ");
  printhex(r_mcause());
  printstring("r_mepc: ");
  printhex(r_mepc());
  printstring("r_mtval: ");
  printhex(r_mtval());

  panic("Unknown Error\n");
}

uint64 yield(stackframes **s, uint64 pc)
{
  // Process wechseln
  if (pcb[current_process].state == RUNNING)
  {
    pcb[current_process].state = READY;
  }
  pcb[current_process].pc = pc; // save pc, stack pointer
  pcb[current_process].sp = (uint64)*s;
  current_process++;
  if (current_process > 1) // da nur zwei Processe vorhanden sind
    current_process = 0;

  if (pcb[current_process].state == READY)
  {
    // ok
  }
  else
    while (pcb[current_process].state == BLOCKED)
    {
      current_process++;
      if (current_process > 1) // da nur zwei Processe vorhanden sind
        current_process = 0;
    }
  config_pmp();
  pc = pcb[current_process].pc; // add +4 later!
  *s = (stackframes *)pcb[current_process].sp;
  pcb[current_process].state = RUNNING;

  return pc;
}

// Memory protection wird umgestellt, damit ein Userprozess nicht auf andere Userprozesse zugreifen darf
void config_pmp(void)
{
  if (current_process == 0)
  {
    w_pmpcfg0(0x00000f0000); // Switch memory protection to new process
  }
  else
  {
    w_pmpcfg0(0x000f000000); // only access to Process 1// 2 - full access; 1,0 - no access
  }
}

uint64 sys_exit(stackframes **s, uint64 pc)
{
  if (current_process == 0)
  {
    pcb[0].pc = 0x80100000;
    pcb[0].sp = 0x80102000;
  }
  else
  {
    pcb[1].pc = 0x80200000;
    pcb[1].sp = 0x80202000;
  }

  if (current_process > 1)
    current_process = 0;

  pc = pcb[current_process].pc;

  return pc;
}

//------------------------interrupt handler-------------------------//
uint64 handle_interrupts(stackframes **s, uint64 pc)
{
  switch ((r_mcause() & 255))
  {
  case 7: //-----------------Timer Interrupt---------------//
    putachar('I');
    pc = yield(s, pc);
    pc = pc - 4;
    *(uint64 *)CLINT_MTIMECMP(0) = *(uint64 *)CLINT_MTIME + interval;
    break;
  case 11:
      //-------------------------Hardware Interrupt-----------------//
      
      ;                                 // leere Instruktion
    uint32 irq = *(uint32 *)PLIC_CLAIM; // get highest prio interrupt nr
    switch (irq)
    {
    case UART_IRQ:                  // #define UART_IRQ 10
        ;                           // leere Instruktion
      uint32 uart_irq = uart0->IIR; // read UART interrupt source
      char c = uart0->RBR;
      rb_write(c);         // schreibe das Zeichen in den Ringbuffer
      
      if (pcb[0].state == BLOCKED)
      {
        pcb[0].state = READY;
      } else if (pcb[1].state == BLOCKED)
      {
        pcb[1].state = READY;
      }
      
      else if (is_full())
        {

          //putachar(readachar());
        }

      *(uint32*)PLIC_COMPLETE = irq;
       pc = pc - 4;

      break; // (clears UART interrupt)

    default:
      printstring("Unknown Hareware interrupt!");
      printhex(r_mcause());
      printhex(irq);
      break;
    }

    *(uint32 *)PLIC_COMPLETE = UART_IRQ; // announce to PLIC that IRQ was handled
    break;

  default:
    printstring("Unknown interrupt!");
    printhex(r_mcause());
    break;
  }
  return pc;
}
//------------------------exeption handler-------------------------//

//---------------panic---------------//
void panic(char *errorCode)
{
  printstring(errorCode);
  while (1)
  {
  }
}

//--------------------------------------exception handler------------------------------//

// This is the C code part of the exception handler
// "exception" is called from the assembler function "ex" in ex.S with registers saved on the stack
void exception(stackframes *s)
{
  uint64 nr = s->a7;    // read syscall number from stack;
  uint64 param = s->a0; // read parameter from stack;
  uint64 sp;
  uint64 pc = r_mepc(); // read exception PC;
  uint64 retval = 0;
  // überprüfen, ob ein Zeichen vorliegt
  // überprüfen, ob ein Prozess vorliegt, der busy ist

  if ((r_mcause() & (1ULL << 63)) != 0)
  { // lieg ein asyncroner Interrupt vor?
    pc = handle_interrupts(&s, pc);
  }
  //-----------------------------Fehler behandlung---------------------//
  else if (r_mcause() != 8)
  {
    exception_handler();    
  } // Überprüfen, ob der Prozess BLOCKED ist
  else /*if (pcb[current_process].state == BLOCKED)
  {
    pc = yield(&s, pc);
  }
  else*/
  {
    switch (nr)
    {
    case 1:
      printstring((char *)param);
      break;
    case 2:
      putachar((char)param);
      break;
    case 3:
	    if (lock.locked == 1 && lock.process == current_process){
	      retval = readachar();
        if (retval == 0){
          pcb[current_process].state = BLOCKED;
          pc = yield(&s,pc);
        }
	    }else{
        printstring("You need to lock the UART!\n");
	    }

      // current process muss busy sein
      // nächsten Process nehmen der ready ist (passiert in yield)
      // bei jeder exeption auf ready/ busy überprüfen
      break;
    case 10:;
      int return_Uart = close_uart();
      if (return_Uart == 2)
      {
        printstring("Error! Uart was already closed by another process\n");
      }
      break;
    case 11:
      open_uart();
      break;
    case 23:
      // yield system call
      pc = yield(&s, pc);
      break;
    case 42: // user program returned, starting from the beginning
      pc = sys_exit(&s, pc);
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

  /*if (current_process == 1 && nr == 10){
    printhex(retval);
  }*/
  
  // this function returns to ex.S
  
  asm volatile("mv a1, %0" : : "r" (s));

  // this function returns to ex.S
}
