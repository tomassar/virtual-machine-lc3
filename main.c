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