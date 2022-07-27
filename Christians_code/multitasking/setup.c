#include "userprogs1.h"
#include "userprogs2.h"
#include "types.h"
#include "riscv.h"
#include "hardware.h"

extern int main(void);
extern void ex(void);
extern void printstring(char *);
extern void printhex(uint64);
extern PCB pcb[];
extern volatile struct uart *uart0;

// The intervall for every interrupt -> 1/10th second
int timer_interval = 10000000;

void timerinit(void){
  // ID for the processor (for multicore, but we have only single core)
  int id = 0;

  // Set the compare-value to a higher value
  *(uint64*)CLINT_MTIMECMP(id) = *(uint64*)CLINT_MTIME + timer_interval;

  // enable machine-mode interrupts
  w_mstatus(r_mstatus() | MSTATUS_MIE);

  // enable machine-mode timer interrupts
  w_mie(r_mie() | MIE_MTIE);
}

void initPLIC(){
  *(uint32*)(PLIC + UART_IRQ*4) = 1;
  *(uint32*)(PLIC + VIRTO0_IRQ*4) = 1;
  *(uint32*)PLIC_THRESHOLD = 0;
  *(uint32*)PLIC_ENABLE = (1 << UART_IRQ);
  w_mie(r_mie() | MIE_MEIE);
}

void uartInit(){
  // disable interrupts
  uart0->IER = 0x00;

  // special mode to set baudrate
  uart0->LCR = 0x80;

  //leave set-baud mode
  //and set word length to 8 bits, no parity
  uart0->LCR = 0x03;

  //reset and enable FIFOs
  uart0->FCR = 0x07;

  //enable receive interrupts
  uart0->IER = 0x01;
}

// Diese Funktion kopiert einen User-Prozess zu der entsprechenden Speicherstelle
void copyprog(int process, uint64 address) {
  // copy user code to memory inefficiently... :)
  unsigned char* from;
  int user_bin_len;
  switch (process) {
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

  unsigned char* to   = (unsigned char *)address;
  for (int i=0; i<user_bin_len; i++) {
    *to++ = *from++;
  }
}

void setup(void) {
  // set M Previous Privilege mode to User so mret returns to user mode.
  unsigned long x = r_mstatus();
  x &= ~MSTATUS_MPP_MASK;
  x |= MSTATUS_MPP_U;
  w_mstatus(x);

  /* enable machine-mode interrupts.
   * Wenn das vierte Bit im mstatus-register 1 ist, sind Interrupts aktiviert. 
   * MSTATUS_MIE = 0x8 - mittels ODER wird das Bit an der vierten Stelle auf 1 gesetzt.*/
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
  w_pmpaddr0(0x80000000ull >> 2);
  w_pmpaddr1(0x80100000ull >> 2);
  w_pmpaddr2(0x80200000ull >> 2);
  w_pmpaddr3(0x80300000ull >> 2);
  w_pmpaddr4(0xffffffffull >> 2);

  /* 0x00(addr4)00(addr3)0f(addr2)00(addr1)00(addr0)
   * 00 = Keine Zugriffsrechte
   * 0f = Alle Zugriffsrechte */
  w_pmpcfg0(0x00000f0000);


  // Maschinencode der Prozesse werden zu den zugewiesenen Speicherstellen kopiert
  copyprog(0, 0x80100000);
  copyprog(1, 0x80200000);

  // Program Control Block wird initialisiert mit den Anfangsadressen von pc und sp
  pcb[0].pc = 0x80100000;
  pcb[0].sp = 0x80102000;
  pcb[0].blocked = 0;
  pcb[1].pc = 0x80200000;
  pcb[1].sp = 0x80202000;
  pcb[1].blocked = 0;

  // set M Exception Program Counter to main, for mret, requires gcc -mcmodel=medany
  w_mepc((uint64)0x80100000);
  
  // Der Timer wird initialisiert
  timerinit();

  uartInit();
  initPLIC();

  // switch to user mode (configured in mstatus) and jump to address in mepc CSR -> main().
  asm volatile("mret");
}

