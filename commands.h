#ifndef COMMANDS_H_
#define COMMANDS_H_

#include "bsd/queue.h"

struct command {
    char* name;
    char** args;
    unsigned short port;
    LIST_ENTRY(command) commands;
};

LIST_HEAD(commands_head, command) commands_head;

#endif  /* COMMANDS_H_ */
