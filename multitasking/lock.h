// Gegenseitiges Ausschlussverfahren.
typedef struct lock {
  uint64 locked;        // Ist die Sperre aufgehoben?

  // Zur Fehlersuche:
  char *name;        // Name von der Sperre.
} uart_lock;