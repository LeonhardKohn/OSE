#include "types.h"
#include "riscv.h"
#include "hardware.h"
#include "ringbuffer.h"
#include "uart.h"
#include "uartlock.h"

extern int interval;          // gibt das Intervall des Timerinterrups an 
extern uartlock lock;         // ist eine Struktur aus uartlock.c, die den Zustand des Uarts angeben kann 

/* 
* - definieren den UART und setzen ihn auf den Pointer 0x10000000 um den offset zu definiren.
*/
extern volatile struct uart *uart0;

/*
 * - __attribute__: weißt einer Struktur bestimmte Eigenschaften zu, in diesem Fall dem Stack des Kernels
 * - aligned(16): Die Adresse in der sich der Kernelstack befindet, sollte durch 16 teilbar sein.
 * - Hier wird der Kernelstack definiert mit einer Größe von 4096 Bytes
 */
__attribute__((aligned(16))) char kernelstack[4096];

/* definiert die Header der Funktionen in diesem File */
void panic(char *);                 // gibt einen Errorcode aus und blokiert alles 
void config_pmp(void);
void change_process_nr(void);
uint64 handle_interrupts(stackframes **s, uint64 pc);

/** schaut welchen Prozess momentan läuft */
int current_process = 0;  
int command = 0;
/** erstelle ein Array mit der größe Zwei. In diesem Array werden die beiden Prozesse gespeichert */
PCBs pcb[2]; // ist in hardware.h

//----------------------------------------------------------------------//
// idle Funktion (Prozess wäre besser)
#if 1
void idle(){
  while (pcb[current_process].state==BLOCKED)
  {
    if(uart0->IIR==0x00000000000000c4ull||!is_empty()){ // schaut ob ein Zeichen im Uart liegt oder ein Zeichen im Ringbuffer ist. 
      if(lock.process==current_process){
        pcb[current_process].state==READY;
        return;
      }
      
    }
    change_process_nr();
  }
}
#endif

//-----------------syscalls-----------------//

void exception_handler(void)
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
  pcb[current_process].sp = (uint64) *s;
  pcb[current_process].pc = pc;
  change_process_nr();
  if (pcb[current_process].state == READY){
     pc = pcb[current_process].pc;
     *s = (stackframes*) pcb[current_process].sp;
     config_pmp();
  }else{
      change_process_nr();
    if(pcb[current_process].state == BLOCKED){
      idle(s);
      pc = pcb[current_process].pc;
      *s = (stackframes*) pcb[current_process].sp;
      config_pmp();
    }
  }
  pcb[current_process].state = RUNNING;
  return pc;
}

void change_process_nr(void){
  if(pcb[current_process].state == RUNNING){
    pcb[current_process].state = READY;
  }
  current_process++;
  if (current_process > 1){
    current_process = 0;
  }
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

void unblock_process(void){
  if (pcb[lock.process].state == BLOCKED){
    pcb[lock.process].state = READY;
	}	
}

//------------------------interrupt handler-------------------------//
uint64 handle_interrupts(stackframes **s, uint64 pc)
{
  switch ((r_mcause() & 255)) // schauen uns nur das erste byte an 
  {
  case 7: //-----------------Timer Interrupt---------------//
    putachar('I');
    *(uint64 *)CLINT_MTIMECMP(0) = *(uint64 *)CLINT_MTIME + interval;
    pc = yield(s, pc);
    break;
  case 11:
      //-------------------------Hardware Interrupt-----------------//
      ;                                 // leere Instruktion
    uint32 irq = *(uint32 *)PLIC_CLAIM; // get highest prio interrupt nr
    switch (irq)
    {
    case UART_IRQ:                  // #define UART_IRQ 10; IRQ steht für interrupt request 
        ;                           // leere Instruktion
      uint32 uart_irq = uart0->IIR; // read UART interrupt source; schauen nach was für ein Uartinterrupt es ist
      if((uart_irq&15)==4){   // weil wir nur die ersten 4 bits von dem Register haben wollen, Daten sind jetzt verfügbar
      // Received Data Ready Interrupt (uart_irq sagt aus, was für ein hardwareinterrupt ausgelöst wurde)
      char c = uart0->RBR;
      if(c=='\\'){
        putachar('\\');
        command = 1;
      }else {
        if(command == 1){
          switch (c)
          {
          case 'n':
            putachar('n');
            printstring("\n");
            command=0;
            break;
          case 'A':
            putachar(c);
            printstring("\nDieses Programm ist von Leonhard Kohn\n");
            command=0;
            break;
          default:
            putachar(c);
            printstring("\nUnbekannter Befehl\n");
            command=0;
            break;
          }
        }else{
          command = 0;
          rb_write(c);         // schreibe das Zeichen in den Ringbuffer
          unblock_process();
        }
      }
      }
      break; // (clears UART interrupt)

    default:
      printstring("Unknown Hareware interrupt!");
      printhex(r_mcause());
      printhex(irq);
      break;
    }
    *(uint32 *)PLIC_COMPLETE = irq; // announce to PLIC that IRQ was handled
    break;

  default:
    printstring("Unknown interrupt!");
    printhex(r_mcause());
    break;
  }
  pc = pc - 4;
  return pc;
}

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

  if ((r_mcause() & (1ULL << 63)) != 0)
  { // lieg ein asyncroner Interrupt vor?
    pc = handle_interrupts(&s, pc);
  }
  //-----------------------------Fehler behandlung---------------------//
  else if (r_mcause() != 8)
  {
    exception_handler();    
  } 
  else
  {
    switch (nr)
    {
    case 1:
      if (pcb[current_process].state == BLOCKED){
        printstring("waiting...\n");
        pc = yield(&s,pc);
      } else{
        printstring((char *)param);
      }
      break;
    case 2:
      if (pcb[current_process].state == BLOCKED){
        printstring("waiting...\n");
        pc = yield(&s,pc);
      } else{
        putachar((char)param);
      }
      break;
    case 3: // readachar
      if (!holding()){
        close_uart();
      }
	    if (lock.locked == 1 && lock.process == current_process){
	      retval = readachar();
        if (retval == 0){
          pcb[current_process].state = BLOCKED;
          pc = yield(&s,pc);
        }
	    }else{
        pcb[current_process].state = BLOCKED;
        printstring("waiting...\n");
        pc = yield(&s,pc);
	    }
      break;
    case 10:
      ;
      int return_Uart = close_uart();
      if (return_Uart == 2)
      {
        printstring("\nError! Uart was already closed by another process\n");
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
  if (r_mcause() == 8){
     s->a0 = retval;
  }
  // um stack wiederherzustellen 
  asm volatile("mv a1, %0" : : "r" (s));

  // this function returns to ex.S
}
