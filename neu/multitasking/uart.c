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
  {                       // as long as the character is not null
    putachar(*s);         // output the character
    s++;                  // and progress to the next character
  }
}

void putachar(char c)
{
  while ((uart0->LSR & (1 << 5)) == 0)
    ;                     // do nothing - wait until bit 5 of LSR = 1
  uart0->THR = c;         // then write the character
}

char readachar(void)
{
  #if 0
  if ((uart0->LSR & (1 << 0)) != 0){
        return uart0->RBR;             // then read the character
  }
  #endif

  char c;
  int error = rb_read(&c);
  if (error == -1){
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