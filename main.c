#include <stdio.h>
#include <stdbool.h>

#define RAM_SIZE 128
#define ROM_SIZE 4096

#define PSW_CY getPSW(7)
#define PSW_AC getPSW(6)
#define PSW_F0 getPSW(5)
#define PSW_RS1 getPSW(4)
#define PSW_RS0 getPSW(3)
#define PSW_OV getPSW(2)
//#define PSW_CY getPSW(1)
#define PSW_P getPSW(0)

#define R0 (*R0p)
#define R1 (*R1p)
#define R2 (*R2p)
#define R3 (*R3p)
#define R4 (*R4p)
#define R5 (*R5p)
#define R6 (*R6p)
#define R7 (*R7p)

typedef unsigned char byte;
typedef short int word;

byte rom[ROM_SIZE];
byte ram[RAM_SIZE];

word pc = 0x0000;
word dptr = 0x0000;

byte sp = 0x07;
byte a = 0x00;
byte b = 0x00;
byte psw = 0x00;

short int register_bank = 0;
byte *R0p, *R1p, *R2p, *R3p, *R4p, *R5p, *R6p, *R7p, *R8p;

bool getPSW(short int pos);
void setPSW(short int pos, bool val);
void add_to_A(byte addend);
void sub_from_A(byte subtrahend);
void change_bank(short int new_bank_val);

void clear_ram()
{
	pc = 0x0000;
	dptr = 0x0000;
	sp = 0x00;
	a = 0x00;
	b = 0x00;
	register_bank = 0;

	for(int i = 0; i < RAM_SIZE; i++)
	{
		ram[i] = 0x000;
	}
}

void start_emu()
{
	rom[0] = 0x24;
	rom[1] = 0x68;
	rom[2] = 0x08;
	ram[0] = 0x41;
	a = 0x01;
	while(true)
	{
		if(pc == 968)//ROM_SIZE - 1)
		{
			fprintf(stderr, "end of ROM reached pc=%0X\n", pc);
			return;
		}
		if(rom[pc - 2] == 0x74 && rom[pc - 1] == 0x69)
			return;
		byte opcode = rom[pc];
		switch(opcode)
		{
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
				dptr++;
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
				add_to_A(rom[++pc]);
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
				case 0xA2: // MOV C,bit addr	// TODO
					setPSW(7, ram[rom[89]]);
					break;
				case 0x90: // MOV DPTR,#data16	
					dptr = (pc + 1) << 8 | (pc + 2);
					pc += 2;
					break;
				case 0x78: // MOV R0,#data	
					break;
				case 0x79: // MOV R1,#data	
					break;
				case 0x7A: // MOV R2,#data	
					break;
				case 0x7B: // MOV R3,#data	
					break;
				case 0x7C: // MOV R4,#data	
					break;
				case 0x7D: // MOV R5,#data	
					break;
				case 0x7E: // MOV R6,#data	
					break;
				case 0x7F: // MOV R7,#data	
					break;
				case 0xF8: // MOV R0,A	
					break;
				case 0xF9: // MOV R1,A	
					break;
				case 0xFA: // MOV R2,A	
					break;
				case 0xFB: // MOV R3,A	
					break;
				case 0xFC: // MOV R4,A	
					break;
				case 0xFD: // MOV R5,A	
					break;
				case 0xFE: // MOV R6,A	
					break;
				case 0xFF: // MOV R7,A	
					break;
				case 0xA8: // MOV R0,iram addr	
					break;
				case 0xA9: // MOV R1,iram addr	
					break;
				case 0xAA: // MOV R2,iram addr	
					break;
				case 0xAB: // MOV R3,iram addr	
					break;
				case 0xAC: // MOV R4,iram addr	
					break;
				case 0xAD: // MOV R5,iram addr	
					break;
				case 0xAE: // MOV R6,iram addr	
					break;
				case 0xAF: // MOV R7,iram addr	
					break;
				case 0x92: // MOV bit addr,C	
					break;
				case 0x75: // MOV iram addr,#data	
					break;
				case 0x86: // MOV iram addr,@R0	
					break;
				case 0x87: // MOV iram addr,@R1	
					break;
				case 0x88: // MOV iram addr,R0	
					break;
				case 0x89: // MOV iram addr,R1	
					break;
				case 0x8A: // MOV iram addr,R2	
					break;
				case 0x8B: // MOV iram addr,R3	
					break;
				case 0x8C: // MOV iram addr,R4	
					break;
				case 0x8D: // MOV iram addr,R5	
					break;
				case 0x8E: // MOV iram addr,R6	
					break;
				case 0x8F: // MOV iram addr,R7	
					break;
				case 0xF5: // MOV iram addr,A	
					break;
				case 0x85: // MOV iram addr,iram addr	
					break;

			case 0x00: // NOP
				break;
			default:
				fprintf(stderr, "unknown opcode at pc = %0X  opcode = %0X\n", pc, opcode);
				break;
		}
		pc += 1;
	}
}

int main(void)
{
	change_bank(0);
	clear_ram();
	start_emu();
	printf("A = %X\n", a);
	printf("R0 = %X\n", R0);
}


void add_to_A(byte addend)
{
	if( a > a + addend)
		setPSW(2, 1);
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
	psw = val? (psw | (1 << pos)) : psw & !(1 << pos);
}
