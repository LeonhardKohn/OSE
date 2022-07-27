// Gegenseitiges Ausschlussverfahren.
typedef struct uartlock {
  uint64 locked;        // Ist die Sperre aufgehoben?
  int process;
} uartlock;

void uart_open(uartlock *lk);

void uart_close(uartlock *lk);

int holding(uartlock *lk);

void initlock(char *name, uartlock *lock_uart);