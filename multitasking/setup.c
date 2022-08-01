#include "userprogs1.h"
#include "userprogs2.h"
#include "types.h"
#include "riscv.h"
#include "hardware.h"
#include "uart.h"
#include "uartlock.h"

extern int main(void);
extern void ex(void);
extern void printstring(char *);
extern void printhex(uint64);
extern PCBs pcb[];
extern volatile struct uart *uart0;
int interval = 10000000; // 1/10 sekunde
//------------------plicInit------------------//
// vieles in riscv.h definiert 
void plicinit(void)
{
  // set desired IRQ priorities non-zero (otherwise disabled).
  *(uint32*)(PLIC_PRIORITY + UART_IRQ*4) = 1;
  *(uint32*)(PLIC_PRIORITY + VIRTIO0_IRQ*4) = 1; // VIRTIO0_IRQ = 1 
  *(uint32*)PLIC_THRESHOLD = 0;
  *(uint32*)PLIC_ENABLE = (1<<UART_IRQ); // UART_IRQ = 10

  w_mie(r_mie() | MIE_MEIE);
}

//--------------copyprog--------------//
// kopiert den Maschienencode dorthin, wo er hin soll

void copyprog(int process, uint64 address)
{
  // copy user code to memory inefficiently... :)
  unsigned char *from;
  int user_bin_len;
  switch (process)
  {
  case 0:
    from = (unsigned char *)&user1_bin;
    user_bin_len = user1_bin_len;
    break;
  case 1:
    from = (unsigned char *)&user2_bin;
    user_bin_len = user2_bin_len;
    break;
  default:
    printstring("unknown process!\n");
    printhex(process);
    printstring("\n");
    break;
  }

  unsigned char *to = (unsigned char *)address;
  for (int i = 0; i < user_bin_len; i++)
  {
    *to++ = *from++;
  }
}
//----------------Timer Interrupt----------------//
void timeInterupt(void)
{
  int id = 0;

  // fragt den CLINT für den timer interupt

  *(uint64 *)CLINT_MTIMECMP(id) = *(uint64 *)CLINT_MTIME + interval;

  // aktiviert den interupt-mode
  w_mstatus(r_mstatus() | MSTATUS_MIE);

  // aktiviere maschiene-mode timer interrupt
  w_mie(r_mie() | MIE_MTIE);
}

//-----------------------setup-----------------------//

void setup(void)
{
  // set M Previous Privilege mode to User so mret returns to user mode.
  unsigned long x = r_mstatus();
  x &= ~MSTATUS_MPP_MASK;
  x |= MSTATUS_MPP_U;
  w_mstatus(x);

  // enable machine-mode interrupts.
  w_mstatus(r_mstatus() | MSTATUS_MIE);

  // enable software interrupts (ecall) in M mode.
  w_mie(r_mie() | MIE_MSIE);

  // set the machine-mode trap handler to jump to function "ex" when a trap occurs.
  w_mtvec((uint64)ex);

  // disable paging for now.
  w_satp(0);

  /* Hier wird die physical memory protection eingestellt.
   * pmpaddr0 - Bereich für Peripheriegeräte 	-> ist für den User gesperrt
   * pmpaddr1 - Bereich des Kernels		-> ist für den User gesperrt
   * pmpaddr2 - Bereich des 1. User-Prozesses	-> ist für andere User-Prozesse gesperrt
   * pmpaddr3 - Bereich des 2. User-Prozesses	-> ist für andere User-Prozesse gesperrt
   * pmpaddr4 - Rest (andere Prozesse)		-> ist hier für beide User gesperrt */

  // configure Physical Memory Protection to give user mode access to all of physical memory.
  w_pmpaddr0(0x80000000ull >> 2);
  w_pmpaddr1(0x80100000ull >> 2);
  w_pmpaddr2(0x80200000ull >> 2);
  w_pmpaddr3(0x80300000ull >> 2);
  w_pmpaddr4(0xffffffffull >> 2);
    /* 0x00(addr4)00(addr3)0f(addr2)00(addr1)00(addr0)
   * 00 = Keine Zugriffsrechte
   * 0f = Alle Zugriffsrechte */
  w_pmpcfg0(0x00000f0000); // only access to Process 1

  // TODO 3.1: Add configuration for PMP so that the I/O and kernel address space is protected
    // Maschinencode der Prozesse werden zu den zugewiesenen Speicherstellen kopiert
  copyprog(0, 0x80100000);
  copyprog(1, 0x80200000);

  // TODO 3.2: Add initalization of correct values in the PCB for all processes.
  pcb[0].pc = 0x80100000;
  pcb[0].sp = 0x80102000;
  pcb[0].state = READY;
  pcb[1].pc = 0x80200000;
  pcb[1].sp = 0x80202000;
  pcb[1].state = READY;

  // Initialisierung 
  timeInterupt();
  uartInit();
  plicinit();
  initlock();

  // set M Exception Program Counter to main, for mret, requires gcc -mcmodel=medany
  w_mepc((uint64)0x80100000);

  // switch to user mode (configured in mstatus) and jump to address in mepc CSR -> main().
  asm volatile("mret");
}
