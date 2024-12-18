#include <stdio.h>
#include <stdlib.h>

void write_to_memory(unsigned char *mem, int offset, unsigned char *data){
    printf("hello function\n");
    for(int i = 0; i < sizeof(data); i++){
        mem[offset + i] = data[i];
    }
    printf("%d\n", mem[offset]);
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

    //initializing hardware
    unsigned char memory[4096];
    unsigned char delay_timer;
    unsigned char sound_timer;
    //registers
    unsigned short I;
    unsigned short pc;
    unsigned char v[16];
    
    printf("%d\n", memory);
    write_to_memory(memory, 200, buffer);
}