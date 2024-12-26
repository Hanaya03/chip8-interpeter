#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <windows.h>
#include <time.h>
#include <SDL.h>

#define instructions 700
#define screen_width 64
#define screen_height 32

struct stack{
    unsigned short elements[32];
    int curr_elements;
};

SDL_Event event;
boolean shift_flag = FALSE;
boolean jump_flag = FALSE;
typedef struct stack Stack;
SDL_Window* win;
SDL_Surface* winSurface;
Uint32 white;
Uint32 black;
unsigned char delay_timer;
unsigned char sound_timer;
unsigned char memory[4096];
unsigned short instruction;
int start_of_program = 512;
Stack chip_stack;
//registers
unsigned short I;
unsigned short PC = 0x0200;
unsigned char v[16];

unsigned char handle_input(int scancode){
    printf("comparing key: %d", event.key.keysym.scancode);
    switch (scancode)
    {
    case SDL_SCANCODE_1:
        printf("input: 0x01\n");
        return 0x01;
        break;
    case SDL_SCANCODE_2:
        printf("input: 0x02\n");
        return 0x02;
        break;
    case SDL_SCANCODE_3:
        printf("input: 0x03\n");
        return 0x03;
        break;
    case SDL_SCANCODE_4:
        printf("input: 0x0c\n");
        return 0x0c;
        break;
    case SDL_SCANCODE_Q:
        printf("input: 0x04\n");
        return 0x04;
        break;
    case SDL_SCANCODE_W:
        printf("input: 0x05\n");
        return 0x05;
        break;
    case SDL_SCANCODE_E:
        printf("input: 0x06\n");
        return 0x06;
        break;
    case SDL_SCANCODE_R:
        printf("input: 0x0d\n");
        return 0x0d;
        break;
    case SDL_SCANCODE_A:
        printf("input: 0x07\n");
        return 0x07;
        break;
    case SDL_SCANCODE_S:
        printf("input: 0x08\n");
        return 0x08;
        break;
    case SDL_SCANCODE_D:
        printf("input: 0x09\n");
        return 0x09;
        break;
    case SDL_SCANCODE_F:
        printf("input: 0x0e\n");
        return 0x0e;
        break;
    case SDL_SCANCODE_Z:
        printf("input: 0x0a\n");
        return 0x0a;
        break;
    case SDL_SCANCODE_X:
        printf("input: 0x00\n");
        return 0x00;
        break;
    case SDL_SCANCODE_C:
        printf("input: 0x0b\n");
        return 0x0b;
        break;
    case SDL_SCANCODE_V:
        printf("input: 0x0f\n");
        return 0x0f;
        break;
    default:
        return 0x10;
        break;
    }
}

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

void push_to_stack(Stack *stackptr, unsigned short data){
    stackptr->elements[stackptr->curr_elements] = data;
    stackptr->curr_elements++;
}

unsigned short pop_stack(Stack *stackptr){
    stackptr->curr_elements--;
    return stackptr->elements[stackptr->curr_elements];
}

unsigned short peak_stack(Stack *stackptr){
    int tmp = stackptr->curr_elements - 1;
    return stackptr->elements[tmp];
}

DWORD WINAPI delay_loop(LPVOID lpParam){
    double update_rate = 1/60;
    unsigned char *timer = (unsigned char *)lpParam;
    clock_t t = clock();
    while(TRUE){
        if((double)t + update_rate <= (double)clock() && timer > 0){
            (*timer)--;
            t = clock();
            sleep(0);
        }
    }
    return 0;
}

void shift_register_right(int reg1, int reg2){
    if(shift_flag){v[reg1] = v[reg2];}
    printf("shifting register %d(%x) to the right. ", reg1, v[reg1]);
    if((v[reg1] & 0x01) == 0x01){
        printf("overflow. ");
        v[reg1] = v[reg1] >> 1;
        v[15] = 0x01;
    }else{
        v[reg1] = v[reg1] >> 1;
        v[15] = 0x00;
    }
    
    printf("is now %x\n", v[reg1]);
}

void shift_register_left(int reg1, int reg2){
    if(shift_flag){v[reg1] = v[reg2];}
    printf("shifting register %d(%x) to the left. ", reg1, v[reg1]);
    if((v[reg1] & 0x80) == 0x80){
        printf("overflow. ");
        v[reg1] = v[reg1] << 1;
        v[15] = 0x01;
    }else{
        v[reg1] = v[reg1] << 1;
        v[15] = 0x00;
    }

    
    printf("is now %x\n", v[reg1]);
}

void wait_for_key(int reg){
    printf("waiting on %x input\n", v[reg]);
    if(event.type == SDL_KEYUP){
        printf("input\n");
        if(handle_input(event.key.keysym.scancode) < 0x10){
            PC = v[reg];
        }else{PC  -= 2;}
    }
}

