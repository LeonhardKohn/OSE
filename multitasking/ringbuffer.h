#define BUFFER_SIZE 6  // gibt die Größe des Buffers an

char ringbuffer[BUFFER_SIZE];

int rb_write(char c);

int rb_read(char *c);

int is_full();

int is_empty();