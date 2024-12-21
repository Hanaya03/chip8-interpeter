#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <time.h>
#include <SDL.h>

#define instructions 700

unsigned char memory[4096];
unsigned short instruction;
int start_of_program = 512;
//registers
unsigned short I;
int pc = 0;
unsigned char v[16];

void write_to_memory(int offset, unsigned char *data, int element_count){
    for(int i = 0; i < element_count; i++){
        memory[offset + i] = data[i];
    }
}

struct stack{
    unsigned short elements[16];
    int curr_elements;
};

typedef struct stack Stack;

void push_to_stack(Stack *stackptr, unsigned char data){
    stackptr->elements[stackptr->curr_elements] = data;
    stackptr->curr_elements++;
}

unsigned char pop_stack(Stack *stackptr){
    stackptr->curr_elements--;
    return stackptr->elements[stackptr->curr_elements];
}

unsigned char peak_stack(Stack *stackptr){
    int tmp = stackptr->curr_elements - 1;
    return stackptr->elements[tmp];
}

DWORD WINAPI delay_loop(LPVOID lpParam){
    double update_rate = 1/60;
    unsigned char *timer = (unsigned char *)lpParam;
    clock_t t = clock();
    while(TRUE){
        if((double)t + update_rate <= (double)clock()){
            (*timer)--;
            t = clock();
            sleep(0);
            // printf("value of timer at %d:%d\n", timer, *timer);
        }

        // if(*timer == 0x00)
        //     break;
    }
    return 0;
}

void key_in(){
    switch (getch()){
    case '1':
        //do something
        break;
    
    default:
        break;
    }
}

//store the instruction at pc and pc + 1
//increment pc by 2
void fetch_instruction(){
    instruction = memory[start_of_program + pc];
    pc++;
    instruction = instruction << 8;
    instruction += memory[start_of_program + pc];
    pc++;
}

void decode_instruction(){

}

void execute_instruction(){

}

int main(int argc, char** argv) {
    //initialize sdl
	// Initialize SDL. SDL_Init will return -1 if it fails.
    printf("1");
	if ( SDL_Init(SDL_INIT_EVERYTHING ) < 0 ) {
		printf("Error initializing SDL: %s", SDL_GetError());
		system("pause");
		// End the program
		return 1;
	}
    printf("2");
    SDL_Window* win = SDL_CreateWindow( "Chip8 Emulator", 100, 100, 64, 32, SDL_WINDOW_SHOWN );
    if ( !win ) {
        printf("Failed to create a window! Error: %s\n", SDL_GetError());
    }
    printf("3");
    SDL_Surface* winSurface = SDL_GetWindowSurface( win );

    SDL_FillRect( winSurface, NULL, SDL_MapRGB( winSurface->format, 255, 255, 255 ) );

    SDL_UpdateWindowSurface( win );

    printf("all good");

    //getting input file
    FILE *fileptr;
    unsigned char *buffer;
    long filelen;

    fileptr = fopen(argv[1], "rb");
    fseek(fileptr, 0, SEEK_END);
    filelen = ftell(fileptr);
    rewind(fileptr);

    buffer = (char *)malloc(filelen);
    fread(buffer, 1, filelen, fileptr);
    fclose(fileptr);

    //initializing components
    unsigned char delay_timer = 0xff;
    unsigned char sound_timer = 0xff;
    double chip_update_rate = 1/instructions;
    clock_t t;
    Stack chip_stack;
    chip_stack.curr_elements = 0;

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

    write_to_memory(80, font, 80);
    write_to_memory(start_of_program, buffer, filelen);

    // printf("delay timer at %d, and sound timer at %d\n", &delay_timer, &sound_timer);
    HANDLE delay_thread = CreateThread(NULL, 0, delay_loop, &delay_timer, 0, NULL);
    HANDLE sound_thread = CreateThread(NULL, 0, delay_loop, &sound_timer, 0, NULL);
    HANDLE threadArray[2] = {delay_thread, sound_thread};

    t = clock();
    while(TRUE){
        if((double)t + chip_update_rate <= clock()){  
            fetch_instruction();
            decode_instruction();
        }
    }
    WaitForMultipleObjects(2, threadArray, TRUE, INFINITE);

    CloseHandle(threadArray[0]);
    CloseHandle(threadArray[1]);

    SDL_DestroyWindow(win);
}