#include <stdio.h>
#include <ncurses.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "interface.h"

#include "definitions.h"
#include "emulator.h"
#include "utils/set.h"
#include "window.h"

typedef enum Commands
{
	CMD_BREAKPOINT, CMD_CONTINUE, CMD_HELP, CMD_MOVE, CMD_NEXT, CMD_SET
} Commands;

Window *ROM_win, *RAM_win, *MISC_win, *CMD_win, *HELP_win, *PORT_win;

bool enable_help;
bool interface_quit;

void interface_main(void)
{
	emu_init("add.bin");
	init_curses();
	insert_set(breakpoints, 0);

	pthread_t emulator_thread;
	pthread_create(&emulator_thread, NULL, (void* (*)(void*))emu_start, NULL);

	printCMD();

	while ( !emu_quit )
	{
		print_curses();
		pthread_mutex_unlock(&data_mutex);

		manage_input();
		usleep(10000);
		pthread_mutex_lock(&data_mutex);

		/* pthread_mutex_unlock(&data_mutex); */
		/* emu_exec_instr(); */
		/* pthread_mutex_lock(&data_mutex); */
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
	CMD_win = malloc(sizeof(Window));
	HELP_win = malloc(sizeof(Window));
	PORT_win = malloc(sizeof(Window));
	create_window(ROM_win, " ROM ", height - 1, 50, 0, 0);
	create_window(RAM_win, " RAM ", 18, width - 106 - 11, 0, 52);
	create_window(MISC_win, " MISC ", height - 1 - 18, width - 106 - 11, 18, 52);
	create_window(CMD_win, " COMMAND ", 3, 40, height - 4, width - 64);
	create_window(PORT_win, " PORTS ", 6, 16, 0, 110);
	int HELP_win_x = 74;
	int HELP_win_y = 30;
	create_window(HELP_win, " HELP ", HELP_win_y, HELP_win_x, \
			height / 2 - HELP_win_y / 2, width / 2 - HELP_win_x / 2);
}

void print_curses(void)
{
	if ( enable_help )
	{
		printHELP();
		clear();
	}
	else
	{
		printRAM();
		printROM();
		printMISC();
		printPORTS();
		printCMD();
	}
}

bool check_command_value(char* command, Commands command_val)
{
	switch (command_val)
	{
	case CMD_SET:
		return (strcmp(command, "set") == 0) ||
			(strcmp(command, "s") == 0);
			break;
	case CMD_MOVE:
		return (strcmp(command, "move") == 0) ||
			(strcmp(command, "m") == 0);
			break;
	case CMD_NEXT:
		return (strcmp(command, "next") == 0) ||
			(strcmp(command, "n") == 0);
			break;
	case CMD_HELP:
		return (strcmp(command, "help") == 0) ||
			(strcmp(command, "?") == 0) ||
			(strcmp(command, "h") == 0);
		break;
	case CMD_BREAKPOINT:
		return (strcmp(command, "breakpoint") == 0) ||
			(strcmp(command, "b") == 0);
		break;
	case CMD_CONTINUE:
		return (strcmp(command, "continue") == 0) ||
			(strcmp(command, "c") == 0);
		break;
	default:
		enable_help = true;
		break;
	}
	return false;
}

void manage_input(void)
{
	if ( get_window_input(CMD_win) )
	{
		char* input_str = get_window_input_str(CMD_win);

		char command[30];
		sscanf(input_str, "%s", command);
		if ( check_command_value(command, CMD_BREAKPOINT) )
		{
			int breakpoint_val;
			sscanf(input_str, "%s %x", command, &breakpoint_val);
			pthread_mutex_lock(&data_mutex);
			if (find_in_set(breakpoints, breakpoint_val))
				erase_set(breakpoints, breakpoint_val);
			else
				insert_set(breakpoints, breakpoint_val);
			pthread_mutex_unlock(&data_mutex);
		}
		else if ( check_command_value(command, CMD_CONTINUE) )
		{
			pthread_mutex_lock(&data_mutex);
			emu_state = EMU_CONTINUE;
			pthread_cond_signal(&breakpoint_cond);
			pthread_mutex_unlock(&data_mutex);
		}
		else if ( check_command_value(command, CMD_MOVE) )
		{
			int step_val;
			sscanf(input_str, "%s %x", command, &step_val);
			pthread_mutex_lock(&data_mutex);
			emu_step_point = step_val;
			emu_state = EMU_CONTINUE;
			pthread_cond_signal(&breakpoint_cond);
			pthread_mutex_unlock(&data_mutex);
		}
		else if ( check_command_value(command, CMD_NEXT) )
		{
			pthread_mutex_lock(&data_mutex);
			emu_step_point = emu_get_next_instr();
			emu_state = EMU_CONTINUE;
			pthread_cond_signal(&breakpoint_cond);
			pthread_mutex_unlock(&data_mutex);
		}
		else if ( check_command_value(command, CMD_SET) )
		{
			char var[30];
			int data_val;
			sscanf(input_str, "%s %s %x", command, var, &data_val);
			pthread_mutex_lock(&data_mutex);
			if ( (strcmp(var, "A") == 0) || (strcmp(var, "a") == 0))
				a = data_val;
			else if ( (strcmp(var, "B") == 0) || (strcmp(var, "b") == 0))
				b = data_val;
			else if ( (strcmp(var, "PC") == 0) || (strcmp(var, "pc") == 0))
				pc = data_val;
			else if ( (strcmp(var, "SP") == 0) || (strcmp(var, "sp") == 0))
				ram[0x81] = data_val;
			else if ( (strcmp(var, "DPTR") == 0) || (strcmp(var, "dptr") == 0))
				dptr = data_val;
			else if ( (strcmp(var, "PSW") == 0) || (strcmp(var, "psw") == 0))
			{
				sscanf(input_str, "%s %s %b", command, var, &data_val);
				psw = data_val;
			}
			pthread_mutex_unlock(&data_mutex);
		}
		else if ( check_command_value(command, CMD_HELP) )
		{
			enable_help = true;
		}
		clear_window_input_buffer(CMD_win);
		printCMD();
	}
}

void printCMD(void)
{
	clear_window(CMD_win);
	print_to_window(CMD_win, 0, "> ");
	print_to_window(CMD_win, 0, "%s", get_window_input_str(CMD_win));
	refresh_window(CMD_win);

}

void printPORTS(void)
{
	clear_window(PORT_win);
	print_to_window(PORT_win, 0, "P0\t");
	print_to_window(PORT_win, 1, "%08b", ram[0x80]);
	print_to_window(PORT_win, 0, "P1\t");
	print_to_window(PORT_win, 1, "%08b", ram[0x90]);
	print_to_window(PORT_win, 0, "P2\t");
	print_to_window(PORT_win, 1, "%08b", ram[0xa0]);
	print_to_window(PORT_win, 0, "P3\t");
	print_to_window(PORT_win, 1, "%08b", ram[0xb0]);
	refresh_window(PORT_win);
}

void printHELP(void)
{
	clear_window(HELP_win);
	print_to_window(HELP_win, 1, "Type help / h / ?: To get this help menu");
	print_to_window(HELP_win, 1, "");
	print_to_window(HELP_win, 1, "Commands that can be used -");
	print_to_window(HELP_win, 1, "breakpoint X / b X:");
	print_to_window(HELP_win, 1, "will place a breakpoint at the specified ROM location.");
	print_to_window(HELP_win, 1, "If the breakpoint already exists, it removes the breakpoint.");
	print_to_window(HELP_win, 1, "");
	print_to_window(HELP_win, 1, "continue / c:");
	print_to_window(HELP_win, 1, "will continue execution until next breakpoint.");
	print_to_window(HELP_win, 1, "");
	print_to_window(HELP_win, 1, "next / n:");
	print_to_window(HELP_win, 1, "will continue to the next instruction.");
	print_to_window(HELP_win, 1, "");
	print_to_window(HELP_win, 1, "move X / m X:");
	print_to_window(HELP_win, 1, "will continue execution until the specified ROM location.");
	print_to_window(HELP_win, 1, "");
	print_to_window(HELP_win, 1, "set X Y / m X Y:");
	print_to_window(HELP_win, 1, "will set the value of the X register to Y.");
	print_to_window(HELP_win, 1, "Eg., 'set A 45' will set the A register to 45.");
	print_to_window(HELP_win, 1, "supported registers - A, B, SP, PC, DPTR and PSW.");
	refresh_window(HELP_win);
	wgetch(HELP_win->win);
	enable_help = false;
	clear_window(HELP_win);
}

void printMISC(void)
{
	clear_window(MISC_win);
	print_to_window(MISC_win, 0, "PC = %04XH;  ", pc);
	print_to_window(MISC_win, 1, "// %s", instructions[rom[pc]].string);
	print_to_window(MISC_win, 0, "A = %02XH;  ", a);
	print_to_window(MISC_win, 0, "B = %02XH;  ", b);
	print_to_window(MISC_win, 0, "SP = %02XH;  ", ram[0x81]);
	print_to_window(MISC_win, 1, "DPTR = %04XH;", dptr);
	print_to_window(MISC_win, 1, "XTAL = %5f MHz;", XTALfreq);
	print_to_window(MISC_win, 1, "PSW = %08b;", psw);
	refresh_window(MISC_win);
}

void printROM(void)
{
	clear_window(ROM_win);
	start_color();
	init_pair(1, COLOR_BLACK, COLOR_WHITE);
	init_pair(2, COLOR_BLACK, COLOR_BLUE);
	init_pair(3, COLOR_BLACK, COLOR_RED);

	int instr_pos = 0;
	bool curr_instr = false;
	for (unsigned int i = 0; i < ROM_FILE_LEN; i++)
	{
		instr_pos = i;
		// For printing the ROM address at the beginning
		if ( find_in_set(breakpoints, instr_pos) )
			wattron(ROM_win->win, COLOR_PAIR(3));
		char buff[30];
		print_to_window(ROM_win, 0, "%04X", i, get_str_set(breakpoints, buff), instr_pos);
		wattroff(ROM_win->win, COLOR_PAIR(3));

		print_to_window(ROM_win, 0, ": ");
		if (i == pc)
			curr_instr = true;
		else
			curr_instr = false;

		// Sets color and displayes the first byte (instruction)
		// then removes colors
		if (curr_instr)
			wattron(ROM_win->win, COLOR_PAIR(1));
		print_to_window(ROM_win, 0, "%02X", rom[i]);
		wattroff(ROM_win->win, COLOR_PAIR(1));
		wattroff(ROM_win->win, COLOR_PAIR(2));
		print_to_window(ROM_win, 0, " ");

		// Sets color and displayes the rest of the bytes of instruction
		// then removes colors
		for (unsigned int j = 0; j < instructions[rom[instr_pos]].no_of_bytes - 1; j++)
		{
			if (curr_instr)
				wattron(ROM_win->win, COLOR_PAIR(2));
			print_to_window(ROM_win, 0, "%02X", rom[++i]);
			wattroff(ROM_win->win, COLOR_PAIR(1));
			wattroff(ROM_win->win, COLOR_PAIR(2));
			print_to_window(ROM_win, 0, " ");
		}

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
