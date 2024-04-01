#include <stdio.h>
#include <ncurses.h>
#include <pthread.h>
#include <unistd.h>

#include "interface.h"

#include "definitions.h"
#include "emulator.h"

Window ROM_win;
Window RAM_win;
Window MISC_win;

bool interface_quit;

void interface_main(void)
{
	emu_init("num1.bin");
	init_curses();

	pthread_t emulator_thread;
	pthread_create(&emulator_thread, NULL, (void* (*)(void*))emu_start, NULL);
	
	while ( !emu_quit )
	{
		print_curses();
		pthread_mutex_unlock(&data_mutex);

		/* manage_input(); */
		usleep(1000);
		pthread_mutex_lock(&data_mutex);

		/* pthread_mutex_unlock(&data_mutex); */
		/* emu_exec_instr(); */
		/* pthread_mutex_lock(&data_mutex); */
	}
	void* return_val;
	pthread_join(emulator_thread, return_val);

	if ( emu_return_cause == END_OF_ROM )
	{
		printend("End of ROM reached. -- Enter any key to exit --");
		getch();
	}
	endwin();
}


void init_curses(void)
{
	initscr();
	cbreak();
	keypad(stdscr, TRUE);
	refresh();

	int height, width;
	getmaxyx(stdscr, height, width);
	create_window(&ROM_win, height - 2, 105, 0, 0);
	create_window(&RAM_win, 18, width - 106 - 11, 0, 105);
	create_window(&MISC_win, 18, width - 106 - 11, 18, 105);
}

void print_curses(void)
{
	printRAM();
	printROM();
	printMISC();
}

void manage_input(void)
{
	int inp = getch();
	if ( inp == KEY_F(1) )
		interface_quit = true;
}



void printMISC(void)
{
	werase(MISC_win.win);
	box(MISC_win.win, 0, 0);
	MISC_win.cur_x = 2;
	MISC_win.cur_y = 1;

	PRINT_IN_WIN(&MISC_win, 0, "PC = %04XH;  ", pc);
	PRINT_IN_WIN(&MISC_win, 1, "// %s", instructions[rom[pc]].string);
	PRINT_IN_WIN(&MISC_win, 0, "A = %02XH;  ", a);
	PRINT_IN_WIN(&MISC_win, 0, "B = %02XH;  ", b);
	PRINT_IN_WIN(&MISC_win, 0, "SP = %02XH;  ", ram[0x81]);
	PRINT_IN_WIN(&MISC_win, 0, "DPTR = %04XH;", dptr);
	wrefresh(MISC_win.win);
}

void printROM(void)
{
	const unsigned int ROM_WIDTH = 32;

	box(ROM_win.win, 0 , 0);
	mvwprintw(ROM_win.win, 0, 3, " ROM ");

	start_color();
	init_pair(1, COLOR_BLACK, COLOR_WHITE);
	init_pair(2, COLOR_BLACK, COLOR_BLUE);

	ROM_win.cur_x = 2;
	ROM_win.cur_y = 1;

	unsigned int romptr = 0;
	for (unsigned int i = 0; i < 1 + (ROM_FILE_LEN / ROM_WIDTH); i++)
	{
		// For printing the ROM address at the beginning
		PRINT_IN_WIN(&ROM_win, 0, "%04X: ", ROM_WIDTH * i);

		for (unsigned int j = 0; j < ROM_WIDTH; j++)
		{
			if (romptr > ROM_FILE_LEN)
				break;

			romptr = ROM_WIDTH * i + j;
			if (romptr == pc)
				wattron(ROM_win.win,COLOR_PAIR(1));
			if (romptr > pc && romptr < pc + instructions[rom[pc]].no_of_bytes)
				wattron(ROM_win.win,COLOR_PAIR(2));

			PRINT_IN_WIN(&ROM_win, 0, "%02X ", rom[romptr]);

			wattroff(ROM_win.win,COLOR_PAIR(1));
			wattroff(ROM_win.win,COLOR_PAIR(2));
		}
		PRINT_IN_WIN(&ROM_win, 1, "");
	}
	wrefresh(ROM_win.win);
}

void printRAM(void)
{
	const unsigned int RAM_WIDTH = 16;

	box(RAM_win.win, 0 , 0);
	mvwprintw(RAM_win.win, 0, 3, " RAM ");

	RAM_win.cur_x = 2;
	RAM_win.cur_y = 1;

	unsigned int ramptr = 0;
	for (unsigned int i = 0; i < (RAM_SIZE / RAM_WIDTH); i++)
	{
		// For printing the RAM address at the beginning
		PRINT_IN_WIN(&RAM_win, 0, "%04X: ", RAM_WIDTH * i);
		for (unsigned int j = 0; j < RAM_WIDTH; j++)
		{
			if (ramptr > RAM_SIZE)
				break;
			ramptr = RAM_WIDTH * i + j;
			PRINT_IN_WIN(&RAM_win, 0, "%02X ", ram[ramptr]);
		}
		PRINT_IN_WIN(&RAM_win, 1, "");
	}
	wrefresh(RAM_win.win);
}

