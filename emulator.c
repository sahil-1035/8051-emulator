#include "emulator.h"
#include "definitions.h"
#include "utils/set.h"

#include <bits/time.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

EMU_ReturnCause emu_return_cause = UNEXPECTED_QUIT;
EMU_State emu_state = EMU_RUNNING;
pthread_mutex_t data_mutex;

Set* breakpoints;
pthread_cond_t breakpoint_cond;
int emu_step_point = -1;

const float XTALfreq = 11.0592f;
const float timePeriodPerMachineCycle = 1000.0f / (XTALfreq / 12);

byte rom[ROM_SIZE];
byte ram[RAM_SIZE];
byte* bit_addressable_map[32];

unsigned short ROM_FILE_LEN = 0;

word pc = 0x0000;

short int register_bank = 0;
byte *R0p, *R1p, *R2p, *R3p, *R4p, *R5p, *R6p, *R7p, *R8p;

bool emu_quit = false;


void set_dptr(word val)
{
	dpl = val & 0b11111111;
	dph = (val & (0b11111111 << 8)) >> 8;
}

word get_dptr(void)
{
	word val = 0;
	val |= dpl;
	val |= dph << 8;
	return val;
}

static void set_up_bit_addresible_map(void)
{
	for(unsigned int i = 0; i < 16; i++)
		bit_addressable_map[i] = &ram[0x20 + i];
	// P0
	bit_addressable_map[16] = &ram[0x80];
	// TCON
	bit_addressable_map[17] = &ram[0x88];
	// P1
	bit_addressable_map[18] = &ram[0x90];
	// SCON
	bit_addressable_map[19] = &ram[0x98];
	// P2
	bit_addressable_map[20] = &ram[0xA0];
	// IE
	bit_addressable_map[21] = &ram[0xA8];
	// P3
	bit_addressable_map[22] = &ram[0xB0];
	// IP
	bit_addressable_map[23] = &ram[0xB8];
	bit_addressable_map[24] = NULL;
	bit_addressable_map[25] = NULL;
	// PSW
	bit_addressable_map[26] = &ram[0xD0];
	bit_addressable_map[27] = NULL;
	// ACC
	bit_addressable_map[28] = &ram[0xE0];
	bit_addressable_map[29] = NULL;
	// B
	bit_addressable_map[30] = &ram[0xF0];
	bit_addressable_map[31] = NULL;
}

int emu_init(const char* ROMpath)
{
	breakpoints = create_set(20);
	insert_set(breakpoints, 0);
	emu_step_point = -1;
	emu_state = EMU_RUNNING;
	if ( emu_load_ROM(ROMpath) == -1 )
		return -1;
	change_bank(0);
	emu_clear_ram();
	set_up_bit_addresible_map();
	return 0;
}

void emu_reset(void)
{
	clear_set(breakpoints);
	insert_set(breakpoints, 0);
	emu_step_point = -1;
	change_bank(0);
	emu_clear_ram();
}

int emu_load_ROM(const char* ROMpath)
{
	FILE* ROMfile = fopen(ROMpath, "rb");
	if (ROMfile == NULL)
	{
		emu_return_cause = ROM_CANT_BE_ACCESSED;
		return -1;
	}
	fseek(ROMfile, 0, SEEK_END);
	ROM_FILE_LEN = ftell(ROMfile);
	fseek(ROMfile, 0, SEEK_SET);
	fread(rom, sizeof(rom), 1, ROMfile);
	fclose(ROMfile);
	return 0;
}


void emu_clear_ram(void)
{
	pc = 0x0000;
	set_dptr(0);
	a = 0x00;
	b = 0x00;
	register_bank = 0;

	for(int i = 0; i < RAM_SIZE; i++)
	{
		ram[i] = 0x000;
	}

	SP = 0x07;
}
void add_to_A(byte addend)
{
	if ( a > ( a + addend))
		setPSW(PSW_CY_POS, 1);
	if ( (a & 0b00001111) < ((a + addend) & 0b00001111) )
		setPSW(PSW_AC_POS, 1);
	a = a + addend;
}

void sub_from_A(byte subtrahend)
{
	a = a - subtrahend;
}

void change_bank(short int new_bank_val)
{
	register_bank = new_bank_val;
	R0p = 8 * register_bank * sizeof(byte) + ram;
	R1p = 8 * register_bank * sizeof(byte) + ram + 1 * sizeof(byte);
	R2p = 8 * register_bank * sizeof(byte) + ram + 2 * sizeof(byte);
	R3p = 8 * register_bank * sizeof(byte) + ram + 3 * sizeof(byte);
	R4p = 8 * register_bank * sizeof(byte) + ram + 4 * sizeof(byte);
	R5p = 8 * register_bank * sizeof(byte) + ram + 5 * sizeof(byte);
	R6p = 8 * register_bank * sizeof(byte) + ram + 6 * sizeof(byte);
	R7p = 8 * register_bank * sizeof(byte) + ram + 7 * sizeof(byte);
	R8p = 8 * register_bank * sizeof(byte) + ram + 8 * sizeof(byte);
}

bool getPSW(short int pos)
{
	return psw & (1 << pos);
}

