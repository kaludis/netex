#include "network.h"

#include "commands.h"
#include "bsd/queue.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define HOST_NAME "localhost"

struct address {
    struct addrinfo* ai_ptr;
    struct command* cmd_ptr;
    LIST_ENTRY(address)  addresses;
};

LIST_HEAD(addresses_head, address) addresses_head;

static
int init_addresses()
{
    int status, ret;
    struct addrinfo ai_hint, *ai_ptr;
    struct command* cmd_ptr;

    ret = 0;
    cmd_ptr = NULL;

    memset(&ai_hint, 0, sizeof(ai_hint));
    ai_hint.ai_family = AF_UNSPEC;
    ai_hint.ai_socktype = SOCK_STREAM;

    LIST_INIT(&addresses_head);

    LIST_FOREACH(cmd_ptr, &commands_head, commands) {
	char port[6];
	sprintf(port, "%hu", cmd_ptr->port);
	if ((status = getaddrinfo(HOST_NAME, port, &ai_hint, &ai_ptr)) != 0) {
	    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
	    continue;
	}

	struct address* addr_ptr = malloc(sizeof(struct address));
	addr_ptr->ai_ptr= ai_ptr;
	addr_ptr->cmd_ptr = cmd_ptr;

	LIST_INSERT_HEAD(&addresses_head, addr_ptr, addresses);

	++ret;
    }

    return ret;
}

static 
void free_addresses()
{
    struct address* addr_ptr;
    LIST_FOREACH(addr_ptr, &addresses_head, addresses) {
	freeaddrinfo(addr_ptr->ai_ptr);
	free(addr_ptr);
    }
}

static
void print_addresses()
{
    struct addrinfo* ptr;
    struct address* addr_ptr;
    char ipstr[INET6_ADDRSTRLEN];

    LIST_FOREACH(addr_ptr, &addresses_head, addresses) {
	fprintf(stdout, "PORT: %hu\n", addr_ptr->cmd_ptr->port);
	for (ptr = addr_ptr->ai_ptr; ptr != NULL; ptr = ptr->ai_next) {
	    void* addr;
	    char* ipver;

	    if (ptr->ai_family == AF_INET) { // IPv4
		struct sockaddr_in* ipv4 = (struct sockaddr_in*) ptr->ai_addr;		
		addr = &(ipv4->sin_addr);
		ipver = "IPv4";
	    } else { // IPv6
		struct sockaddr_in6* ipv6 = (struct sockaddr_in6*) ptr->ai_addr;
		addr = &(ipv6->sin6_addr);
		ipver = "IPv6";
	    }

	    if (!inet_ntop(ptr->ai_family, addr, ipstr, INET6_ADDRSTRLEN)) {
		perror("inet_ntop()");
	    } else {
		fprintf(stdout, "\t%s: %s\n", ipver, ipstr);		
	    }
	}
	puts("");
    }    
}

void listen_ports()
{
    if (!init_addresses()) {
	fprintf(stderr, "error: no port are available to listening");
	return;
    }

    print_addresses();

    free_addresses();
}
