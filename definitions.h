#ifndef DEFINITIONS_H
#define DEFINITIONS_H
#include <stdbool.h>

#define RAM_SIZE 256
#define ROM_SIZE 4096

#define SP ram[0x81]

#define PSW_CY_POS 7
#define PSW_AC_POS 6
#define PSW_F0_POS 5
#define PSW_RS1_POS 4
#define PSW_RS0_POS 3
#define PSW_OV_POS 2
// #define PSW_CY_POS 1
#define PSW_P_POS 0

#define PSW_CY getPSW(7)
#define PSW_AC getPSW(6)
#define PSW_F0 getPSW(5)
#define PSW_RS1 getPSW(4)
#define PSW_RS0 getPSW(3)
#define PSW_OV getPSW(2)
// #define PSW_CY getPSW(1)
#define PSW_P getPSW(0)

#define R0 (*R0p)
#define R1 (*R1p)
#define R2 (*R2p)
#define R3 (*R3p)
#define R4 (*R4p)
#define R5 (*R5p)
#define R6 (*R6p)
#define R7 (*R7p)

typedef bool bit;
typedef unsigned char byte;
typedef short int word;


extern const char* get_instruction[];
extern unsigned short get_instr_len[];
#endif
