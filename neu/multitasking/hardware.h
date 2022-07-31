  struct uart
{
  union
  {
    uint8_t THR; // W = transmit hold register (offset 0)
    uint8_t RBR; // R = receive buffer register (also offset 0)
    uint8_t DLL; // R/W = divisor latch low (offset 0 when DLAB=1)
  };
  union
  {
    uint8_t IER; // R/W = interrupt enable register (offset 1)
    uint8_t DLH; // R/W = divisor latch high (offset 1 when DLAB=1)
  };
  union
  {
    uint8_t IIR; // R = interrupt identif. reg. (offset 2)
    uint8_t FCR; // W = FIFO control reg. (also offset 2)
  };
  uint8_t LCR; // R/W = line control register (offset 3)
  uint8_t MCR; // R/W = modem control register (offset 4)
  uint8_t LSR; // R   = line status register (offset 5)
};

typedef enum state{
  RUNNING = 1,
  READY = 2,
  BLOCKED = 3
} state; // 0 bedeutet keiner ist da

typedef struct PCB
{
  uint64 sp;
  uint64 pc;
  state state;
} PCBs;

typedef struct stackframe
{
  uint64 ra;
  uint64 sp;
  uint64 gp;
  uint64 tp;
  uint64 t0;
  uint64 t1;
  uint64 t2;
  uint64 s0;
  uint64 s1;
  uint64 a0;
  uint64 a1;
  uint64 a2;
  uint64 a3;
  uint64 a4;
  uint64 a5;
  uint64 a6;
  uint64 a7;
  uint64 s2;
  uint64 s3;
  uint64 s4;
  uint64 s5;
  uint64 s6;
  uint64 s7;
  uint64 s8;
  uint64 s9;
  uint64 s10;
  uint64 s11;
  uint64 t3;
  uint64 t4;
  uint64 t5;
  uint64 t6;
} stackframes;
