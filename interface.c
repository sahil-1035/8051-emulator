#include <stdio.h>

#include "emulator.h"

void printROM()
{
	const unsigned int ROM_WIDTH = 32;
	printf("ROM - \n");
	for (unsigned int i = 0; i < ROM_FILE_LEN/ROM_WIDTH; i++)
	{
		for (unsigned int j = 0; j < ROM_WIDTH; j++)
		{
			printf("%02X ", rom[ROM_WIDTH * i + j]);
		}
		printf("\n");
	}
}

void printRAM()
{
	const unsigned int RAM_WIDTH = 16;
	printf("RAM - \n");
	for (unsigned int i = 0; i < RAM_SIZE/RAM_WIDTH; i++)
	{
		for (unsigned int j = 0; j < RAM_WIDTH; j++)
		{
			printf("%02X ", ram[RAM_WIDTH * i + j]);
		}
		printf("\n");
	}
}

void interface_main()
{
	emu_load_ROM("p2");
	change_bank(0);
	emu_clear_ram();
	pc = 0x03ac;
	while(true)
	{
		if(pc == ROM_SIZE - 1)
		{
			fprintf(stderr, "end of ROM reached pc=%0X\n", pc);
			return;
		}
		if(rom[pc] == 0x6f && rom[pc + 1] == 0x22)
			return;
		printROM();
		printRAM();
		emu_exec_instr();
		char inp;
		printf("%s\n", get_instruction[rom[pc]]);
		/* getchar(); */
	}
}
