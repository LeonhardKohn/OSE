#define BUFFER_SIZE 6

char ringbuffer[BUFFER_SIZE];

int head, tail, full_flag;

int rb_write(char c);

int rb_read(char *c);

int buffer_is_full();

int buffer_is_empty();