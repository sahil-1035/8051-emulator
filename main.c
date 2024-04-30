#include "interface.h"
#include <stdio.h>
#include <string.h>

void help_menu(void)
{
	printf("Usage: 8051 [OPTION] [VALUE]\n");
	printf("  -f, --bin-file FILE_PATH\n");
}

int main(int argc, char** argv)
{
	bool bin_given = false;
	char bin_path[256];
	for (unsigned int i = 0; i < argc; i++)
	{
		/* printf("%s\n", argv[i]); */
		if ( strcmp(argv[i],"--bin-file") == 0 ||
				strcmp(argv[i], "-f") == 0)
		{
			strcpy(bin_path, argv[ ++i ]);
			bin_given = true;
		}
	}
	if (!bin_given)
		help_menu();
	else
		interface_main(bin_path);
	return 0;
}
