#include "types.h"
#include "riscv.h"
#include "hardware.h"
#include "ringbuffer.h"
#include "uart.h"
#include "uartlock.h"

extern int main(void);
extern void ex(void);
extern void printstring(char *s);
extern int rb_write(char c);
extern int rb_read(char *c);
extern int interval;

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

/** schaut welchen Prozess momentan läuft */
int current_process = 0;
/** erstelle ein Array mit der größe Zwei. In diesem Array werden die beiden Prozesse gespeichert */
PCBs pcb[2];
uartlock lock;
uint64 tmpRead = 0;

//----------------------------------------------------------------------//

//-----------------syscalls-----------------//

void exception_handler(){
    printstring("\nERROR!\n");
    printstring("r_mcause: ");
    printhex(r_mcause());
    printstring("r_mepc: ");
    printhex(r_mepc());
    printstring("r_mtval: ");
    printhex(r_mtval());

    panic("Unknown Error\n");
}

uint64 yield(stackframes **s, uint64 pc){
  // Process wechseln 
  if (pcb[current_process].state == RUNNING){
    pcb[current_process].state = READY;
  }  
  pcb[current_process].pc = pc; // save pc, stack pointer
  pcb[current_process].sp = (uint64)*s;;
  pcb[current_process].kernel_sp = r_sp(); // 
  current_process++;
  if (current_process > 1) // da nur zwei Processe vorhanden sind 
    current_process = 0;

  if (pcb[current_process].state==READY){
    //hier abfrage auf uartblock
    //tmpRead = readachar();
  } else if (pcb[current_process].state==BLOCKED){
    current_process++;
    if (current_process > 1) // da nur zwei Processe vorhanden sind 
      current_process = 0;
  }
  
  config_pmp();
  pc = pcb[current_process].pc; // add +4 later!
  *s = (stackframes *)pcb[current_process].sp;
  uint64 kernel_sp = pcb[current_process].kernel_sp;
  pcb[current_process].state=RUNNING;
  w_sp(kernel_sp);
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

  pc = pcb[current_process].pc - 4;

  return pc;
}

//------------------------interrupt handler-------------------------//
uint64 handle_interrupts(stackframes **s, uint64 pc)
{
  switch ((r_mcause() & 255))
  {
  case 7: //-----------------Timer Interrupt---------------//
    putachar('I');
    pc = pc - 4;
    pc = yield(s,pc);
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
      rb_write(uart0->RBR);         // schreibe das Zeichen in den Ringbuffer
      //printhex(pcb[lock.process].state);
      if (pcb[lock.process].state==BLOCKED){
        pcb[lock.process].state = READY;
      }else{
          //putachar(readachar());
      }
      
      break;                        // (clears UART interrupt)

    default:
      printstring("Unknown Hareware interrupt!");
      printhex(r_mcause());
      printhex(irq);
      break;
    }
    pc = pc - 4;
    *(uint32 *)PLIC_COMPLETE = UART_IRQ; // announce to PLIC that IRQ was handled
    break;

  default:
    printstring("Unknown interrupt!");
    printhex(r_mcause());
    break;
  }
  return pc;
}
//#endif
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
  uint64 nr;
  uint64 param;
  uint64 sp;
  uint64 pc;
  uint64 retval = 0;
// überprüfen, ob ein Zeichen vorliegt
// überprüfen, ob ein Prozess vorliegt, der busy ist

  nr = s->a7;    // read syscall number from stack
  param = s->a0; // read parameter from stack
  pc = r_mepc(); // read exception PC

  if ((r_mcause() & (1ULL << 63)) != 0)
  { // lieg ein asyncroner Interrupt vor?
    pc = handle_interrupts(&s, pc);
    w_mepc(pc + 4);
    asm volatile("mv a1, %0"
                 :
                 : "r"(s));
    return;
  }
  //-----------------------------Fehler behandlung---------------------//
  else if (r_mcause() != 8){
    exception_handler();
  }
  else
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
      if(lock.locked==0){
        close_uart();
        }
      if(lock.process==current_process){
        /*if (!buffer_is_empty()){
            retval = readachar();
        }else*/{
            printhex(pc);
            retval = readachar();
            pcb[current_process].state = BLOCKED;
            pc = yield(&s, pc);
        }
       }else{
        printstring("Uart is locked from Process "+current_process);
       }
      

      
        // current process muss busy sein
        // nächsten Process nehmen der ready ist (passiert in yield)
        // bei jeder exeption auf ready/ busy überprüfen
      break;
    case 10:
      close_uart();
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
      break;

    default:
      printstring("Invalid syscall: ");
      printhex(nr);
      printstring("\n");
      break;
    }
  }
  // adjust return value - we want to return to the instruction _after_ the ecall! (at address mepc+4)
  w_mepc(pc + 4);

  asm volatile("mv a1, %0"
               :
               : "r"(s));
  // nur wenn's ein syscall war, dann ... s->a0 = return value from syscall;
/*
  // wenn es einen Interrupt gab muss das zeichen noch in retval geschrieben werden 
  if ((r_mcause() & (1ULL << 63)) != 0){
    retval = tmpRead;
    tmpRead = 0;
  }
*/
  // retval = 0x40;
  //if ((r_mcause() == 8) && (r_mcause() & (1ULL << 63)) != 0||(r_mcause() & 255)==11)
  {
    s->a0 = retval;
  }
  // this function returns to ex.S
}
