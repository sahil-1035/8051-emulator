#include <stdio.h>
#include <ncurses.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#include "interface.h"

#include "definitions.h"
#include "emulator.h"
#include "window.h"

Window *ROM_win, *RAM_win, *MISC_win, *POPUP_win;

bool interface_quit;

void interface_main(void)
{
	emu_init("num1.bin");
	init_curses();

	pthread_t emulator_thread;
	pthread_create(&emulator_thread, NULL, (void* (*)(void*))emu_start, NULL);

	printPOPUP();

	pthread_t input_thread;
	pthread_create(&input_thread, NULL, (void* (*)(void*))manage_input, NULL);

	while ( !emu_quit )
	{
		print_curses();
		pthread_mutex_unlock(&data_mutex);

		usleep(10000);
		pthread_mutex_lock(&data_mutex);

		pthread_mutex_unlock(&data_mutex);
		emu_exec_instr();
		pthread_mutex_lock(&data_mutex);
	}
	void* return_val = NULL;
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
	nodelay(stdscr, TRUE);
	refresh();

	int height, width;
	getmaxyx(stdscr, height, width);
	ROM_win = malloc(sizeof(Window));
	RAM_win = malloc(sizeof(Window));
	MISC_win = malloc(sizeof(Window));
	POPUP_win = malloc(sizeof(Window));
	create_window(ROM_win, " ROM ", height - 1, 50, 0, 0);
	create_window(RAM_win, " RAM ", 18, width - 106 - 11, 0, 52);
	create_window(MISC_win, " MISC ", height - 1 - 18, width - 106 - 11, 18, 52);
	create_window(POPUP_win, "", 3, 15, height - 3, width - 15);
}

void print_curses(void)
{
	printRAM();
	printROM();
	printMISC();
	printPOPUP();
}

void manage_input(void)
{
	while ( 1 )
	{
		if ( get_window_input(POPUP_win) )
		{
			set_window_title(POPUP_win, get_window_input_str(POPUP_win));
			clear_window_input_buffer(POPUP_win);
			printPOPUP();
		}
	}
	return;
	int inp = getch();
	if ( inp == ERR )
		return;
	else if ( inp == 'q' )
		interface_quit = true;
}

void printPOPUP(void)
{
	clear_window(POPUP_win);
	print_to_window(POPUP_win, 0, "> ");
	print_to_window(POPUP_win, 0, "%s", get_window_input_str(POPUP_win));
	refresh_window(POPUP_win);
}


void printMISC(void)
{
	clear_window(MISC_win);
	print_to_window(MISC_win, 0, "PC = %04XH;  ", pc);
	print_to_window(MISC_win, 1, "// %s", instructions[rom[pc]].string);
	print_to_window(MISC_win, 0, "A = %02XH;  ", a);
	print_to_window(MISC_win, 0, "B = %02XH;  ", b);
	print_to_window(MISC_win, 0, "SP = %02XH;  ", ram[0x81]);
	print_to_window(MISC_win, 0, "DPTR = %04XH;", dptr);
	refresh_window(MISC_win);
}

void printROM(void)
{
	clear_window(ROM_win);
	start_color();
	init_pair(1, COLOR_BLACK, COLOR_WHITE);
	init_pair(2, COLOR_BLACK, COLOR_BLUE);

	int instr_pos = 0;
	bool curr_instr = false;
	for (unsigned int i = 0; i < ROM_FILE_LEN; i++)
	{
		instr_pos = i;
		// For printing the ROM address at the beginning
		print_to_window(ROM_win, 0, "%04X: ", i);

		if (i == pc)
			curr_instr = true;
		else
			curr_instr = false;

		if (curr_instr)
			wattron(ROM_win->win, COLOR_PAIR(1));

		print_to_window(ROM_win, 0, "%02X ", rom[i]);

		wattroff(ROM_win->win, COLOR_PAIR(1));
		wattroff(ROM_win->win, COLOR_PAIR(2));

		if (curr_instr)
			wattron(ROM_win->win, COLOR_PAIR(2));

		for (unsigned int j = 0; j < instructions[rom[instr_pos]].no_of_bytes - 1; j++)
			print_to_window(ROM_win, 0, "%02X ", rom[++i]);

		wattroff(ROM_win->win, COLOR_PAIR(1));
		wattroff(ROM_win->win, COLOR_PAIR(2));

		set_window_cursor(ROM_win, 20, ROM_win->cur_y);
		print_to_window(ROM_win, 1, "; %s", instructions[rom[instr_pos]].string);
	}
	refresh_window(ROM_win);
}

void printRAM(void)
{
	const unsigned int RAM_WIDTH = 16;
	clear_window(RAM_win);

	unsigned int ramptr = 0;
	for (unsigned int i = 0; i < (RAM_SIZE / RAM_WIDTH); i++)
	{
		// For printing the RAM address at the beginning
		print_to_window(RAM_win, 0, "%04X: ", RAM_WIDTH * i);
		for (unsigned int j = 0; j < RAM_WIDTH; j++)
		{
			if (ramptr > RAM_SIZE)
				break;
			ramptr = RAM_WIDTH * i + j;
			print_to_window(RAM_win, 0, "%02X ", ram[ramptr]);
		}
		print_to_window(RAM_win, 1, "");
	}
	refresh_window(RAM_win);
}
