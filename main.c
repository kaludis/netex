#include "cfgreader.h"
#include "network.h"
#include "commands.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[])
{
    if (argc < 2) {
	printf("%s: missing file operand\n", argv[0]);
	printf("usage: %s <file>\n", argv[0]);
	return EXIT_FAILURE;
    }

    LIST_INIT(&commands_head);

    read_config(argv[1]);

    printf("command on port 10103: '%s'\n", 
	   cmd_by_port(10103));

    listen_ports();

    clear_config();

    return EXIT_SUCCESS;
}
