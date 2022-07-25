#include "types.h"
#include "riscv.h"
#include "hardware.h"
#include "ringbuffer.h"
#include "uart.h"
#include "uartlock.h"

extern int main(void);
extern void ex(void);
extern void printstring(char *s);
extern int interval;

/** definieren den UART und setzen ihn auf den Pointer 0x10000000 um den offset zu definiren.
* Wichtig lernen! */
extern volatile struct uart *uart0;

/** registriert sich mindestens 16 bytes
* char kernelstack[4096] : minimum 16 bytes (anfangsadress muss durch 16 teilbar sein)
* registrieren uns auf dem Stack einen gewissen Speicherplatz */
__attribute__((aligned(16))) char kernelstack[4096];

/** definiert die Header der Funktionen */
void printhex(uint64);
void putachar(char);
void copyprog(int);

/** schaut welchen Prozess momentan läuft */
int current_process = 0;
/** erstelle ein Array mit der größe Zwei. In diesem Array werden die beiden Prozesse gespeichert */
PCBs pcb[2];

uint64 busy_uart = 0;
spinlock lock_uart = initlock("uart");

//----------------------------------------------------------------------//


//-----------------syscalls-----------------//

uint64 yield(stackframes **s, uint64 pc)
{
  // TODO emum wechsel  
      pcb[current_process].pc = pc;
      pcb[current_process].sp = (uint64)s;

      if(busy_uart<0){
        if(pcb[busy_uart].state==READY){
          current_process = busy_uart;
        }
      }else{
        // select new process
        current_process++;
        if (current_process > 1)
          current_process = 0;
                // Switch memory protection to new process
      if (current_process == 0)
      {
        w_pmpcfg0(0x00000f0000);
      } // only access to Process 1// 2 - full access; 1,0 - no access
      if (current_process == 1)
      {
        w_pmpcfg0(0x000f000000);
      } // only access to Process 1 // 3 - full; 2,1,0 - no access
      }
      pc = pcb[current_process].pc; // add +4 later!
      s = (stackframes *)pcb[current_process].sp;

      return pc;
}


uint64 sys_exit(stackframes **s,uint64 pc){
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
uint64 handle_interrupts(stackframes **s,uint64 pc){
switch ((r_mcause() & 255))
    {
    case 7: //-----------------Timer Interrupt---------------//
      putachar('I');
      pc = pc - 4;
      pcb[current_process].pc = pc;            // save pc, stack pointer
      pcb[current_process].sp = (uint64)*s;
      current_process++;                       // select new process
      if (current_process > 1)
        current_process = 0;
      pc = pcb[current_process].pc;           // add +4 later!
      *s = (stackframes *)pcb[current_process].sp;
      if (current_process == 0){              // Switch memory protection to new process
        w_pmpcfg0(0x00000f0000);
      } 
      if (current_process == 1){              // only access to Process 1// 2 - full access; 1,0 - no access
        w_pmpcfg0(0x000f000000);
      }                                       // only access to Process 1 // 3 - full; 2,1,0 - no access

      *(uint64 *)CLINT_MTIMECMP(0) = *(uint64 *)CLINT_MTIME + interval;
      break;

    case 11:
      //-------------------------Hardware Interrupt-----------------//
      ;                                      // leere Instruktion
      uint32 irq = *(uint32 *)PLIC_CLAIM;    // get highest prio interrupt nr
      switch (irq)
      {
      case UART_IRQ:                         // #define UART_IRQ 10
          ;                                  // leere Instruktion
        uint32 uart_irq = uart0->IIR;        // read UART interrupt source
        putachar(readachar());
        break;                               // (clears UART interrupt)

      default:
        printstring("Unknown Hareware interrupt!");
        printhex(r_mcause());
        printhex(irq);
        break;
      }
      pc = pc - 4;
      *(uint32 *)PLIC_COMPLETE = UART_IRQ;   // announce to PLIC that IRQ was handled
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
  uint64 retval = 0;
  uint64 sp;
  uint64 pc;
  // überprüfen, ob ein Zeichen vorliegt 
  // überprüfen, ob ein Prozess vorliegt, der busy ist 

  if ((uart0->LSR & (1 << 0)) != 0){
    rb_write(uart0->RBR);
    if (busy_uart < 0){
      if (pcb[busy_uart].state == BLOCKED){
        pcb[busy_uart].state = READY;
      }
    } 
  }



  nr = s->a7;                                  // read syscall number from stack
  param = s->a0;                               // read parameter from stack
  pc = r_mepc();                               // read exception PC

  if ((r_mcause() & (1ULL << 63)) != 0){       // lieg ein asyncroner Interrupt vor?  
    pc = handle_interrupts(&s,pc);
     w_mepc(pc + 4);
     asm volatile("mv a1, %0"
               :
               : "r"(s));
     return;                  
  }
  //-----------------------------Fehler behandlung---------------------//
  else if (r_mcause() != 8)
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
      if (!buffer_is_empty()){
        rb_read(&retval);
      }else{
        retval=0;
        pcb[current_process].state = BLOCKED;
        busy_uart = current_process;
        pc = yield(&s,pc);



        // current process muss busy sein 
        // nächsten Process nehmen der ready ist 
        // bei jeder exeption auf ready/ busy überprüfen  


      }
      break;
    case 10:
      //uart_close();
      break;
    case 11:
      break;
    case 23:  
      // yield system call
      pc = yield(&s,pc);
      break;
    case 42:                                       // user program returned, starting from the beginning
      pc = sys_exit(&s,pc);
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
  if ((r_mcause() == 8) && (r_mcause() & (1ULL << 63)) != 0)
  {
    s->a0 = retval;
  }
  // this function returns to ex.S
}
