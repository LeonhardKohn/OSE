#include "types.h"
#include "riscv.h"
#include "hardware.h"
#include "ringbuffer.h"
#include "uart.h"
#include "uartlock.h"

extern int interval;  // gibt das Intervall des Timerinterrups an
extern uartlock lock; // ist eine Struktur aus uartlock.c, die den Zustand des Uarts angeben kann

extern volatile struct uart *uart0; // definieren den UART und setzen ihn auf den Pointer 0x10000000 um den offset zu definiren.

/*
 * - __attribute__: weißt einer Struktur bestimmte Eigenschaften zu, in diesem Fall dem Stack des Kernels
 * - aligned(16): Die Adresse in der sich der Kernelstack befindet, sollte durch 16 teilbar sein.
 * - Hier wird der Kernelstack definiert mit einer Größe von 4096 Bytes
 */
__attribute__((aligned(16))) char kernelstack[4096];

//-----------definiert die Header der Funktionen in diesem File-----------//
void panic(char *);                                   // gibt einen Errorcode aus und blokiert alles
void config_pmp(void);                                // konfiguriert pmp (pysical memory protection) bei einem prozess wechsel neu
void change_process_nr(void);                         // wechselt die Prozesse
uint64 handle_interrupts(stackframes **s, uint64 pc); // behandelt die verschiedenen asyncronen Interrupts

//-----------Globale variablen-----------//
int current_process = 0; // gibt momentanen Prozess an
int command = 0;         // boolean, der angibt, ob gerade ein backslash eingegeben wurde
PCBs pcb[2];             // Program Controll Block für die beiden Prozesse; Ist in hardware.h

//----------------------------------------------------------------------//
// idle Funktion (Prozess wäre besser)
void idle()
{
  while (pcb[current_process].state == BLOCKED)
  {
    if (uart0->IIR == 0x00000000000000c4ull || !is_empty())
    { // schaut ob ein Zeichen im Uart liegt oder ein Zeichen im Ringbuffer ist.
      if (lock.process == current_process)
      {
        pcb[current_process].state == READY;
        return;
      }
    }
    change_process_nr();
  }
}

//-----------------Hilfsfunktionen für den Kernel-----------------//
void error_handler(void)
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

// Memory protection wird umgestellt, damit ein Userprozess nicht auf andere Userprozesse zugreifen darf
void config_pmp(void)
{
  if (current_process == 0)
  {
    w_pmpcfg0(0x00000f0000);       // Speicherschutz auf neuen Prozess umschalten
  }
  else
  {
    w_pmpcfg0(0x000f000000);       // nur Zugriff auf Prozess 1; 2 - voller Zugriff; 1,0 - kein Zugriff
  }
}

void unblock_process(void)         // wechselt Prozess
{
  if (pcb[lock.process].state == BLOCKED)
  {
    pcb[lock.process].state = READY;
  }
}

//-----------------syscalls-----------------//
uint64 yield(stackframes **s, uint64 pc)         // nr = 23
{
  pcb[current_process].sp = (uint64)*s;
  pcb[current_process].pc = pc;
  change_process_nr();                          // speichere und wechsele den Prozess
  if (pcb[current_process].state == READY)      // schaut, ob der Prozess READY ist 
  {                                             // wenn ja, lade den Prozess
    pc = pcb[current_process].pc;
    *s = (stackframes *)pcb[current_process].sp;
    config_pmp();
  }
  else
  {                                              // sonst wechsele zu nächsten Prozess
    change_process_nr();  
    if (pcb[current_process].state == BLOCKED)   // falls auch dieser BLOCKED ist, gehe in die idle Funktion
    {
      idle(s);
      pc = pcb[current_process].pc;
      *s = (stackframes *)pcb[current_process].sp;
      config_pmp();
    }
  }
  pcb[current_process].state = RUNNING;          // setze den Prozess auf RUNNING
  return pc;
}

void change_process_nr(void)
{
  if (pcb[current_process].state == RUNNING)
  {
    pcb[current_process].state = READY;
  }
  current_process++;                             // Prozess wird gewechselt; müsste eine while(...) sein bei mehreren Prozessen 
  if (current_process > 1)
  {
    current_process = 0;
  }
}

