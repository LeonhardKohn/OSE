#include "types.h"
#include "hardware.h"
#include "uart.h"
#include "uartlock.h"
#include "ringbuffer.h"

volatile struct uart *uart0 = (volatile struct uart *)0x10000000;
//-----------IO Funktionen--------------//
void printstring(char *s)
{
  while (*s)
  {                       // Solange das Zeichen nicht Null ist
    putachar(*s);         // das Zeichen ausgeben
    s++;                  // und weiter zum nächsten Zeichen
  }
}

void putachar(char c)
{
  while ((uart0->LSR & (1 << 5)) == 0)
    ;                     // nichts tun - warten, bis Bit 5 des LSR = 1
  uart0->THR = c;         // dann das Zeichen schreiben
}

char readachar(void){
    char c;
    int error = rb_read(&c);  // liese ein Zeichen aus dem Ringbuffer 
    if (error == -1){         // wenn kein zeichen im ringbuffer ist gib 0 zurück
      return 0;
    }
    return c; 
}

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

void uartInit(){

  // disable interrupts.
  uart0->IER = 0x00;

  // special mode to set baud rate.
  uart0->LCR = 0x80;

  // leave set-baud mode,
  // and set word length to 8 bits, no parity.
  uart0->LCR = 0x03;

  // reset and enable FIFOs.
  uart0->FCR = 0x07;

  // enable receive interrupts.
  uart0->IER = 0x01;
}