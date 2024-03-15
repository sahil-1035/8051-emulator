#ifndef EMULATOR_H
#define EMULATOR_H

#include "definitions.h"
extern unsigned short ROM_FILE_LEN;

extern byte rom[ROM_SIZE];
extern byte ram[RAM_SIZE];

extern word pc;
extern word dptr;

extern byte sp;
extern byte a;
extern byte b;
extern byte psw;

extern short int register_bank;
extern byte *R0p, *R1p, *R2p, *R3p, *R4p, *R5p, *R6p, *R7p, *R8p;

bit getBit(byte address);
void writeBit(bit val, byte address);

bool getPSW(short int pos);
void setPSW(short int pos, bool val);
void add_to_A(byte addend);
void sub_from_A(byte subtrahend);
void change_bank(short int new_bank_val);

extern bool emu_quit;

void emu_start();
void emu_exec_instr();
void emu_clear_ram();

void emu_load_ROM(const char* ROMpath);
#endif