void skip_if_key_down(int reg){
    printf("checking for %x input\n", v[reg]);
    if(event.type == SDL_KEYDOWN){
        if(handle_input(event.key.keysym.scancode) == v[reg]){
            PC += 2;
        }
    }
}

void skip_if_key_not_down(int reg){
    printf("checking for NOT %x input\n", v[reg]);
    if(event.type == SDL_KEYDOWN){
        if(handle_input(event.key.keysym.scancode) != v[reg]){
            printf("true\n");
            PC += 2;
        }
    }
}

void if_equal(int reg, unsigned char value){
    printf("checking if register %d(%x) is equal to %x\n", reg, v[reg], value);
    if (v[reg] == value){
        printf("true\n");
        PC += 2;
    }
    
}

void if_registers_equal(int reg1, int reg2){
    printf("checking if registers %d(%x) and %d(%x) are equal", reg1, v[reg1], reg2, v[reg2]);
    if (v[reg1] == v[reg2]){
        printf("true\n");
        PC += 2;
    }
    
}

void if_not_equal(int reg, unsigned char value){
    printf("checking if register %d(%x) is not equal to %x\n", reg, v[reg], value);
    if (v[reg] != value){
        printf("true\n");
        PC += 2;
    }
    
}

void if_registers_not_equal(int reg1, int reg2){
    printf("checking if registers %d(%x) and %d(%x) are not equal\n", reg1, v[reg1], reg2, v[reg2]);
    if (v[reg1] != v[reg2]){
        printf("true! skipping pc %d\n", PC);
        PC += 2;
    }
    
}

void return_from_subroutine(){
    printf("returing from subroutine. setting PC(%x) to %x. ", PC, peak_stack(&chip_stack));
    PC = pop_stack(&chip_stack);
    printf("PC is now %x\n", PC);
}

void call_subroutine(unsigned short addr){
    printf("pushing %x to stack. ", PC);
    push_to_stack(&chip_stack, PC);
    printf("is now %x. setting PC to %x. ", peak_stack(&chip_stack), addr);
    PC = addr;
    printf("PC is now %x\n", PC);
}

void clear_screen(){
    SDL_FillRect(winSurface, NULL, black);
    SDL_UpdateWindowSurface(win);
}

void jump_to_address(unsigned short addr){
    PC = addr;
    printf("setting PC to %x\n", addr);
}

void jump_with_offset(unsigned short addr){
    printf("Changing PC to %x, was", addr, PC);
    PC = addr;
    printf("is now %x\n", PC);
    if (jump_flag){
        printf("adding register 0(%x) to PC(%x)", v[0], PC); 
        PC += v[0];}
    else{
        printf("adding register %d(%x) to PC(%x)", (int)(addr >> 8) & 0x0f, v[(int)(addr >> 8) & 0x0f], PC); 
        PC += v[(int)(addr >> 8) & 0x0f];}
}

void set_register(int reg, unsigned char value){
    printf("setting register %d(%x) to %x ", reg, v[reg], value);
    v[reg] = value;
    printf("is now %x\n", v[reg]);
}

void set_random_value(int reg, unsigned char value){
    v[reg] = ((unsigned char)rand() % 255) & value;
}

void add_registers(int reg1, int reg2){
    unsigned short tmp = v[reg1] + v[reg2];
    printf("adding registers %d(%x) and %d(%x)", reg1, v[reg1], reg2, v[reg2]);
    v[reg1] = tmp;
    printf(" stored %x into %d\n", v[reg1], reg1);
    v[15] = 0x00;
    if(tmp > 0xff){
        printf("overflow! ");
        v[15] = 0x01; 
    }

}

void sub_registers(int dest, int reg1, int reg2){
    unsigned short tmp = v[reg1] - v[reg2];
    printf("subtracting %d(%x) from %d(%x). storing into %d ", reg2, v[reg2], reg1, v[reg1], dest);
    if(v[reg2] <= v[reg1]){v[dest] = tmp;v[15] = 0x01;}
    else{v[dest] = tmp;v[15] = 0x00;}
    printf("result is %x\n", v[dest]);
}

void add_to_register(int reg, unsigned char value){
    v[15] = 0x00; 
    printf("adding %x to register %d(%x) ", value, reg, v[reg]);
    v[reg] += value;
    printf("is now %x\n", v[reg]);
}

void set_index_register(unsigned short value){
    printf("setting Index to %x was %x ", value, I);
    I = value;
    printf("is now %x\n", I);
}

