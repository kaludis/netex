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

	ptrdiff_t new_len, l;

	c = strchr(&line[len], ' ');

	new_len = c - &line[len];
	if (new_len) {
	    cmd_ptr = malloc(sizeof(struct command));
	    cmd_ptr->port = atoi(port);

	    cmd_ptr->name = malloc(sizeof(char) * new_len);
	    strncpy(cmd_ptr->name, &line[len], new_len);
	    cmd_ptr->name[new_len] = '\0';
	    
	    fprintf(stdout, "\tcommand: %s\n", cmd_ptr->name);

	    len += new_len;
	    ++len;

	    unsigned args_cnt, pos, last_pos;
	    args_cnt = 0;
	    pos = last_pos = len;

	    do {
		c = strchr(&line[pos], ' ');

		++args_cnt;

		if (!c) break;

		last_pos = c - &line[pos];
		pos += last_pos;
		++pos;
	    } while(1);


	    cmd_ptr->args = malloc(sizeof(char*) * args_cnt + 2);

	    c = strrchr(cmd_ptr->name, '/');

	    new_len = c - cmd_ptr->name;
	    l = strlen(&cmd_ptr->name[new_len]);
	    --l;
	    cmd_ptr->args[0] = malloc(sizeof(char) * l + 1);
	    strncpy(cmd_ptr->args[0], &cmd_ptr->name[new_len + 1], l);
	    cmd_ptr->args[0][l] = '\0';

	    cmd_ptr->args[args_cnt] = NULL;

	    unsigned i;

	    fprintf(stdout, "\targ%d: '%s' %d\n", 0, cmd_ptr->args[0], (int)strlen(cmd_ptr->args[0]));

	    for (i = 1; i <= args_cnt; ++i) {
		c = strchr(&line[len], ' ');

		if (!c) {
		    new_len = strlen(line) - len;
		    cmd_ptr->args[i] = malloc(sizeof(char) * new_len);
		    strncpy(cmd_ptr->args[i], &line[len], new_len);
		    cmd_ptr->args[i][new_len - 1] = '\0';
		} else {
		    new_len = c - &line[len];
		    cmd_ptr->args[i] = malloc(sizeof(char) * new_len);
		    strncpy(cmd_ptr->args[i], &line[len], new_len);
		    cmd_ptr->args[i][new_len] = '\0';
		}

		fprintf(stdout, "\targ%d: '%s' %d\n", i, cmd_ptr->args[i], (int)strlen(cmd_ptr->args[i]));

		len += new_len;
		++len;
	    }
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
    char** arg_ptr;

    LIST_FOREACH(cmd_ptr, &commands_head, commands) {
	LIST_REMOVE(cmd_ptr, commands);

	free(cmd_ptr->name);

	for (arg_ptr = cmd_ptr->args; *arg_ptr != NULL; ++arg_ptr) {
	    free(*arg_ptr);
	}

	free(cmd_ptr->args);
	free(cmd_ptr);
    }    
}








