#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <time.h>
#include <SDL.h>

#define instructions 700
#define screen_width 64
#define screen_height 32

SDL_Window* win;
SDL_Surface* winSurface;
Uint32 white;
Uint32 black;
unsigned char memory[4096];
unsigned short instruction;
int start_of_program = 512;
//registers
unsigned short I;
unsigned short PC = 0x0200;
unsigned char v[16];

void write_to_memory(int offset, unsigned char *data, int element_count){
    for(int i = 0; i < element_count; i++){
        memory[offset + i] = data[i];
    }
}

boolean set_pixel(SDL_Surface *surface, int x, int y)
{
    Uint32 *const target_pixel = (Uint32 *) ((Uint8 *) surface->pixels
                                             + y * surface->pitch
                                             + x * surface->format->BytesPerPixel);
    
    if(*target_pixel == white){
        *target_pixel = black;
        return TRUE;
    }
    *target_pixel = white;
    return FALSE;
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
        }
    }
    return 0;
}

void clear_screen(){
    SDL_FillRect(winSurface, NULL, black);
    SDL_UpdateWindowSurface(win);
    printf("clearing screen\n");
}

void jump_to_address(unsigned short addr){
    printf("setting PC to %x, was %x, ", addr, PC);
    PC = addr;
    printf(" is %x\n", PC);
}

void set_register(int reg, unsigned char value){
    v[reg] = value;
    printf("setting register %d to %x, is now %x\n", reg, value, v[reg]);
}

void add_to_register(int reg, unsigned char value){
    v[reg] += value;
    printf("adding %x to register %d, is now %x\n", value, reg, v[reg]);
}

void set_index_register(unsigned short value){
    I = value;
    printf("setting I to %x, is now %x\n", value, I);
}

void draw_to_screen(int x, int y, int rows){
    printf("drawing a sprite at x:%d, y:%d; %d pixels tall\n", x, y, rows);
    v[15] = 0x00;
    for(int i = 0; i < rows; i++){
        
        if(y + i > screen_height){break;}

        for(int j = 0; j < 8; j++){

            if(((memory[I + i] >> 7 - j) & 0x01) == 0x01){
                if(x + j > screen_width){break;}

                if(set_pixel(winSurface, x + j, y + i)){
                    v[15] = 0x01;
                }
            }
        }
    }
    SDL_UpdateWindowSurface(win);
}

//get the instruction at pc and pc + 1
//increment pc by 2
void fetch_instruction(){
    instruction = memory[(int)PC];
    PC++;
    instruction = instruction << 8;
    instruction += memory[PC];
    PC++;
}

void decode_then_execute_instruction(){
    switch (instruction >> 12)
    {
    case 0x00:
        clear_screen();
        break;
    case 0x01:
        jump_to_address(instruction & 0x0fff);
        break;
    case 0x06:
        set_register((int)(instruction >> 8) & 0x0f, instruction);
        break;
    case 0x07:
        add_to_register((int)(instruction >> 8) & 0x0f, instruction & 0xff);
        break;
    case 0x0a:
        set_index_register(instruction & 0x0fff);
        break;
    case 0x0d:
        draw_to_screen((int)(v[(int)(instruction >> 8) & 0x0f] & 63),
                       (int)(v[(int)(instruction >> 4) & 0x0f] & 31),
                       (int)instruction & 0x0f);
        break;
    default:
        break;
    }
}

void execute_instruction(){

}


int main(int argc, char** argv) {
	// Initialize SDL. SDL_Init will return -1 if it fails.
	if ( SDL_Init(SDL_INIT_EVERYTHING ) < 0 ) {
		printf("Error initializing SDL: %s", SDL_GetError());
		system("pause");
		// End the program
		return 1;
	}
    win = SDL_CreateWindow( "Chip8 Emulator", 100, 100, 64, 32, SDL_WINDOW_SHOWN );
    if ( !win ) {
        printf("Failed to create a window! Error: %s\n", SDL_GetError());
    }
    winSurface = SDL_GetWindowSurface( win );

    //Reading the bytes of input file into a buffer
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
    black = SDL_MapRGB(winSurface->format, 0, 0, 0);
    white = SDL_MapRGB(winSurface->format, 255, 255, 255);
    unsigned char delay_timer = 0xff;
    unsigned char sound_timer = 0xff;
    double chip_update_rate = 1/instructions;
    clock_t t;
    Stack chip_stack;
    chip_stack.curr_elements = 0;

    SDL_FillRect( winSurface, NULL, black);
    SDL_UpdateWindowSurface(win);

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

    // HANDLE delay_thread = CreateThread(NULL, 0, delay_loop, &delay_timer, 0, NULL);
    // HANDLE sound_thread = CreateThread(NULL, 0, delay_loop, &sound_timer, 0, NULL);
    // HANDLE threadArray[2] = {delay_thread, sound_thread};

    t = clock();
    int i = 0;
    while(TRUE){
        if((double)t + chip_update_rate <= clock()){
            fetch_instruction();
            decode_then_execute_instruction();
        }
        i++;
    }
    // WaitForMultipleObjects(2, threadArray, TRUE, INFINITE);

    // CloseHandle(threadArray[0]);
    // CloseHandle(threadArray[1]);
    Sleep(10000);
    SDL_DestroyWindow(win);
}