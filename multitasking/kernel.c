#include "types.h"
#include "riscv.h"
#include "hardware.h"

extern int main(void);
extern void ex(void);
extern void printstring(char *s);
extern int interval;
/*
definieren den UART und setzen ihn auf den Pointer 0x10000000 um den offset zu definiren.
Wichtig lernen!

*/
extern volatile struct uart *uart0;

/*
registriert sich mindestens 16 bytes

__attribute__() : allows you to specify special attributes of struct and union types when you define such types.
(aligned(16)) : This attribute specifies a minimum alignment (in bytes) for variables of the specified type.
                also hier mindestens 16 bytes für jeden char
char kernelstack[4096] : minimum 16 bytes (anfangsadress muss durch 16 teilbar sein)

registrieren uns auf dem Stack einen gewissen Speicherplatz
*/
__attribute__((aligned(16))) char kernelstack[4096];

/*
definiert die Header der Funktionen
*/
void printhex(uint64);
void putachar(char);
void copyprog(int);

// schaut welchen Prozess momentan läuft
int current_process = 0;

/*
erstelle ein Array mit der größe Zwei. In diesem Array werden die beiden Prozesse gespeichert
*/
PCBs pcb[2];

//----------------------------------------------------------------------//

void interrupt_toggle()
{
}

// our syscalls
void printstring(char *s)
{
  while (*s)
  {               // as long as the character is not null
    putachar(*s); // output the character
    s++;          // and progress to the next character
  }
}

void putachar(char c)
{
  while ((uart0->LSR & (1 << 5)) == 0)
    ;             // do nothing - wait until bit 5 of LSR = 1
  uart0->THR = c; // then write the character
}

char readachar(void)
{
  while ((uart0->LSR & (1 << 0)) == 0)
    ;                // do nothing - wait until bit 5 of LSR = 1
  return uart0->RBR; // then read the character
}

// TODO Yield und exit
void yield(uint64 pc, stackframes *s)
{
  // save pc, stack pointer
  pcb[current_process].pc = pc;
  pcb[current_process].sp = (uint64)s;
  // select new process
  current_process++;
  if (current_process > 1)
    current_process = 0;
  pc = pcb[current_process].pc; // add +4 later!
  s = (stackframes *)pcb[current_process].sp;
}

// just a helper function
void printhex(uint64 x)
{
  int i;
  char s[2];
  s[1] = 0;

  printstring("0x");
  for (i = 60; i >= 0; i -= 4)
  {
    int d = ((x >> i) & 0x0f);
    if (d < 10)
      s[0] = d + '0';
    else
      s[0] = d - 10 + 'a';
    printstring(s);
  }
  printstring("\n");
}

//---------------panic---------------//
void panic()
{
  while (1)
  {
  }
}

// This is the C code part of the exception handler
// "exception" is called from the assembler function "ex" in ex.S with registers saved on the stack
void exception(stackframes *s)
{
  uint64 nr;
  uint64 param;
  uint64 retval = 0;
  uint64 pc;
  uint64 sp;

  nr = s->a7;    // read syscall number from stack
  param = s->a0; // read parameter from stack
  pc = r_mepc(); // read exception PC

  // TODO 3.1: Add code to detect exceptions that are not system calls here.
  // Print the values of the mcause, mepc and mtval registers
  if ((r_mcause() & (1ULL << 63)) != 0)
  { // async interrupt?
    //-------------------------Hardware Interrupt-----------------//
    if ((r_mcause() & 255) == 11)
    {                                     // M-mode external interrupt?
      uint32 irq = *(uint32 *)PLIC_CLAIM; // get highest prio interrupt nr
      switch (irq)
      {
      case UART_IRQ:                  // #define UART_IRQ 10
          ;                           // leere Instruktion
        uint32 uart_irq = uart0->IIR; // read UART interrupt source
        putachar(readachar());
        break; // (clears UART interrupt)

      default:
        printstring("Unknown Hareware interrupt!");
        printhex(r_mcause());
        printhex(*(uint32 *)PLIC_CLAIM);
        break;
      }
      pc = pc - 4;
      *(uint32 *)PLIC_COMPLETE = UART_IRQ; // announce to PLIC that IRQ was handled
    }
    //-----------------Timer Interrupt---------------//
    else if ((r_mcause() & 255) == 7)
    {
      putachar('I');
      // printstring("\nInterrupt\n");
      pc = pc - 4;
      // save pc, stack pointer

      pcb[current_process].pc = pc;
      pcb[current_process].sp = (uint64)s;
      // select new process
      current_process++;
      if (current_process > 1)
        current_process = 0;
      pc = pcb[current_process].pc; // add +4 later!
      s = (stackframes *)pcb[current_process].sp;

      // yield(pc,s);
      // Switch memory protection to new process
      if (current_process == 0)
      {
        w_pmpcfg0(0x00000f0000);
      } // only access to Process 1// 2 - full access; 1,0 - no access
      if (current_process == 1)
      {
        w_pmpcfg0(0x000f000000);
      } // only access to Process 1 // 3 - full; 2,1,0 - no access

      *(uint64 *)CLINT_MTIMECMP(0) = *(uint64 *)CLINT_MTIME + interval;
    }
    else
    {
      printstring("Unknown interrupt!");
      printhex(r_mcause());
    }
    // hier returnen!
  }
  else if (r_mcause() != 8)
  {
    printstring("\nERROR!\n");
    printstring("r_mcause: ");
    printhex(r_mcause());
    printstring("r_mepc: ");
    printhex(r_mepc());
    printstring("r_mtval: ");
    printhex(r_mtval());

    panic();
  }
  else
  {

    // TODO 3.2: Create a data structure (PCB) for storing the PC and SP of each process.
    //           Save registers when exception is entered - you may need to change ex.S, too!

    // pcb[current_process].pc = r_mepc();
    // pcb[current_process].sp = r_sp();

    // printhex(r_sp);

    // TODO 3.2: Implement a simple round-robin scheduler within the yield system call.

    // sp = r_sp();
    // pc = r_mepc();

    // decode syscall number

    // TODO 3.2: Add handling of system call 23 (yield) here
    switch (nr)
    {
    case 1:
      printstring((char *)param);
      break;
    case 2:
      putachar((char)param);
      break;
    case 3:
      retval = readachar();
      break;
    case 23: // yield system call
      // save pc, stack pointer

      pcb[current_process].pc = pc;
      pcb[current_process].sp = (uint64)s;
      // select new process
      current_process++;
      if (current_process > 1)
        current_process = 0;
      pc = pcb[current_process].pc; // add +4 later!
      s = (stackframes *)pcb[current_process].sp;
      // Switch memory protection to new process
      if (current_process == 0)
      {
        w_pmpcfg0(0x00000f0000);
      } // only access to Process 1// 2 - full access; 1,0 - no access
      if (current_process == 1)
      {
        w_pmpcfg0(0x000f000000);
      } // only access to Process 1 // 3 - full; 2,1,0 - no access
      // ...handle further processes in a more elegant way?
      break;
    case 42: // printstring("user program returned, starting from the beginning\n");
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

  // TODO 3.2: Return the value of the stack pointer so that restoring of the correct registers
  //           in ex.S works...
  // asm volatile("mv a1, %0" : : "r" (sp));
  asm volatile("mv a1, %0"
               :
               : "r"(s));

// nur wenn's ein syscall war, dann ... s->a0 = return value from syscall;
if ((r_mcause() == 8)&&(r_mcause() & (1ULL << 63)) != 0)
{
  s->a0 = retval;
}

  // this function returns to ex.S
}
