#include "network.h"

#include "commands.h"
#include "bsd/queue.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>


#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <sys/epoll.h>

#define BACKLOG 10
#define HOST_NAME "localhost"

static struct events_info {
    struct epoll_event* events;
    int sfd_count;
} sfd_events;

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

static
int allocate_epoll_events()
{
    struct addrinfo* ptr;
    struct address* addr_ptr;
    int ret;
    ret = 0;

    sfd_events.events = NULL;
    sfd_events.sfd_count = 0;

    /*
     * Use only IPv4 interfaces for simplicity
     */
    LIST_FOREACH(addr_ptr, &addresses_head, addresses) {
	for (ptr = addr_ptr->ai_ptr; ptr != NULL; ptr = ptr->ai_next) {
	    if (ptr->ai_family == AF_INET) {
		++sfd_events.sfd_count;			    
		break;
	    }
	}
    }

    if (sfd_events.sfd_count) {
	sfd_events.events = malloc(sizeof(struct epoll_event) * sfd_events.sfd_count);
	if (!sfd_events.events) {
	    perror("malloc()");
	    ret = -1;
	}
    }

    return ret;
}

static
void deallocate_epoll_events()
{
    free(sfd_events.events);
}

static
int close_socket(int sfd)
{
    int ret;   

    while ((ret = close(sfd) != 0)) {
	if (errno == EINTR) {
	    continue;
	} else if (errno != EBADF) {
	    break;
	} else {
	    perror("close()");
	    break;	    
	}
    }

    return ret;
}

static
void set_nonblock(int sfd) 
{
    int flags;

    flags = fcntl(sfd, F_GETFL,0);
    if (flags == -1) {
	perror("fcntl()");
	close_socket(sfd);	
    } else {
	if (fcntl(sfd, F_SETFL, flags | O_NONBLOCK) == -1) {
	    perror("fcntl()");
	    close_socket(sfd);		    
	}
    }
}

static
void init_epoll_events()
{
    struct addrinfo* ptr;
    struct address* addr_ptr;
    int sock_index;
    sock_index = 0;

    if (allocate_epoll_events() != -1) {
	/*
	 * Use only IPv4 interfaces for simplicity
	 */
	LIST_FOREACH(addr_ptr, &addresses_head, addresses) {
	    for (ptr = addr_ptr->ai_ptr; ptr != NULL; ptr = ptr->ai_next) {
		if (ptr->ai_family == AF_INET) {
		    int sfd = socket(ptr->ai_family, 
				     ptr->ai_socktype, 
				     ptr->ai_protocol);
		    if (sfd == -1) {
			perror("socket()");
		    } else {
			int ok;
			ok = 1;

			/*
			 * Set socket to nonblocking mode
			 */
			set_nonblock(sfd);

			/*
			 * Reuse socket
			 */
			if (setsockopt(sfd, 
				       SOL_SOCKET, 
				       SO_REUSEADDR, 
				       &ok, 
				       sizeof(int)) == -1) {
			    fprintf(stderr, 
				    "Error setsockopt for reuse socket%d\n", 
				    sfd);
			}

			/*
			 * Bind socket
			 */
			if (bind(sfd, ptr->ai_addr, ptr->ai_addrlen) == -1) {
			    perror("bind()");
			    close_socket(sfd);
			    continue;
			}

			/*
			 * Enable socket for listening
			 */
			if (listen(sfd, BACKLOG) == -1) {
			    perror("listen()");
			    close_socket(sfd);
			    continue;
			}

			sfd_events.events[sock_index].data.fd = sfd;
			sfd_events.events[sock_index].events = 
			    EPOLLIN | EPOLLOUT;
			++sock_index;
			fprintf(stdout, "socket %d created\n", sfd);
			break;
		    }
		}
	    }
	}
    }
}

static
void clear_epoll_events()
{
    int sock_index;

    for (sock_index = 0; sock_index < sfd_events.sfd_count; ++sock_index) {
	if (!close_socket(sfd_events.events[sock_index].data.fd))
	    fprintf(stdout , "socket %d closed\n",
		    sfd_events.events[sock_index].data.fd);
    }

    deallocate_epoll_events();
}

void listen_ports()
{
    int epfd, ret, sock_index, client_fd;
    char ipstr[INET6_ADDRSTRLEN];
    void* addr;

    if (!init_addresses()) {
	fprintf(stderr, "error: no port are available to listening");
	return;
    }

    init_epoll_events();

    epfd = epoll_create1(0);
    if (epfd < 0) {
	perror("epoll_create1()");
    } else {
	for (sock_index = 0; sock_index < sfd_events.sfd_count; ++sock_index) {
	    ret = epoll_ctl(epfd, 
			    EPOLL_CTL_ADD, 
			    sfd_events.events[sock_index].data.fd,
			    &sfd_events.events[sock_index]);
	    if (ret == -1) perror("epoll_ctl()");
	}

	while (1) {
	    fprintf(stdout, "\npress Ctrl-Z to shutdown server\n\n");
	    ret = epoll_wait(epfd, 
			     sfd_events.events, 
			     sfd_events.sfd_count, -1);
	    if (ret == -1) {
		perror("epoll_wait()");
		break;
	    }

	    for (sock_index = 0; sock_index < sfd_events.sfd_count; ++sock_index) {
		int listener = sfd_events.events[sock_index].data.fd;
		if ((sfd_events.events[sock_index].events & EPOLLOUT) ||
		    (sfd_events.events[sock_index].events & EPOLLIN)) {
		    struct sockaddr client_info;
		    socklen_t client_info_size;

		    client_fd = accept(listener,
				       &client_info, 
				       &client_info_size);
		    if (client_fd == -1) {
			perror("accept()");
		    } else {
			struct sockaddr_in* ipv4 = (struct sockaddr_in*) &client_info;
			addr = &(ipv4->sin_addr);		    
			
			if (!inet_ntop(AF_INET, addr, ipstr, INET6_ADDRSTRLEN)) {
			    perror("inet_ntop()");
			} else {
			    fprintf(stdout, "new client: %s", ipstr);
			}
			close_socket(client_fd);
		    }
		}

		if (!--ret) break;
	    }
	}
    }

    clear_epoll_events();

    /* print_addresses(); */

    free_addresses();
}

void shutdown_signal()
{
    clear_epoll_events();
    free_addresses();
}








