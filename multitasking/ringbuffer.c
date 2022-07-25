#include "ringbuffer.h"

int buffer_is_full(){
    if(head+1==tail){
        return 1;
    }else{
        return 0;
    }
}

int buffer_is_empty(){
    if(tail+1==head){
        return 1;
    }else{
        return 0;
    }
    
}

int rb_write(char c){
    if(buffer_is_full()){
        return -1;
    }else{
        ringbuffer[head] = c;
        head = (head+1) % BUFFER_SIZE;
        if(head == tail){
            full_flag = 1;
        }
    }
}

int rb_read(char *c){
    if(buffer_is_empty()){
        return -1;
    }else{
        *c = ringbuffer[tail];
        tail = (tail+1) % BUFFER_SIZE;
        full_flag = 0;
    }
}