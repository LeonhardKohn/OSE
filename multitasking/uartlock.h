// Gegenseitiges Ausschlussverfahren.
typedef struct uartlock {
  uint64 locked;        // Ist die Sperre aufgehoben?
  int process;          // speichert den momentanen Prozess
} uartlock;

int close_uart(void);

void open_uart(void);

int holding(void);

void initlock();
