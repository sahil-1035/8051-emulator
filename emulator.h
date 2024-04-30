#ifndef EMULATOR_H
#define EMULATOR_H

#include <pthread.h>

#include "definitions.h"
#include "utils/set.h"

extern pthread_mutex_t data_mutex;

typedef enum EMU_ReturnCause {
	ROM_CANT_BE_ACCESSED, END_OF_ROM, UNEXPECTED_QUIT
} EMU_ReturnCause;

typedef enum EMU_State {
	EMU_RUNNING, EMU_BREAKPOINT, EMU_CONTINUE
} EMU_State;

extern EMU_ReturnCause emu_return_cause;
extern EMU_State emu_state;

extern Set* breakpoints;
extern pthread_cond_t breakpoint_cond;
extern int emu_step_point;

extern const float XTALfreq;

extern unsigned short ROM_FILE_LEN;

extern byte rom[ROM_SIZE];
extern byte ram[RAM_SIZE];
extern byte* bit_addressable_map[32];

extern word pc;
void set_dptr(word val);
word get_dptr(void);

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

void emu_init(const char* ROMpath);
void emu_start(void);
void emu_reset(void);
void emu_exec_instr(void);
void emu_clear_ram(void);

word emu_get_next_instr();

void emu_load_ROM(const char* ROMpath);
#endif
