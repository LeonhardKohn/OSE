#include "hardware.h"
#include "riscv.h"
#include "interrups.h"



void interrupt_handler(stackframes *s)
{
    switch ((r_mcause() & 255))
    {
    case 7:
      /* Timer interrupt */
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
      break;
    case 11:
      /* extern (Hardware) interrupt*/
      // M-mode external interrupt?
      ;
      uint32 irq = *(uint32 *)PLIC_CLAIM; // get highest prio interrupt nr
      if (irq == UART_IRQ)
      {                               // #define UART_IRQ 10
         ; //This is an empty statement.
        uint32 uart_irq = uart0->IIR; // read UART interrupt source
        putachar(readachar());
      } // (clears UART interrupt)
      pc = pc - 4;
      *(uint32 *)PLIC_COMPLETE = UART_IRQ; // announce to PLIC that IRQ was handled
      break;

    default:
      printstring("Unknown Hareware interrupt!");
      printhex(r_mcause());
      printhex(*(uint32 *)PLIC_CLAIM);
      break;
    }
  
}