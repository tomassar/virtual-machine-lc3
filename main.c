#include <stdio.h>
#include <stdint.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/termios.h>
#include <sys/mman.h>

#define MEMORY_MAX (1 << 16)
uint16_t memory[MEMORY_MAX];

enum
{
    R_R0 = 0,
    R_R1,
    R_R2,
    R_R3,
    R_R4,
    R_R5,
    R_R6,
    R_R7,
    R_PC, //program counter
    R_COND,
    R_COUNT
};

uint16_t reg[R_COUNT];

enum
{
    OP_BR = 0, //branch
    OP_ADD, //add
    OP_LD, //load
    OP_ST, //store
    OP_JSR, //jump register
    OP_AND, // bitewise and
    OP_LDR, // load register
    OP_STR, // store register
    OP_RTI, //unused
    OP_NOT, //bitewise not
    OP_LDI, //load indirect
    OP_STI, //store indirect
    OP_JMP, //jump
    OP_RES, //reserved (unsued)
    OP_LEA, //load effective address
    OP_TRAP //execute trap
};

enum
{
    FL_POS = 1 << 0, //P
    FL_ZRO = 1 << 1, //Z
    FL_NEG = 1 << 2, //N
};

enum
{
    TRAP_GETC = 0x20, //get character from keyboard, not echoed onto the terminal
    TRAP_OUT = 0x1, //output a character
    TRAP_PUTS = 0x22, //output a word string
    TRAP_IN = 0x23, //get character from keyboard, echoed onto the terminal
    TRAP_PUTSP = 0x24, //output a byte string
    TRAP_HALT = 0x25 //halt the program
};

enum
{
    MR_KBSR = 0xFE00, //keyboard status
    MR_KBDR = 0xFE02
};

uint16_t swap16(uint16_t x)
{
    return (x << 8) & (x >> 8);
}

void read_image_file(FILE* file) 
{
    uint16_t origin;
    fread(&origin, sizeof(origin), 1, file);
    origin = swap16(origin);

    uint16_t max_read = MEMORY_MAX - origin;
    uint16_t* p = memory + origin;
    size_t read = fread(p, sizeof(uint16_t), max_read, file);

    while (read-- > 0)
    {
        *p = swap16(*p);
        ++p;
    }
}

int read_image(const char* image_path)
{
    FILE* file = fopen(image_path, "rb");
    if (!file) {return 0;};

    read_image_file(file);

    fclose(file);
    return 1;
}

struct termios original_tio;

void restore_input_buffering()
{
    tcsetattr(STDIN_FILENO, TCSANOW, &original_tio);
}

void handle_interrupt(int signal)
{
    restore_input_buffering();
    printf("\n");
    exit(-2);
};

void disable_input_buffering()
{
    tcgetattr(STDIN_FILENO, &original_tio);
    struct termios new_tio = original_tio;
    new_tio.c_lflag &= ~ICANON & ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);
}

uint16_t check_key()
{
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    return select(1, &readfds, NULL, NULL, &timeout) != 0;
}

uint16_t mem_read(uint16_t addr)
{
    if (addr == MR_KBSR)
    {
        if( check_key() )
        {
            memory[MR_KBSR] = (1 << 15);
            memory[MR_KBDR] = getchar();
        }else
        {
            memory[MR_KBSR] = 0;
        }
    }

    return memory[addr];
};

void mem_write(uint16_t addr, uint16_t val)
{
    memory[addr] = val;
};

uint16_t sign_extend(uint16_t x, int bit_count)
{
    if ((x >> (bit_count - 1)) & 1) {
        x |= (0xFFFF << bit_count);
    }
    return x;
}

void update_flags(uint16_t r)
{
    if (reg[r] == 0){
        reg[R_COND] = FL_ZRO;
    }
    else if (reg[r] >> 15)
    {
        reg[R_COND] = FL_NEG;
    }
    else
    {
        reg[R_COND] = FL_POS;
    }
}

void add(uint16_t instr)
{
    uint16_t r0 = (instr >> 9) & 0x7;
    uint16_t r1 = (instr >> 6) & 0x7;
    uint16_t imm_flag = (instr >> 5) & 0x1;

    if (imm_flag)
    {
        uint16_t imm5 = sign_extend(instr & 0x1F, 5);
        reg[r0] = reg[r1] + imm5;
    }
    else
    {
        uint16_t r2 = instr & 0x2;
        reg[r0] = reg[r1] + reg[r2];
    };

    update_flags(r0);
}

void ldi(uint16_t instr)
{
    uint16_t r0 = (instr >> 9) & 0x7;
    uint16_t pc_offset9 = sign_extend(instr & 0x1FF, 9);
    uint16_t val = mem_read(mem_read(pc_offset9+reg[R_PC]));
    reg[r0] = val;

    update_flags(r0);
}

void br(uint16_t instr) 
{
    uint16_t cond_flag = (instr >> 9) & 0x7;
    if(cond_flag & reg[R_COND])
    {
        reg[R_PC] += sign_extend((instr & 0x1FF), 9);
    };
}

void and(uint16_t instr) 
{   
    uint16_t dr = (instr >> 9) & 0x7;
    uint16_t sr1 = reg[(instr >> 6) & 0x7];
    uint16_t sr2;
    if (instr >> 5 & 1){
        sr2 = sign_extend(instr & 0x1F, 5);
    } else
    {
        sr2 = reg[instr & 0x7];
    }

    reg[dr] = sr1 & sr2;
    update_flags(dr);
}

