// Gegenseitiges Ausschlussverfahren.
typedef struct spinlock {
  uint64 locked;        // Ist die Sperre aufgehoben?

  // Zur Fehlersuche:
  char *name;        // Name von der Sperre.
  int process;
} spinlock;

void uart_open(spinlock *lk);

void uart_close(spinlock *lk);

int holding(spinlock *lk);