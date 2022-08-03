#include "ringbuffer.h"
#include "types.h"

extern void printstring(char *c); 

uint8_t head = 0;                   // Position des schreibenden Elementes
uint8_t tail = 0;                   // Position des lesenden Elementes
char ringbuffer[BUFFER_SIZE];

uint8_t full_flag = 0;
uint8_t empty_flag = 1;

int is_full(){
    return full_flag; 
}

int is_empty(){
    return empty_flag;
}

int rb_write(char c){      // schreibt in den Buffer und verändert head
   if (is_full()){         // wenn der Buffer voll ist gibt er -1 zurück (als Fehlermeldung)
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

int rb_read(char *c){    // liest ein zeichen und verändert tail
   if (is_empty()){      // wenn der Buffer leer ist gibt er -1 zurück (als Fehlermeldung)
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
