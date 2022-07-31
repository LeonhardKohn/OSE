#include "ringbuffer.h"
#include "types.h"

extern void printstring(char *c);

int head = 1;
int tail = 0;
char ringbuffer[BUFFER_SIZE];

int full_flag = 0;
uint8_t empty_flag = 1;

int is_full(){
    return full_flag; 
}

int is_empty(){
    return empty_flag;
}

int rb_write(char c){
   if (is_full()){
      return -1;
   }else{
      if (empty_flag = 1){
         empty_flag = 0;
      }
      ringbuffer[head] = c;
      head = (head + 1) % BUFFER_SIZE;
      if (head == tail){
         full_flag = 1;
      }
   }
}

int rb_read(char *c){
   if (is_empty()){
     return -1;
   }else{
     if (full_flag = 1){
        full_flag = 0;
     }
     *c = ringbuffer[tail];
     tail = (tail + 1) % BUFFER_SIZE;
     if (head == tail){
        empty_flag = 1;
     }
   }
}
