#include <stdio.h>
#include <stdlib.h>

void write_to_memory(unsigned char *mem, int offset, unsigned char *data){
    for(int i = 0; i < sizeof(data)/sizeof(data[0]); i++){
        mem[offset + i] = data[i];
    }
}

struct stack{
    unsigned char elements[20];
    int curr_elements;
};

typedef struct stack Stack;

void push_to_stack(Stack *stackptr, unsigned char data){
    stackptr->elements[stackptr->curr_elements] = data;
    stackptr->curr_elements++;
    printf("after push: size:%d\n",stackptr->curr_elements);
}

unsigned char pop_stack(Stack *stackptr){
    stackptr->curr_elements--;
    return stackptr->elements[stackptr->curr_elements];
}

unsigned char peak_stack(Stack *stackptr){
    int tmp = stackptr->curr_elements - 1;
    return stackptr->elements[tmp];
}

int main(int argc, char** argv) {
    //getting input file
    FILE *fileptr;
    char *buffer;
    long filelen;

    fileptr = fopen(argv[1], "rb");
    fseek(fileptr, 0, SEEK_END);
    filelen = ftell(fileptr);
    rewind(fileptr);

    buffer = (char *)malloc(filelen * sizeof(char));
    fread(buffer, filelen, 1, fileptr);
    fclose(fileptr);

    //initializing components
    unsigned char memory[4096];
    unsigned char delay_timer;
    unsigned char sound_timer;
    Stack chip_stack;
    chip_stack.curr_elements = 0;
    //registers
    unsigned short I;
    unsigned short pc;
    unsigned char v[16];
    //font data
    unsigned char font[80] = {0xF0, 0x90, 0x90, 0x90, 0xF0,   // 0
                              0x20, 0x60, 0x20, 0x20, 0x70,   // 1
                              0xF0, 0x10, 0xF0, 0x80, 0xF0,   // 2
                              0xF0, 0x10, 0xF0, 0x10, 0xF0,   // 3
                              0x90, 0x90, 0xF0, 0x10, 0x10,   // 4
                              0xF0, 0x80, 0xF0, 0x10, 0xF0,   // 5
                              0xF0, 0x80, 0xF0, 0x90, 0xF0,   // 6
                              0xF0, 0x10, 0x20, 0x40, 0x40,   // 7
                              0xF0, 0x90, 0xF0, 0x90, 0xF0,   // 8
                              0xF0, 0x90, 0xF0, 0x10, 0xF0,   // 9
                              0xF0, 0x90, 0xF0, 0x90, 0x90,   // A
                              0xE0, 0x90, 0xE0, 0x90, 0xE0,   // B
                              0xF0, 0x80, 0x80, 0x80, 0xF0,   // C
                              0xE0, 0x90, 0x90, 0x90, 0xE0,   // D
                              0xF0, 0x80, 0xF0, 0x80, 0xF0,   // E
                              0xF0, 0x80, 0xF0, 0x80, 0x80};  // F
    
    write_to_memory(memory, 80, font);
    write_to_memory(memory, 512, buffer);

    printf("before push: size:%d\n",chip_stack.curr_elements);
    push_to_stack(&chip_stack, 0x03);
    printf("peak: %x ", peak_stack(&chip_stack));
    printf("pop: %x ", pop_stack(&chip_stack));
    printf("size: %d\n", chip_stack.curr_elements);
}