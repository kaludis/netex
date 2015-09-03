#include "cfgreader.h"

#include "commands.h"
#include "exit.h"

#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <assert.h>

#define LINE_SIZE 512

static
struct command* get_cmd_from_line(const char* line)
{
    char* c;
    struct command* cmd_ptr;
    cmd_ptr = NULL;

    c = strchr(line, ' ');
    if (c) {
	char port[6];

	ptrdiff_t len = c - line;
	strncpy(port, line, len);
	port[len + 1] = '\0';
	++len;
	size_t cmd_len = strlen(&line[len]);

	if (len && cmd_len) {
	    cmd_ptr = malloc(sizeof(struct command));
	    cmd_ptr->port = atoi(port);
	    
	    cmd_ptr->name = malloc(sizeof(char) * cmd_len);
	    strncpy(cmd_ptr->name, &line[len], cmd_len);
	    cmd_ptr->name[cmd_len - 1] = '\0';
	}
    }

    return cmd_ptr;
}

void read_config(const char* file_name)
{
    assert(file_name);

    char buf[LINE_SIZE];
    struct command *cmd_ptr;
    cmd_ptr = NULL;

    FILE* cmd_file_ptr = fopen(file_name, "r");
    if (!cmd_file_ptr) {
	EXIT("fopen()");
    }

    while (fgets(buf, LINE_SIZE, cmd_file_ptr) != NULL) {
	cmd_ptr = get_cmd_from_line(buf);
	if (cmd_ptr) LIST_INSERT_HEAD(&commands_head, cmd_ptr, commands);
    }

    if (feof(cmd_file_ptr)) {
	puts("Configuration file loaded");
	goto close_file;
    } else if (ferror(cmd_file_ptr)) {
	puts("Unknown i/o error");
	goto close_file;
    }

close_file:
    if (fclose(cmd_file_ptr) == EOF) {
	EXIT("fclose()");	    
    }
}

const char* cmd_by_port(unsigned short port)
{
    assert(port > 0);

    struct command* cmd_ptr;
    const char* cmd_name;

    cmd_name = NULL;

    LIST_FOREACH(cmd_ptr, &commands_head, commands) {
	if (cmd_ptr->port == port) {
	    cmd_name = cmd_ptr->name;
	    break;
	}
    }    

    return cmd_name;
}

void clear_config()
{
    struct command* cmd_ptr;
    LIST_FOREACH(cmd_ptr, &commands_head, commands) {
	LIST_REMOVE(cmd_ptr, commands);
	free(cmd_ptr->name);
	free(cmd_ptr);
    }    
}