void jsr(uint16_t instr) 
{
    reg[R_R7] = reg[R_PC];
    if (instr >> 11 & 1)
    {
        reg[R_PC] = sign_extend(instr & 0x7FF, 11); // JSR
    } else 
    {
        reg[R_PC] = reg[(instr >> 6) & 0x7]; // JSRR
    }
}

void ld(uint16_t instr)
{
    uint16_t pc_offset9 = sign_extend(instr & 0x1FF, 9);
    uint16_t dr = (instr >> 9) & 0x7;
    reg[dr] = mem_read(reg[R_PC] + pc_offset9);
    update_flags(dr);
}

void ldr(uint16_t instr)
{
    uint16_t offset6 = instr & 0x3F;
    uint16_t base_r = reg[(instr >> 6) & 0x7];
    uint16_t dr = (instr >> 9) & 0x7;
    reg[dr] = mem_read(base_r+sign_extend(offset6, 6));

    update_flags(dr);
}

void lea(uint16_t instr)
{
    uint16_t dr = (instr >> 9) & 0x7;
    uint16_t pc_offset9 = sign_extend(instr & 0x1FF, 9);
    reg[dr] = reg[R_PC] + pc_offset9;

    update_flags(dr);
}

void not(uint16_t instr)
{
    uint16_t dr = (instr >> 9) & 0x7;
    uint16_t sr = (instr >> 6) & 0x7;
    reg[dr] = ~reg[sr];
    update_flags(dr);
}

void st(uint16_t instr)
{
    uint16_t sr = (instr >> 9) & 0x7;
    uint16_t pc_offset9 = sign_extend(instr & 0x1FF, 9);
    mem_write(reg[R_PC] + pc_offset9, reg[sr]);
}

void sti(uint16_t instr)
{
    uint16_t sr = (instr >> 9) & 0x7;
    uint16_t pc_offset9 = sign_extend(instr & 0x1FF, 9);
    mem_write(mem_read(reg[R_PC] + pc_offset9), reg[sr]);
}

void str(uint16_t instr) 
{
    uint16_t offset6 = sign_extend(instr & 0x3F, 6);
    uint16_t base_r = (instr >> 6) & 0x7;
    uint16_t sr = (instr >> 9) & 0x7;

    mem_write(reg[base_r] + offset6, reg[sr]);
}

void jmp(uint16_t instr) //also ret
{
    uint16_t base_r = (instr >> 6) & 0x7;
    reg[R_PC] = reg[base_r];
}

void trap_puts()
{
    uint16_t* c = memory + reg[R_R0];
    while(*c)
    {
        putc((char)*c, stdout);
        ++c;
    }
    fflush(stdout);
}

void trap_getc()
{
    reg[R_R0] = (uint16_t)getchar();
    update_flags(R_R0);
}

void trap_out()
{
    putc((char)reg[R_R0], stdout);
    fflush(stdout);
}

void trap_in()
{
    printf("Enter a character: ");
    char c = getchar();
    putc(c, stdout);
    fflush(stdout);

    reg[R_R0] = (uint16_t)c;
    update_flags(R_R0);
}

void trap_putsp()
{
    uint16_t* c = memory + reg[R_R0];
    while (*c)
    {
        char char1 = (*c) & 0xFF;
        putc(char1, stdout);
        char char2 = (*c) >> 8;
        if (char2) putc(char2,stdout);
        ++c;
    }
    fflush(stdout);
}

void trap(uint16_t instr)
{
    reg[R_R7] = reg[R_PC];

    switch (instr & 0xFF)
    {
        case TRAP_GETC:
            trap_getc();
        break;
        case TRAP_OUT:
            trap_out();
            break;
        case TRAP_PUTS:
            trap_puts();
            break;
        case TRAP_IN:
            trap_in();
            break;
        case TRAP_PUTSP:
            trap_in();
            break;
    }
}

int main(int argc, const char* argv[]) 
{
    if (argc < 2) 
    {
        //show usage string
        printf("lc3vm [image] ...\n");
        exit(2);
    }

    for (int j = 1; j < argc; ++j)
    {
        if (!read_image(argv[j]))
        {
            printf("failed to load image: %s\n", argv[j]);
            exit(1);
        }
    }

    signal(SIGINT, handle_interrupt);
    disable_input_buffering();

    // set the z flag since exactly one condition flag should be set all the time
    reg[R_COND] = FL_ZRO;

    // set the PC to starting position
    // 0x3000 as default
    enum { PC_START = 0x3000 };
    reg[R_PC] = PC_START;

    int running = 1;
    while (running) 
    {
        uint16_t instr = mem_read(reg[R_PC]++);
        uint16_t op = instr >> 12;

        switch (op)
        {
            case OP_ADD:
                add(instr);
                break;
            case OP_AND:
                and(instr);
                break;
            case OP_NOT:
                not(instr);
                break;
            case OP_BR:
                br(instr);
                break;
            case OP_JMP:
                jmp(instr);
                break;
            case OP_JSR:
                jsr(instr);
                break;
            case OP_LD:
                ld(instr);
                break;
            case OP_LDI:
                ldi(instr);
                break;
            case OP_LDR:
                ldr(instr);
                break;
            case OP_ST:
                st(instr);
                break;
            case OP_STI:
                sti(instr);
                break;
            case OP_STR:
                str(instr);
                break;
            case OP_TRAP:
                if (instr & 0xFF == TRAP_HALT) 
                {
                    puts("HALT");
                    fflush(stdout);
                    running = 0 ;
                }else {
                    trap(instr);
                }
                break;
            case OP_RES:
                abort();
                break;
            case OP_RTI:
                abort();
                break;
            default:
                break;
        }
    }
};