void setPSW(short int pos, bool val)
{
	psw = val ? (psw | (1 << pos)) : psw & !(1 << pos);
}

bit getBit(byte address)
{
	return (bit)((*bit_addressable_map[address / 8]) & ( 0b00000001 << ( address % 8 )));
}

void writeBit(bit val, byte address)
{
	bit prevVal = (*bit_addressable_map[address / 8]) & (0b00000001 << (address % 8));
	if (prevVal == val)
		return;
	(*bit_addressable_map[address / 8]) = prevVal ?
		(*bit_addressable_map[address / 8]) & (~(0b00000001 << (address % 8)))
		: (*bit_addressable_map[address / 8]) | (0b00000001 << (address % 8));
	return;
}

static void manage_timers(void)
{
	if ( TR0 && !TF0 )
	{
		switch (TMOD & 0b00000011)
		{
			case 1:
				if ( TL0 > (byte)(TL0 + 1) )
				{
					if ( TH0 > (byte)(TH0 + 1) )
						writeBit(1, TF0_POS);
					TH0++;
				}
				TL0++;
				break;
			case 2:
				if ( TL0 < TH0 )
					TL0 = TH0;
				if ( TL0 > (byte)(TL0 + 1) )
				{
					writeBit(1, TF0_POS);
					TL0 = TH0;
				}
				TL0++;
				break;
		}
	}
	if ( TR1 && !TF1 )
	{
		switch (TMOD & 0b00000011)
		{
			case 1:
				if ( TL1 > (byte)(TL1 + 1) )
				{
					if ( TH1 > (byte)(TH1 + 1) )
						writeBit(1, TF1_POS);
					TH1++;
				}
				TL1++;
				break;
			case 2:
				if ( TL1 < TH1 )
					TL1 = TH1;
				if ( TL1 > (byte)(TL1 + 1) )
				{
					writeBit(1, TF1_POS);
					TL1 = TH1;
				}
				TL1++;
				break;
		}
	}
}


word emu_get_next_instr()
{
	return pc + instructions[rom[pc]].no_of_bytes;
}

void emu_start(void)
{
	while ( !emu_quit )
	{
		pthread_mutex_unlock(&data_mutex);
		emu_exec_instr();
		pthread_mutex_lock(&data_mutex);
	}
}

