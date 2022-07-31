// Gegenseitiges Ausschlussverfahren.
typedef struct uartlock {
  uint64 locked;        // Ist die Sperre aufgehoben?
  int process;
} uartlock;

int close_uart(void);

void open_uart(void);

int holding(void);