void draw_to_screen(int x, int y, int rows){
    printf("printing to x: %d, y: %d, height: %d. from sprite at %d\n", x, y, rows, I);
    for(int i = 0; i < rows; i++){

        if(y + i > screen_height){break;}

        for(int j = 0; j < 8; j++){

            if(((memory[I + i] >> 7 - j) & 0x01) == 0x01){
                if(x + j > screen_width){break;}

                if(set_pixel(winSurface, x + j, y + i)){
                    printf("pixel at %d, %d, was white\n", x + j, y + i);
                    v[15] = 0x01;
                }else{
                    v[15] = 0x00;
                }
            }
        }
    }
    SDL_UpdateWindowSurface(win);
}

void determine_bitwise_op(unsigned char last_nibble){
    switch (last_nibble)
    {
    case 0x00:
        //set register of 2nd nibble to the value of register of 3rd nibble
        set_register((int)(instruction >> 8) & 0x0f, v[(int)(instruction >> 4) & 0x0f]);
        break;
    case 0x01:
        //bitwise OR registers of 2nd and 3rd nibble. store value in register of 2nd nibble
        set_register((int)(instruction >> 8) & 0x0f,
                     v[(int)(instruction >> 8) & 0x0f] | v[(int)(instruction >> 4) & 0x0f]);
        break;
    case 0x02:
        //bitwise AND registers of 2nd and 3rd nibble. store value in register of 2nd nibble
        set_register((int)(instruction >> 8) & 0x0f,
                     v[(int)(instruction >> 8) & 0x0f] & v[(int)(instruction >> 4) & 0x0f]);
        break;
    case 0x03:
        //bitwise XOR register of 2nd and 3rd nibble. store value in register of 2nd nibble
        set_register((int)(instruction >> 8) & 0x0f,
                     v[(int)(instruction >> 8) & 0x0f] ^ v[(int)(instruction >> 4) & 0x0f]);
        break;
    case 0x04:
        //add registers of 2nd and 3rd nibble. store in register of 2nd nibble
        add_registers((int)((instruction >> 8) & 0x0f), (int)((instruction >> 4) & 0x0f));
        break;
    case 0x05:
        sub_registers((int)(instruction >> 8) & 0x0f,
                      (int)(instruction >> 8) & 0x0f,
                      (int)(instruction >> 4) & 0x0f);
        break;
    case 0x06:
        shift_register_right((int)(instruction >> 8) & 0x0f,
                             (int)(instruction >> 4) & 0x0f);
        break;
    case 0x07:
        sub_registers((int)(instruction >> 8) & 0x0f,
                      (int)(instruction >> 4) & 0x0f,
                      (int)(instruction >> 8) & 0x0f);
        break;
    case 0x0e:
        shift_register_left((int)(instruction >> 8) & 0x0f,
                             (int)(instruction >> 4) & 0x0f);
        break;
    default:
        break;
    }
}

void determine_skip_op(){
    if(instruction & 0x00ff == 0x009e){
        skip_if_key_down((int)(instruction >> 8) & 0x0f);
    }else{
        skip_if_key_not_down((int)(instruction >> 8) & 0x0f);
    }
}

void set_index_to_font(int reg){
    switch (v[reg])
    {
    case 0x00:
        set_index_register(0x0050);
        break;
    case 0x01:
        set_index_register(0x0050 + 5);
        break;
    case 0x02:
        set_index_register(0x0050 + 10);
        break;
    case 0x03:
        set_index_register(0x0050 + 15);
        break;
    case 0x04:
        set_index_register(0x0050 + 20);
        break;
    case 0x05:
        set_index_register(0x0050 + 25);
        break;
    case 0x06:
        set_index_register(0x0050 + 30);
        break;
    case 0x07:
        set_index_register(0x0050 + 35);
        break;
    case 0x08:
        set_index_register(0x0050 + 40);
        break;
    case 0x09:
        set_index_register(0x0050 + 45);
        break;
    case 0x0a:
        set_index_register(0x0050 + 50);
        break;
    case 0x0b:
        set_index_register(0x0050 + 55);
        break;
    case 0x0c:
        set_index_register(0x0050 + 60);
        break;
    case 0x0d:
        set_index_register(0x0050 + 65);
        break;
    case 0x0e:
        set_index_register(0x0050 + 70);
        break;
    case 0x0f:
        set_index_register(0x0050 + 75);
        break;
    default:
        break;
    }
}

void determine_f_op(int reg, unsigned char value){
    printf("checking for f op with register %d and value %x\n", reg, value);
    switch (value)
    {
    case 0x07:
        v[reg] = delay_timer;
        printf("setting register %d(%x) to delay_timer\n", reg, v[reg]);
        break;
    case 0x15:
        delay_timer = v[reg];
        printf("setting delay timer to register %d(%x)\n", reg, v[reg]);
        break;
    case 0x18:
        sound_timer = v[reg];
        printf("setting sound timer to register %d(%x)\n", reg, v[reg]);
        break;
    case 0x1e:
        printf("setting Index(%x) to register %d(%x) ", I, reg, v[reg]);
        I += v[reg];
        printf("is now %x\n", I);
        if(I >= 0x1000){ printf("overflow\n"); v[15] = 0x01;}
        break;
    case 0x0a:
        wait_for_key(reg);
        break;
    case 0x29:
        set_index_to_font(reg);
        break;
    case 0x33:
        memory[I] = floor(v[reg] / 100);
        memory[I + 1] = floor(v[reg] / 10);
        memory[I + 1] = memory[I + 1] % 10;
        memory[I + 2] = v[reg] % 10;
        break;
    case 0x55:
        for(int i = 0; i <= reg; i++){
            memory[(int)I + i] = v[i];
        }
        break;
    case 0x65:
        for(int i = 0; i <= reg; i++){
            v[i] = memory[(int)I + i];
        }
        break;
    default:
        break;
    }
}