void emu_exec_instr(void)
{
	if(pc >= ROM_SIZE - 1)
	{
		emu_return_cause = END_OF_ROM;
		pthread_mutex_lock(&data_mutex);
		emu_quit = true;
		pthread_mutex_unlock(&data_mutex);
	}

	if ( emu_state == EMU_CONTINUE )
		emu_state = EMU_BREAKPOINT;
	else if ( pc == emu_step_point || find_in_set(breakpoints, pc) )
	{
		emu_step_point = -1;
		emu_state = EMU_BREAKPOINT;
		pthread_mutex_lock(&data_mutex);
		while ( emu_state == EMU_BREAKPOINT )
			pthread_cond_wait(&breakpoint_cond, &data_mutex);
		pthread_mutex_unlock(&data_mutex);
		return;
	}
	if ( set_is_empty(breakpoints) )
		emu_state = EMU_RUNNING;

	pthread_mutex_lock(&data_mutex);
	byte opcode = rom[pc];

	const float timePeriodForInstruction =
		instructions[opcode].no_of_bytes * timePeriodPerMachineCycle;

	struct timespec initTime;
	clock_gettime(CLOCK_REALTIME, &initTime);

	bool tmp;
	byte tmpByte;
	word tmpWord;
	switch(opcode)
	{
		case 0x02: // LJMP code addr
			pc = ( rom[pc + 1] << 8 ) | rom[pc + 2];
			pc--;
			break;
		case 0xA5: // ???
			// ???
			break;
		case 0x12: // LCALL code addr
			tmpWord = pc + 3;
			ram[ ++SP ] = tmpWord & 0x0F;
			ram[ ++SP ] = tmpWord & 0xF0;
			pc = ( rom[ pc + 1 ] << 8 ) | rom[ pc + 2 ];
			pc--;
			break;
		case 0x11: // ACALL page0
			tmpWord = pc;
			pc += 2;
			ram[ ++SP ] = pc & 0x0F;
			ram[ ++SP ] = pc & 0xF0;
			pc = pc & 0b1111100000000000;
			pc |= rom[ tmpWord + 1 ];
			pc |= (0) << 8;
			pc--;
			break;
		case 0x31: // ACALL page1
			tmpWord = pc;
			pc += 2;
			ram[ ++SP ] = pc & 0x0F;
			ram[ ++SP ] = pc & 0xF0;
			pc = pc & 0b1111100000000000;
			pc |= rom[ tmpWord + 1 ];
			pc |= (1) << 8;
			pc--;
			break;
		case 0x51: // ACALL page2
			tmpWord = pc;
			pc += 2;
			ram[ ++SP ] = pc & 0x0F;
			ram[ ++SP ] = pc & 0xF0;
			pc = pc & 0b1111100000000000;
			pc |= rom[ tmpWord + 1 ];
			pc |= (2) << 8;
			pc--;
			break;
		case 0x71: // ACALL page3
			tmpWord = pc;
			pc += 2;
			ram[ ++SP ] = pc & 0x0F;
			ram[ ++SP ] = pc & 0xF0;
			pc = pc & 0b1111100000000000;
			pc |= rom[ tmpWord + 1 ];
			pc |= (3) << 8;
			pc--;
			break;
		case 0x91: // ACALL page4
			tmpWord = pc;
			pc += 2;
			ram[ ++SP ] = pc & 0x0F;
			ram[ ++SP ] = pc & 0xF0;
			pc = pc & 0b1111100000000000;
			pc |= rom[ tmpWord + 1 ];
			pc |= (4) << 8;
			pc--;
			break;
		case 0xB1: // ACALL page5
			tmpWord = pc;
			pc += 2;
			ram[ ++SP ] = pc & 0x0F;
			ram[ ++SP ] = pc & 0xF0;
			pc = pc & 0b1111100000000000;
			pc |= rom[ tmpWord + 1 ];
			pc |= (5) << 8;
			pc--;
			break;
		case 0xD1: // ACALL page6
			tmpWord = pc;
			pc += 2;
			ram[ ++SP ] = pc & 0x0F;
			ram[ ++SP ] = pc & 0xF0;
			pc = pc & 0b1111100000000000;
			pc |= rom[ tmpWord + 1 ];
			pc |= (6) << 8;
			pc--;
			break;
		case 0xF1: // ACALL page7
			tmpWord = pc;
			pc += 2;
			ram[ ++SP ] = pc & 0x0F;
			ram[ ++SP ] = pc & 0xF0;
			pc = pc & 0b1111100000000000;
			pc |= rom[ tmpWord + 1 ];
			pc |= (7) << 8;
			pc--;
			break;

		// AJMP
		case 0x01: // AJMP page0
			pc = ( pc & 0b1111100000000000) | ( 0b0000000000000000 ) | rom[ pc + 1 ];
			pc++;
			break;
		case 0x21: // AJMP page1
			pc = ( pc & 0b1111100000000000) | ( 0b0000000100000000 ) | rom[ pc + 1 ];
			pc++;
			break;
		case 0x41: // AJMP page2
			pc = ( pc & 0b1111100000000000) | ( 0b0000001000000000 ) | rom[ pc + 1 ];
			pc++;
			break;
		case 0x61: // AJMP page3
			pc = ( pc & 0b1111100000000000) | ( 0b0000001100000000 ) | rom[ pc + 1 ];
			pc++;
			break;
		case 0x81: // AJMP page4
			pc = ( pc & 0b1111100000000000) | ( 0b0000010000000000 ) | rom[ pc + 1 ];
			pc++;
			break;
		case 0xA1: // AJMP page5
			pc = ( pc & 0b1111100000000000) | ( 0b0000010100000000 ) | rom[ pc + 1 ];
			pc++;
			break;
		case 0xC1: // AJMP page6
			pc = ( pc & 0b1111100000000000) | ( 0b0000011000000000 ) | rom[ pc + 1 ];
			pc++;
			break;
		case 0xE1: // AJMP page7
			pc = ( pc & 0b1111100000000000) | ( 0b0000011100000000 ) | rom[ pc + 1 ];
			pc++;
			break;

		// INC
		case 0x04: // INC A	
			add_to_A(1);
			break;
		case 0x05: // INC iram addr	
			ram[rom[++pc]]++;
			break;
		case 0x06: // INC @R0	
			ram[R0]++;
			break;
		case 0x07: // INC @R1	
			ram[R0]++;
			break;
		case 0x08: // INC R0	
			R0++;
			break;
		case 0x09: // INC R1	
			R1++;
			break;
		case 0x0A: // INC R2	
			R2++;
			break;
		case 0x0B: // INC R3	
			R3++;
			break;
		case 0x0C: // INC R4	
			R4++;
			break;
		case 0x0D: // INC R5	
			R5++;
			break;
		case 0x0E: // INC R6	
			R6++;
			break;
		case 0x0F: // INC R7	
			R7++;
			break;
		case 0xA3: // INC DPTR	
			set_dptr( get_dptr() + 1 );
			break;
		case 0x24: // ADD A,#data
			add_to_A(rom[++pc]);
			break;

		// DEC
		case 0x14: // DEC A	
			sub_from_A(1);
			break;
		case 0x15: // DEC iram addr	
			ram[rom[++pc]]--;
			break;
		case 0x16: // DEC @R0	
			ram[R0]--;
			break;
		case 0x17: // DEC @R1	
			ram[R0]--;
			break;
		case 0x18: // DEC R0	
			R0--;
			break;
		case 0x19: // DEC R1	
			R1--;
			break;
		case 0x1A: // DEC R2	
			R2--;
			break;
		case 0x1B: // DEC R3	
			R3--;
			break;
		case 0x1C: // DEC R4	
			R4--;
			break;
		case 0x1D: // DEC R5	
			R5--;
			break;
		case 0x1E: // DEC R6	
			R6--;
			break;
		case 0x1F: // DEC R7	
			R7--;
			break;

		// ADD
		case 0x25: // ADD A,iram addr
			add_to_A(ram[rom[++pc]]);
			break;
		case 0x26: // ADD A,@R0
			add_to_A(ram[R0]);
			break;
		case 0x27: // ADD A,@R1
			add_to_A(ram[R1]);
			break;
		case 0x28: // ADD A,R0
			add_to_A(R0);
			break;
		case 0x29: // ADD A,R1
			add_to_A(R1);
			break;
		case 0x2A: // ADD A,R2
			add_to_A(R2);
			break;
		case 0x2B: // ADD A,R3
			add_to_A(R3);
			break;
		case 0x2C: // ADD A,R4
			add_to_A(R4);
			break;
		case 0x2D: // ADD A,R5
			add_to_A(R5);
			break;
		case 0x2E: // ADD A,R6
			add_to_A(R6);
			break;
		case 0x2F: // ADD A,R7
			add_to_A(R7);
			break;

		// ADDC
		case 0x34: // ADDC A,#data
			add_to_A(rom[++pc] + PSW_CY);
			break;
		case 0x35: // ADDC A,iram addr
			add_to_A(ram[rom[++pc]] + PSW_CY);
			break;
		case 0x36: // ADDC A,@R0
			add_to_A(ram[R0] + PSW_CY);
			break;
		case 0x37: // ADDC A,@R1
			add_to_A(ram[R1] + PSW_CY);
			break;
		case 0x38: // ADDC A,R0
			add_to_A(R0 + PSW_CY);
			break;
		case 0x39: // ADDC A,R1
			add_to_A(R1 + PSW_CY);
			break;
		case 0x3A: // ADDC A,R2
			add_to_A(R2 + PSW_CY);
			break;
		case 0x3B: // ADDC A,R3
			add_to_A(R3 + PSW_CY);
			break;
		case 0x3C: // ADDC A,R4
			add_to_A(R4 + PSW_CY);
			break;
		case 0x3D: // ADDC A,R5
			add_to_A(R5 + PSW_CY);
			break;
		case 0x3E: // ADDC A,R6
			add_to_A(R6 + PSW_CY);
			break;
		case 0x3F: // ADDC A,R7
			add_to_A(R7 + PSW_CY);
			break;
		
		// MOV
		case 0x76: // MOV @R0,#data	
			ram[R0] = rom[++pc];
			break;
		case 0x77: // MOV @R1,#data	
			ram[R1] = rom[++pc];
			break;
		case 0xF6: // MOV @R0,A	
			ram[R0] = a;
			break;
		case 0xF7: // MOV @R1,A	
			ram[R1] = a;
			break;
		case 0xA6: // MOV @R0,iram addr	
			ram[R0] = ram[rom[++pc]];
			break;
		case 0xA7: // MOV @R1,iram addr	
			ram[R1] = ram[rom[++pc]];
			break;
		case 0x74: // MOV A,#data	
			a = rom[++pc];
			break;
		case 0xE6: // MOV A,@R0	
			a = ram[R0];
			break;
		case 0xE7: // MOV A,@R1	
			a = ram[R1];
			break;
		case 0xE8: // MOV A,R0	
			a = R0;
			break;
		case 0xE9: // MOV A,R1	
			a = R1;
			break;
		case 0xEA: // MOV A,R2	
			a = R2;
			break;
		case 0xEB: // MOV A,R3	
			a = R3;
			break;
		case 0xEC: // MOV A,R4	
			a = R4;
			break;
		case 0xED: // MOV A,R5	
			a = R5;
			break;
		case 0xEE: // MOV A,R6	
			a = R6;
			break;
		case 0xEF: // MOV A,R7	
			a = R7;
			break;
		case 0xE5: // MOV A,iram addr	
			a = ram[rom[++pc]];
			break;
		case 0xA2: // MOV C,bit addr
			setPSW(PSW_CY_POS, getBit( rom[++pc] ));
			break;
		case 0x90: // MOV DPTR,#data16	
			set_dptr(( rom[pc + 1] << 8) | rom[pc + 2]);
			pc += 2;
			break;
		case 0x78: // MOV R0,#data	
			R0 = rom[++pc];
			break;
		case 0x79: // MOV R1,#data	
			R1 = rom[++pc];
			break;
		case 0x7A: // MOV R2,#data	
			R2 = rom[++pc];
			break;
		case 0x7B: // MOV R3,#data	
			R3 = rom[++pc];
			break;
		case 0x7C: // MOV R4,#data	
			R4 = rom[++pc];
			break;
		case 0x7D: // MOV R5,#data	
			R5 = rom[++pc];
			break;
		case 0x7E: // MOV R6,#data	
			R6 = rom[++pc];
			break;
		case 0x7F: // MOV R7,#data	
			R7 = rom[++pc];
			break;
		case 0xF8: // MOV R0,A	
			R0 = a;
			break;
		case 0xF9: // MOV R1,A	
			R1 = a;
			break;
		case 0xFA: // MOV R2,A	
			R2 = a;
			break;
		case 0xFB: // MOV R3,A	
			R3 = a;
			break;
		case 0xFC: // MOV R4,A	
			R4 = a;
			break;
		case 0xFD: // MOV R5,A	
			R5 = a;
			break;
		case 0xFE: // MOV R6,A	
			R6 = a;
			break;
		case 0xFF: // MOV R7,A	
			R7 = a;
			break;
		case 0xA8: // MOV R0,iram addr	
			R0 = ram[rom[++pc]];
			break;
		case 0xA9: // MOV R1,iram addr	
			R1 = ram[rom[++pc]];
			break;
		case 0xAA: // MOV R2,iram addr	
			R2 = ram[rom[++pc]];
			break;
		case 0xAB: // MOV R3,iram addr	
			R3 = ram[rom[++pc]];
			break;
		case 0xAC: // MOV R4,iram addr	
			R4 = ram[rom[++pc]];
			break;
		case 0xAD: // MOV R5,iram addr	
			R5 = ram[rom[++pc]];
			break;
		case 0xAE: // MOV R6,iram addr	
			R6 = ram[rom[++pc]];
			break;
		case 0xAF: // MOV R7,iram addr	
			R7 = ram[rom[++pc]];
			break;
		case 0x92: // MOV bit addr,C
			writeBit(PSW_CY, rom[++pc]);
			break;
		case 0x75: // MOV iram addr,#data
			ram[rom[pc + 1]] = rom[pc + 2];
			pc += 2;
			break;
		case 0x86: // MOV iram addr,@R0
			ram[rom[++pc]] = ram[R0];
			break;
		case 0x87: // MOV iram addr,@R1	
			ram[rom[++pc]] = ram[R1];
			break;
		case 0x88: // MOV iram addr,R0	
			ram[rom[++pc]] = R0;
			break;
		case 0x89: // MOV iram addr,R1	
			ram[rom[++pc]] = R1;
			break;
		case 0x8A: // MOV iram addr,R2	
			ram[rom[++pc]] = R2;
			break;
		case 0x8B: // MOV iram addr,R3	
			ram[rom[++pc]] = R3;
			break;
		case 0x8C: // MOV iram addr,R4	
			ram[rom[++pc]] = R4;
			break;
		case 0x8D: // MOV iram addr,R5	
			ram[rom[++pc]] = R5;
			break;
		case 0x8E: // MOV iram addr,R6	
			ram[rom[++pc]] = R6;
			break;
		case 0x8F: // MOV iram addr,R7	
			ram[rom[++pc]] = R7;
			break;
		case 0xF5: // MOV iram addr,A	
			ram[rom[++pc]] = a;
			break;
		case 0x85: // MOV iram addr,iram addr	
			ram[rom[pc + 1]] = ram[rom[pc + 2]];
			pc += 2;
			break;

		// ORL
		case 0x42: // ORL iram addr,A
			ram[rom[pc + 1]] = ram[rom[pc + 1]] | a;
			pc++;
			break;
		case 0x43: // ORL iram addr,#data
			ram[rom[pc + 1]] = ram[rom[pc + 1]] | rom[pc + 1];
			pc++;
			break;
		case 0x44: // ORL A,#data
			a = a | rom[++pc];
			break;
		case 0x45: // ORL A,iram addr
			a = a | ram[rom[++pc]];
			break;
		case 0x46: // ORL A,@R0
			a = a | ram[R0];
			break;
		case 0x47: // ORL A,@R1
			a = a | ram[R1];
			break;
		case 0x48: // ORL A,R0
			a = a | R0;
			break;
		case 0x49: // ORL A,R1
			a = a | R1;
			break;
		case 0x4A: // ORL A,R2
			a = a | R2;
			break;
		case 0x4B: // ORL A,R3
			a = a | R3;
			break;
		case 0x4C: // ORL A,R4
			a = a | R4;
			break;
		case 0x4D: // ORL A,R5
			a = a | R5;
			break;
		case 0x4E: // ORL A,R6
			a = a | R6;
			break;
		case 0x4F: // ORL A,R7
			a = a | R7;
			break;
		case 0x72: // ORL C,bit addr
			setPSW(PSW_CY_POS, PSW_CY | getBit( rom[++pc] ));
			break;
		case 0xA0: // ORL C,/bit addr
			setPSW(PSW_CY, getBit(rom[ ++pc ]) | PSW_CY);
			break;

		// AND
		case 0x52: // ANL iram addr,A
			ram[rom[pc + 1]] = ram[rom[pc + 1]] & a;
			pc++;
			break;
		case 0x53: // ANL iram addr,#data
			ram[rom[pc + 1]] = ram[rom[pc + 1]] | rom[pc + 1];
			pc++;
			break;
		case 0x54: // ANL A,#data
			a = a & rom[++pc];
			break;
		case 0x55: // ANL A,iram addr
			a = a & ram[rom[++pc]];
			break;
		case 0x56: // ANL A,@R0
			a = a & ram[R0];
			break;
		case 0x57: // ANL A,@R1
			a = a & ram[R1];
			break;
		case 0x58: // ANL A,R0
			a = a & R0;
			break;
		case 0x59: // ANL A,R1
			a = a & R1;
			break;
		case 0x5A: // ANL A,R2
			a = a & R2;
			break;
		case 0x5B: // ANL A,R3
			a = a & R3;
			break;
		case 0x5C: // ANL A,R4
			a = a & R4;
			break;
		case 0x5D: // ANL A,R5
			a = a & R5;
			break;
		case 0x5E: // ANL A,R6
			a = a & R6;
			break;
		case 0x5F: // ANL A,R7
			a = a & R7;
			break;
		case 0x82: // ANL C,bit addr
			setPSW(PSW_CY_POS, PSW_CY & getBit( rom[++pc] ));
			break;
		case 0xB0: // ANL C,/bit addr
			 setPSW(PSW_CY, getBit(rom[ ++pc ]) & PSW_CY);
			break;

		// XRL
		case 0x62: // XRL iram addr,A
			ram[rom[ pc + 1 ]] = ram[rom[ pc + 1 ]] ^ a;
			pc++;
			break;
		case 0x63: // XRL iram addr,#data
			ram[rom[ pc + 1 ]] = ram[rom[ pc + 1 ]] ^ rom[ pc + 1];
			pc++;
			break;
		case 0x64: // XRL A,#data
			a = a ^ rom[++pc];
			break;
		case 0x65: // XRL A,iram addr
			a = a ^ ram[rom[++pc]];
			break;
		case 0x66: // XRL A,@R0
			a = a ^ ram[R0];
			break;
		case 0x67: // XRL A,@R1
			a = a ^ ram[R1];
			break;
		case 0x68: // XRL A,R0
			a = a ^ R0;
			break;
		case 0x69: // XRL A,R1
			a = a ^ R1;
			break;
		case 0x6A: // XRL A,R2
			a = a ^ R2;
			break;
		case 0x6B: // XRL A,R3
			a = a ^ R3;
			break;
		case 0x6C: // XRL A,R4
			a = a ^ R4;
			break;
		case 0x6D: // XRL A,R5
			a = a ^ R5;
			break;
		case 0x6E: // XRL A,R6
			a = a ^ R6;
			break;
		case 0x6F: // XRL A,R7
			a = a ^ R7;
			break;

		// DJNZ
		case 0xD5: // DJNZ iram addr,reladdr
			pc = --ram[rom[++pc]] ? pc + (char)rom[pc + 1] : pc;
			pc++;
			break;
		case 0xD8: // DJNZ R0,reladdr
			pc = --R0 ? pc + (char)rom[pc + 1] : pc;
			pc++;
			break;
		case 0xD9: // DJNZ R1,reladdr
			pc = --R1 ? pc + (char)rom[pc + 1] : pc;
			pc++;
			break;
		case 0xDA: // DJNZ R2,reladdr
			pc = --R2 ? pc + (char)rom[pc + 1] : pc;
			pc++;
			break;
		case 0xDB: // DJNZ R3,reladdr
			pc = --R3 ? pc + (char)rom[pc + 1] : pc;
			pc++;
			break;
		case 0xDC: // DJNZ R4,reladdr
			pc = --R4 ? pc + (char)rom[pc + 1] : pc;
			pc++;
			break;
		case 0xDD: // DJNZ R5,reladdr
			pc = --R5 ? pc + (char)rom[pc + 1] : pc;
			pc++;
			break;
		case 0xDE: // DJNZ R6,reladdr
			pc = --R6 ? pc + (char)rom[pc + 1] : pc;
			pc++;
			break;
		case 0xDF: // DJNZ R7,reladdr
			pc = --R7 ? pc + (char)rom[pc + 1] : pc;
			pc++;
			break;

		// CJNE
		case 0xB4: // CJNE A,#data,reladdr
			pc = (a == rom[ pc + 1 ]) ? pc :  pc + (char)rom[ pc + 2 ];
			setPSW( PSW_CY_POS, ( a < rom[ pc + 1 ]));
			pc += 2;
			break;
		case 0xB5: // CJNE A,iram addr,reladdr
			pc = (a == ram[rom[ pc + 1 ]]) ? pc : pc + (char)rom[ pc + 2 ];
			setPSW( PSW_CY_POS, ( a < rom[ pc + 1 ]));
			pc += 2;
			break;
		case 0xB6: // CJNE @R0,#data,reladdr
			pc = (ram[R0] == rom[pc + 1]) ? pc :  pc + (char)rom[ pc + 2 ];
			setPSW( PSW_CY_POS, ( a < rom[ pc + 1 ]));
			pc += 2;
			break;
		case 0xB7: // CJNE @R1,#data,reladdr
			pc = (ram[R1] == rom[ pc + 1]) ? pc : instructions[0xB4].no_of_bytes + pc + (char)rom[ pc + 2 ];
			setPSW( PSW_CY_POS, ( a < rom[ pc + 1 ]));
			pc += 2;
			break;
		case 0xB8: // CJNE R0,#data,reladdr
			pc = (R0 == rom[pc + 1]) ? pc : instructions[0xB4].no_of_bytes + pc + (char)rom[ pc + 2 ];
			setPSW( PSW_CY_POS, ( a < rom[ pc + 1 ]));
			pc += 2;
			break;
		case 0xB9: // CJNE R1,#data,reladdr
			pc = (R1 == rom[pc + 1]) ? pc : instructions[0xB4].no_of_bytes + pc + (char)rom[ pc + 2 ];
			setPSW( PSW_CY_POS, ( a < rom[ pc + 1 ]));
			pc += 2;
			break;
		case 0xBA: // CJNE R2,#data,reladdr
			pc = (R2 == rom[pc + 1]) ? pc : instructions[0xB4].no_of_bytes + pc + (char)rom[ pc + 2 ];
			setPSW( PSW_CY_POS, ( a < rom[ pc + 1 ]));
			pc += 2;
			break;
		case 0xBB: // CJNE R3,#data,reladdr
			pc = (R3 == rom[pc + 1]) ? pc : instructions[0xB4].no_of_bytes + pc + (char)rom[ pc + 2 ];
			setPSW( PSW_CY_POS, ( a < rom[ pc + 1 ]));
			pc += 2;
			break;
		case 0xBC: // CJNE R4,#data,reladdr
			pc = (R4 == rom[pc + 1]) ? pc : instructions[0xB4].no_of_bytes + pc + (char)rom[ pc + 2 ];
			setPSW( PSW_CY_POS, ( a < rom[ pc + 1 ]));
			pc += 2;
			break;
		case 0xBD: // CJNE R5,#data,reladdr
			pc = (R5 == rom[pc + 1]) ? pc : instructions[0xB4].no_of_bytes + pc + (char)rom[ pc + 2 ];
			setPSW( PSW_CY_POS, ( a < rom[ pc + 1 ]));
			pc += 2;
			break;
		case 0xBE: // CJNE R6,#data,reladdr
			pc = (R6 == rom[pc + 1]) ? pc : instructions[0xB4].no_of_bytes + pc + (char)rom[ pc + 2 ];
			setPSW( PSW_CY_POS, ( a < rom[ pc + 1 ]));
			pc += 2;
			break;
		case 0xBF: // CJNE R7,#data,reladdr
			pc = (R7 == rom[pc + 1]) ? pc : instructions[0xB4].no_of_bytes + pc + (char)rom[ pc + 2 ];
			setPSW( PSW_CY_POS, ( a < rom[ pc + 1 ]));
			pc += 2;
			break;
		
		case 0x40: // JC reladdr
			if (PSW_CY)
				pc = pc + (char)rom[ pc + 1 ] + 1;
			else
				pc++;
			break;
		case 0x50: // JNC reladdr
			if (!PSW_CY)
				pc = pc + (char)rom[ pc + 1 ] + 1;
			else
				pc++;
			break;
		case 0x60: // JZ reladdr
			if (!a)
				pc = pc + (char)rom[ pc + 1 ] + 1;
			else
				pc++;
			break;
		case 0x70: // JNZ reladdr
			if (a)
				pc = pc + (char)rom[ pc + 1 ] + 1;
			else
				pc++;
			break;
		case 0x80: // SJMP reladdr
			pc = pc + (char)rom[ pc + 1 ] + instructions[0x80].no_of_bytes - 1;
			break;
		// RR A
		case 0x03:
			a = a >> 1 | (a & 0b00000001) << 7;
			break;
		// RL A
		case 0x23:
			a = a << 1 | (a & 0b10000000) >> 7;
			break;
		// RRC A
		case 0x13:
			tmp = a & 0b00000001;
			a = a >> 1 | ( PSW_CY << 7);
			setPSW(PSW_CY_POS, tmp);
			break;
		// RLC A
		case 0x33:
			tmp = (a & 0b10000000) >> 7;
			a = a << 1 |  (byte)PSW_CY;
			setPSW(PSW_CY_POS, tmp);
			break;
		
		// XCH
		case 0xC6: // XCH A,@R0
			tmpByte = a;
			a = ram[R0];
			ram[R0] = tmpByte;
			break;
		case 0xC7: // XCH A,@R1
			tmpByte = a;
			a = ram[R1];
			ram[R1] = tmpByte;
			break;
		case 0xC8: // XCH A,R0
			tmpByte = a;
			a = R0;
			R0 = tmpByte;
			break;
		case 0xC9: // XCH A,R1
			tmpByte = a;
			a = R1;
			R1 = tmpByte;
			break;
		case 0xCA: // XCH A,R2
			tmpByte = a;
			a = R2;
			R2 = tmpByte;
			break;
		case 0xCB: // XCH A,R3
			tmpByte = a;
			a = R3;
			R3 = tmpByte;
			break;
		case 0xCC: // XCH A,R4
			tmpByte = a;
			a = R4;
			R4 = tmpByte;
			break;
		case 0xCD: // XCH A,R5
			tmpByte = a;
			a = R5;
			R5 = tmpByte;
			break;
		case 0xCE: // XCH A,R6
			tmpByte = a;
			a = R6;
			R6 = tmpByte;
			break;
		case 0xCF: // XCH A,R7
			tmpByte = a;
			a = R7;
			R7 = tmpByte;
			break;
		case 0xC5: // XCH A,iram addr
			tmpByte = a;
			a = ram[ rom[ pc + 1]];
			rom[ rom[ pc + 1 ]] = tmpByte;
			pc++;
			break;

		// XCHD
		case 0xD6: // XCHD A,@R0
			tmpByte = a & 0b00001111;
			a = a & 0b11110000 | ram[R0] & 0b00001111;
			ram[R0] = ram[R0] & 0b11110000 | tmpByte & 0b00001111;
			break;
		case 0xD7: // XCHD A,@R1
			tmpByte = a & 0b00001111;
			a = a & 0b11110000 | ram[R1] & 0b00001111;
			ram[R1] = ram[R1] & 0b11110000 | tmpByte & 0b00001111;
			break;

		// SWAP
		case 0xC4: // SWAP A
		    tmpByte = a & 0b00001111;
		    a = a >> 4 | tmpByte << 4;
		    break;

		// DA
		case 0xD4: // DA
			if ( (a & 0x0F) > 9 )
				add_to_A(0x06);
			if ( (((a & 0xF0) >> 4) > 9) | PSW_CY )
				add_to_A(0x60);
			break;

		// PUSH
		case 0xC0: // PUSH iram addr
			ram[ ++SP ] = ram[rom[ ++pc ]];
			break;
		// POP
		case 0xD0: // POP iram addr
			ram[rom[ ++pc ]] = ram[ SP-- ];
			break;

		// CPL
		case 0xF4: // CPL A
			a = !a;
			break;
		case 0xB3: // CPL C
			setPSW(PSW_CY_POS, !PSW_CY);
			break;
		case 0xB2: // CPL bit addr
			writeBit( !getBit( rom[ pc + 1 ] ), rom[ pc + 1 ] );
			pc++;
			break;

		// CLR
		case 0xC2: // CLR bit addr
			writeBit( 0, rom[ ++pc ] );
			break;
		case 0xC3: // CLR C
			setPSW(PSW_CY_POS, 0);
			break;
		case 0xE4: // CLR A
			a = 0;
			break;

		// SETB
		case 0xD3: // SETB C
			setPSW(PSW_CY_POS, 1);
			break;
		case 0xD2: // SETB bit addr
			writeBit( 1, rom[ ++pc ] );
			break;

		// DIV
		case 0x84: // DIV AB
			tmpByte = a / b;
			b = a % b;
			a = tmpByte;
			break;

		case 0xA4: // MUL AB
			tmpWord = a * b;
			a = tmpWord & 0x00FF;
			b = (tmpWord & 0xFF00) >> 8;
			break;

		case 0x73: // JMP @A+DPTR
			pc = a + get_dptr() - 1;
			break;

		// SUBB
		case 0x94: // SUBB A,#data
			sub_from_A( rom[ ++pc ] );
			break;
		case 0x95: // SUBB A,iram addr
			sub_from_A( ram[rom[ ++pc ]] );
			break;
		case 0x96: // SUBB A,@R0
			sub_from_A( ram[R0] );
			break;
		case 0x97: // SUBB A,@R1
			sub_from_A( ram[R1] );
			break;
		case 0x98: // SUBB A,R0
			sub_from_A( R0 );
			break;
		case 0x99: // SUBB A,R1
			sub_from_A( R1 );
			break;
		case 0x9A: // SUBB A,R2
			sub_from_A( R2 );
			break;
		case 0x9B: // SUBB A,R3
			sub_from_A( R3 );
			break;
		case 0x9C: // SUBB A,R4
			sub_from_A( R4 );
			break;
		case 0x9D: // SUBB A,R5
			sub_from_A( R5 );
			break;
		case 0x9E: // SUBB A,R6
			sub_from_A( R6 );
			break;
		case 0x9F: // SUBB A,R7
			sub_from_A( R7 );
			break;

		// JBC
		case 0x10: // JBC bit addr,reladdr
			tmpWord = getBit(rom[pc + 1])? pc + (char)rom[ pc + 2 ] - 1 : pc + 2;
			writeBit(0, rom[ pc + 1]);
			pc = tmpWord;
			break;

		// JB
		case 0x20: // JB bit addr,reladdr
			pc = getBit(rom[pc + 1])?
				pc + (char)rom[ pc + 2 ] + instructions[0x20].no_of_bytes - 1 :
				pc + 2;
			break;

		// JNB
		case 0x30: // JNB bit addr,reladdr
			pc = (!getBit(rom[pc + 1]))?
				pc + (char)rom[ pc + 2 ] + instructions[0x30].no_of_bytes - 1:
				pc + 2;
			break;

		// MOVC
		case 0x93: // MOVC A,@A+DPTR
			a = rom[a + get_dptr()];
			break;
		case 0x83: // MOVC A,@A+PC
			a = rom[a + pc];
			break;
		
		// RET
		case 0x22: // RET
			pc = (ram[ SP ] << 8) | ram[ SP - 1 ];
			SP -= 2;
			pc--;
			break;
		case 0x00: // NOP
			break;
			default:
				fprintf(stderr, "unknown opcode at pc = %0X  opcode = %0X\n", pc, opcode);
				break;
	}
	pc += 1;
	// Manage Timers
	manage_timers();
	pthread_mutex_unlock(&data_mutex);

	struct timespec nowTime;
	clock_gettime(CLOCK_REALTIME, &nowTime);

	unsigned int timeElapsed;
	if ( (nowTime.tv_sec - initTime.tv_sec) == 0)
		timeElapsed = nowTime.tv_nsec - initTime.tv_nsec;
	else
		timeElapsed = 10E9 - initTime.tv_nsec + nowTime.tv_nsec;

	usleep( timePeriodForInstruction - timeElapsed / 1000.0f );
}
