#include "types.h"

extern void printstring(char *c);

uint64 BUFFER_SIZE = 6;
uint64 head = 1;
uint64 tail = 0;
char ringbuffer[6];

uint8_t full_flag = 0;
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
