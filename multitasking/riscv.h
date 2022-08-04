#define MSTATUS_MPP_MASK (3L << 11) // previous mode.
#define MSTATUS_MPP_M (3L << 11)
#define MSTATUS_MPP_S (1L << 11)
#define MSTATUS_MPP_U (0L << 11)
#define MSTATUS_MIE (1L << 3) // machine-mode interrupt enable. (4 Bit ist gewollt)

#define CLINT 0x2000000L
#define CLINT_MTIMECMP(hartid) (CLINT + 0x4000 + 8 * (hartid))
#define CLINT_MTIME (CLINT + 0xBFF8)

//----------PLIC---------//
#define PLIC_PRIORITY 0x0c000000  // Sets the priority of a particular interrupt source
#define PLIC_PENDING 0x0c001000   // Contains a list of interrupts that have been triggered (are pending)
#define PLIC_ENABLE 0x0c002000    // Enable/disable certain interrupt sources (1 bit per int.)
#define PLIC_THRESHOLD 0x0c200000 // Sets the threshold that interrupts must meet before being able to trigger (1 32-bit word per int.)
#define PLIC_CLAIM 0x0c200004     // Returns the next interrupt in priority order
#define PLIC_COMPLETE 0x0c200004  // Completes handling of a particular interrupt

// virtio mmio interface
#define VIRTIO0 0x10001000
#define VIRTIO0_IRQ 1

#define UART_IRQ 10

static inline uint64
r_mcause()
{
  uint64 x;
  asm volatile("csrr %0, mcause"
               : "=r"(x));
  return x;
}

static inline uint64
r_sp()
{
  uint64 x;
  asm volatile("mv %0, sp"
               : "=r"(x));
  return x;
}

static inline void
w_sp(uint64 x)
{
  asm volatile("mv sp, %0"
               :
               : "r"(x));
}

static inline uint64
r_mstatus()
{
  uint64 x;
  asm volatile("csrr %0, mstatus"
               : "=r"(x));
  return x;
}

static inline void
w_mstatus(uint64 x)
{
  asm volatile("csrw mstatus, %0"
               :
               : "r"(x));
}

// machine exception program counter, holds the
// instruction address to which a return from
// exception will go.
static inline uint64
r_mepc()
{
  uint64 x;
  asm volatile("csrr %0, mepc"
               : "=r"(x));
  return x;
}

static inline void
w_mepc(uint64 x)
{
  asm volatile("csrw mepc, %0"
               :
               : "r"(x));
}

static inline uint64
r_mtval()
{
  uint64 x;
  asm volatile("csrr %0, mtval"
               : "=r"(x));
  return x;
}
// supervisor address translation and protection;
// holds the address of the page table.
static inline void
w_satp(uint64 x)
{
  asm volatile("csrw satp, %0"
               :
               : "r"(x));
}

// Machine Interrupt Enable
#define MIE_MEIE (1L << 11) // external
#define MIE_MTIE (1L << 7)  // timer
#define MIE_MSIE (1L << 3)  // software
static inline uint64
r_mie()
{
  uint64 x;
  asm volatile("csrr %0, mie"
               : "=r"(x));
  return x;
}

static inline void
w_mie(uint64 x)
{
  asm volatile("csrw mie, %0"
               :
               : "r"(x));
}

// Supervisor Interrupt Enable
#define SIE_SEIE (1L << 9) // external
#define SIE_STIE (1L << 5) // timer
#define SIE_SSIE (1L << 1) // software
static inline uint64
r_sie()
{
  uint64 x;
  asm volatile("csrr %0, sie"
               : "=r"(x));
  return x;
}

static inline void
w_sie(uint64 x)
{
  asm volatile("csrw sie, %0"
               :
               : "r"(x));
}

static inline void
w_pmpcfg0(uint64 x)
{
  asm volatile("csrw pmpcfg0, %0"
               :
               : "r"(x));
}

static inline void
w_pmpaddr0(uint64 x)
{
  asm volatile("csrw pmpaddr0, %0"
               :
               : "r"(x));
}

static inline void
w_pmpaddr1(uint64 x)
{
  asm volatile("csrw pmpaddr1, %0"
               :
               : "r"(x));
}

static inline void
w_pmpaddr2(uint64 x)
{
  asm volatile("csrw pmpaddr2, %0"
               :
               : "r"(x));
}

static inline void
w_pmpaddr3(uint64 x)
{
  asm volatile("csrw pmpaddr3, %0"
               :
               : "r"(x));
}

static inline void
w_pmpaddr4(uint64 x)
{
  asm volatile("csrw pmpaddr4, %0"
               :
               : "r"(x));
}

static inline void
w_pmpcfg1(uint64 x)
{
  asm volatile("csrw pmpcfg1, %0"
               :
               : "r"(x));
}

static inline void
w_pmpcfg2(uint64 x)
{
  asm volatile("csrw pmpcfg2, %0"
               :
               : "r"(x));
}

// Machine-mode interrupt vector
static inline void
w_mtvec(uint64 x)
{
  asm volatile("csrw mtvec, %0"
               :
               : "r"(x));
}
