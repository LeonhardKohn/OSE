#define BUFFER_SIZE 6

char ringbuffer[BUFFER_SIZE];

int head, tail, full_flag;

int rb_write(char c);

int rb_read(char *c);

int is_full();

int is_empty();