#include <stdio.h>

#define RAM_SIZE 128
#define ROM_SIZE 4096

typedef char byte;
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
byte *R0, *R1, *R2, *R3, *R4, *R5, *R6, *R7, *R8;

void add_to_A(byte addend);
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
	a = 0x01;
	while(1)
	{
		if(pc == ROM_SIZE - 1)
		{
			fprintf(stderr, "end of ROM reached pc=%0X\n", pc);
			return;
		}
		byte opcode = rom[pc];
		switch(opcode)
		{
			case 0x24: // ADD A,#data
				add_to_A(rom[++pc]);
				break;
			case 0x25: // ADD A,iram addr
				break;
			case 0x26: // ADD A,@R0
				add_to_A(ram[*R0]);
				break;
			case 0x27: // ADD A,@R1
				add_to_A(ram[*R1]);
				break;
			case 0x28: // ADD A,R0
				add_to_A(*R0);
				break;
			case 0x29: // ADD A,R1
				add_to_A(*R1);
				break;
			case 0x2A: // ADD A,R2
				add_to_A(*R2);
				break;
			case 0x2B: // ADD A,R3
				add_to_A(*R3);
				break;
			case 0x2C: // ADD A,R4
				add_to_A(*R4);
				break;
			case 0x2D: // ADD A,R5
				add_to_A(*R5);
				break;
			case 0x2E: // ADD A,R6
				add_to_A(*R6);
				break;
			case 0x2F: // ADD A,R7
				add_to_A(*R7);
				break;
			case 0x34: // ADDC A,#data
				break;
			case 0x35: // ADDC A,iram addr
				break;
			case 0x36: // ADDC A,@R0
				break;
			case 0x37: // ADDC A,@R1
				break;
			case 0x38: // ADDC A,R0
				break;
			case 0x39: // ADDC A,R1
				break;
			case 0x3A: // ADDC A,R2
				break;
			case 0x3B: // ADDC A,R3
				break;
			case 0x3C: // ADDC A,R4
				break;
			case 0x3D: // ADDC A,R5
				break;
			case 0x3E: // ADDC A,R6
				break;
			case 0x3F: // ADDC A,R7
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
	printf("A = %0X", a);
}


void add_to_A(byte addend)
{
	if( a > a + addend)
		psw = psw | 0b00000100; // setting OV bit
}

void change_bank(short int new_bank_val)
{
	register_bank = new_bank_val;
	R0 = 8 * register_bank * sizeof(byte) + ram;
	R1 = 8 * register_bank * sizeof(byte) + ram + 1 * sizeof(byte);
	R2 = 8 * register_bank * sizeof(byte) + ram + 2 * sizeof(byte);
	R3 = 8 * register_bank * sizeof(byte) + ram + 3 * sizeof(byte);
	R4 = 8 * register_bank * sizeof(byte) + ram + 4 * sizeof(byte);
	R5 = 8 * register_bank * sizeof(byte) + ram + 5 * sizeof(byte);
	R6 = 8 * register_bank * sizeof(byte) + ram + 6 * sizeof(byte);
	R7 = 8 * register_bank * sizeof(byte) + ram + 7 * sizeof(byte);
	R8 = 8 * register_bank * sizeof(byte) + ram + 8 * sizeof(byte);
}