//get the instruction at pc and pc + 1
//increment pc by 2
void fetch_instruction(){
    instruction = memory[(int)PC];
    PC++;
    instruction = instruction << 8;
    instruction += memory[PC];
    PC++;

    printf("current instruction: %x\n", instruction);
}

void decode_then_execute_instruction(){
    switch (instruction >> 12)
    {
    case 0x00:
        if((instruction & 0x00ff) == 0x00e0){clear_screen();}
        else{return_from_subroutine();}
        break;
    case 0x01:
        jump_to_address(instruction & 0x0fff);
        break;
    case 0x02:
        call_subroutine(instruction & 0x0fff);
        break;
    case 0x03:
        if_equal((int)(instruction >> 8) & 0x0f, instruction);
        break;
    case 0x04:
        if_not_equal((int)(instruction >> 8) & 0x0f, instruction);
        break;
    case 0x05:
        if_registers_equal((int)(instruction >> 8) & 0x0f, (int)(instruction & 0xf0));
        break;
    case 0x06:
        set_register((int)(instruction >> 8) & 0x0f, instruction);
        break;
    case 0x07:
        add_to_register((int)(instruction >> 8) & 0x0f, instruction & 0xff);
        break;
    case 0x08:
        determine_bitwise_op(instruction & 0x0f);
        break;
    case 0x09:
        if_registers_not_equal((int)(instruction >> 8) & 0x0f, (int)(instruction >> 4 & 0x0f));
        break;
    case 0x0a:
        set_index_register(instruction & 0x0fff);
        break;
    case 0x0b:
        jump_with_offset(instruction & 0x0fff);
        break;
    case 0x0c:
        set_random_value((int)(v[(int)(instruction >> 8) & 0x0f]),
                               instruction & 0xff);
        break;
    case 0x0d:
        draw_to_screen((int)(v[(int)(instruction >> 8) & 0x0f] & 63),
                       (int)(v[(int)(instruction >> 4) & 0x0f] & 31),
                       (int)instruction & 0x0f);
        break;
    case 0x0e:
        determine_skip_op();
        break;
    case 0x0f:
        determine_f_op((int)((int)(instruction >> 8) & 0x0f),
                       instruction & 0x00ff);
        break;
    default:
        break;
    }
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
    if(argc == 4){fileptr = fopen(argv[3], "rb"); shift_flag = TRUE; jump_flag = TRUE;}
    else if(argc == 3){
        fileptr = fopen(argv[2], "rb");
        if(argv[1] == "-v"){    shift_flag == TRUE;}
        else{                   jump_flag == TRUE;}
    }else if(argc == 2){fileptr = fopen(argv[1], "rb");}
    else{
        printf("ERROR: incorrect number of parameters");
		return 1;
    }
    fseek(fileptr, 0, SEEK_END);
    filelen = ftell(fileptr);
    rewind(fileptr);
    buffer = (char *)malloc(filelen);
    fread(buffer, 1, filelen, fileptr);
    fclose(fileptr);

    //initializing components
    black = SDL_MapRGB(winSurface->format, 0, 0, 0);
    white = SDL_MapRGB(winSurface->format, 255, 255, 255);
    delay_timer = 0xff;
    sound_timer = 0xff;
    double chip_update_rate = 1/instructions;
    clock_t t;
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

    HANDLE delay_thread = CreateThread(NULL, 0, delay_loop, &delay_timer, 0, NULL);
    HANDLE sound_thread = CreateThread(NULL, 0, delay_loop, &sound_timer, 0, NULL);
    HANDLE threadArray[2] = {delay_thread, sound_thread};

    t = clock();
    while(TRUE){
        SDL_PollEvent(&event);
        if((double)t + chip_update_rate <= clock()){
            // SDL_WaitEvent(&event);
            fetch_instruction();
            decode_then_execute_instruction();
            // sleep(1);
        }
    }
    // WaitForMultipleObjects(2, threadArray, TRUE, INFINITE);

    // CloseHandle(threadArray[0]);
    // CloseHandle(threadArray[1]);
    SDL_DestroyWindow(win);
}