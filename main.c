#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <signal.h>

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

int read_image(const char* image_path)
{
    return 1;
};

void handle_interrupt(int signal)
{
};

void disable_input_buffering()
{
};

uint16_t mem_read(uint16_t addr)
{

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
    uint16_t pc_offset9 = sign_extend(instr & 0xFF, 9);
    uint16_t val = mem_read(mem_read(pc_offset9+reg[R_PC]));
    reg[r0] = val;

    update_flags(r0);
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
                break;
            case OP_NOT:
                break;
            case OP_BR:
                break;
            case OP_JMP:
                break;
            case OP_JSR:
                break;
            case OP_LD:
                break;
            case OP_LDI:
                ldi(instr);
                break;
            case OP_LDR:
                break;
            case OP_ST:
                break;
            case OP_STI:
                break;
            case OP_STR:
                break;
            case OP_TRAP:
                break;
            case OP_RES:
                break;
            case OP_RTI:
                break;
            default:
                break;
        }
    }
};