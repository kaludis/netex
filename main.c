#include "cfgreader.h"
#include "network.h"
#include "commands.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <signal.h>
#include <unistd.h>

#define UNUSED(x) (void)(x)

void sig_handler(int signum, siginfo_t* info, void* ptr)
{
    UNUSED(info);
    UNUSED(ptr);

    if (signum == SIGTSTP) {
	fprintf(stdout, "\nshutdown signal has been caught\n");
	shutdown_signal();
	clear_config();
	fprintf(stdout, "all resources has been released\nbye!\n");
	exit(EXIT_SUCCESS);
    }
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
	printf("%s: missing file operand\n", argv[0]);
	printf("usage: %s <file>\n", argv[0]);
	return EXIT_FAILURE;
    }

    struct sigaction sact;
    memset(&sact, 0, sizeof(sact));

    sact.sa_sigaction = sig_handler;
    sact.sa_flags = SA_SIGINFO;

    sigaction(SIGTSTP, &sact, NULL);

    LIST_INIT(&commands_head);

    read_config(argv[1]);

/*
    printf("command on port 10103: '%s'\n", 
	   cmd_by_port(10103));
*/

    listen_ports();

    clear_config();

    return EXIT_SUCCESS;
}