uint64 sys_exit(stackframes **s, uint64 pc)      // startet die Prozesse neu; Round Robin
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
  switch ((r_mcause() & 255))                   // schauen uns nur das erste byte an
  {
  case 7: //-----------------Timer Interrupt---------------//
    putachar('I');
    *(uint64 *)CLINT_MTIMECMP(0) = *(uint64 *)CLINT_MTIME + interval;
    pc = yield(s, pc);
    break;
  case 11:;
      //-------------------------Hardware Interrupt-----------------//
    uint32 irq = *(uint32 *)PLIC_CLAIM;        // höchste Prio-interrupt nr
    switch (irq)
    {
    case UART_IRQ:;                            // UART_IRQ = 10; IRQ steht für interrupt request
      uint32 uart_irq = uart0->IIR;            // UART interrupt source lesen; schauen nach was für ein Uartinterrupt es ist
      if ((uart_irq & 15) == 4)
      {                                        // weil wir nur die ersten 4 bits von dem Register haben wollen, Daten sind jetzt verfügbar
                                               // uart_irq sagt aus, was für ein hardwareinterrupt ausgelöst wurde
        char c = uart0->RBR;
        if (c == '\\')                         // kleine Spielerrei, probiert ein paar Commands einzufügen
        {
          putachar('\\');
          command = 1;
        }
        else
        {
          if (command == 1)
          {
            switch (c)
            {
            case 'n':
              putachar('n');
              printstring("\n");
              command = 0;
              break;
            case 'A':
              putachar(c);
              printstring("\nDieses Programm ist von Leonhard Kohn\n");
              command = 0;
              break;
            default:
              putachar(c);
              printstring("\nUnbekannter Befehl\n");
              command = 0;
              break;
            }
          }
          else
          {
            command = 0;
            rb_write(c);                         // schreibe das Zeichen in den Ringbuffer
            unblock_process();
          }
        }
      }
      break;                                     // Hardwareinterrupt ende
    default:
      printstring("Unknown Hareware interrupt!");
      printhex(r_mcause());
      printhex(irq);
      break;
    }
    *(uint32 *)PLIC_COMPLETE = irq;              // dem PLIC mitteilen, dass der IRQ bearbeitet wurde
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

// Dies ist der C-Code-Teil des Exception-Handlers
// "exception" wird von der Assemblerfunktion "ex" in ex.S mit auf dem Stack gespeicherten Registern aufgerufen
void exception(stackframes *s)
{
  uint64 nr = s->a7;         // read syscall number from stack;
  uint64 param = s->a0;      // read parameter from stack;
  uint64 sp;
  uint64 pc = r_mepc();      // read exception PC;
  uint64 retval = 0;         // return value

  if ((r_mcause() & (1ULL << 63)) != 0) // lieg ein asyncroner Interrupt vor?
  { 
    pc = handle_interrupts(&s, pc);
  }
  //-----------------------------Fehler behandlung---------------------//
  else if (r_mcause() != 8)
  {
    error_handler();
  }
  else
  {
    switch (nr)
    {
    case 1: // printstring(...)
      if (pcb[current_process].state == BLOCKED)
      {
        printstring("waiting...\n");
        pc = yield(&s, pc);
      }
      else
      {
        printstring((char *)param);
      }
      break;
    case 2: // putachar(...)
      if (pcb[current_process].state == BLOCKED)
      {
        printstring("waiting...\n");
        pc = yield(&s, pc);
      }
      else
      {
        putachar((char)param);
      }
      break;
    case 3: // readachar()
      if (!holding())      // wenn der Uart noch offen ist, schließe ihn
      {
        close_uart();
      }
      if (lock.locked == 1 && lock.process == current_process)
      {                   // wenn der Uart von dem momentanen Prozess geschlossen wurde lese ein zeichen aus oder warte
        retval = readachar();
        if (retval == 0)
        {
          pcb[current_process].state = BLOCKED;
          pc = yield(&s, pc);
        }
      }
      else
      {                   // Falls ein zweiter Prozess auch auf den Uart zugreifen möchte, Blockiere ihn und warte bis der Uart wieder offen ist
        pcb[current_process].state = BLOCKED;
        printstring("waiting...\n");
        pc = yield(&s, pc);
      }
      break;
    case 10:; // close_uart()
      int return_Uart = close_uart();
      if (return_Uart == 2)
      {
        printstring("\nError! Uart was already closed by another process\n");
      }
      break;
    case 11: // open_uart()
      open_uart();
      break;
    case 23: // yield(...) system call
      pc = yield(&s, pc);
      break;
    case 42: // sys_exit(...)
    // user programm beenden und neustarten (Round Robin)
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

  // return value (pc) anpassen - wir wollen zu der Anweisung nach dem ecall zurückkehren! (an Adresse mepc+4)
  w_mepc(pc + 4);

  // den Rückgabewert in a0 schreiben
  if (r_mcause() == 8)
  {
    s->a0 = retval;
  }
  // um stack wiederherzustellen
  asm volatile("mv a1, %0"
               :
               : "r"(s));
  // geht zu ex.S zurück
}
