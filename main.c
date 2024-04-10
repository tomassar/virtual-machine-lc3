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