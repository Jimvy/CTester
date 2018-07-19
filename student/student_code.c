#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include "student_code.h"

//void __real_exit(int status);

int print_address(char *node, char *service)
{
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = 0;
    hints.ai_protocol = 0;
    hints.ai_flags = 0;
    int rep = getaddrinfo(node, service, &hints, &res);
    if (rep != 0) {
	fprintf(stderr, "%s\n", gai_strerror(rep));
	return rep;
    }
    for (struct addrinfo *rp = res; rp != NULL; rp = rp->ai_next) {
	int length = 0;
	void *addr = NULL;
	in_port_t port = 0;
	switch (rp->ai_addr->sa_family) {
	    case AF_INET:
		length = INET_ADDRSTRLEN;
		addr = &(((struct sockaddr_in*)rp->ai_addr)->sin_addr);
		port = ((struct sockaddr_in*)rp->ai_addr)->sin_port;
		break;
	    case AF_INET6:
		length = INET6_ADDRSTRLEN;
		addr = &(((struct sockaddr_in6*)rp->ai_addr)->sin6_addr);
		port = ((struct sockaddr_in6*)rp->ai_addr)->sin6_port;
		break;
	};
	char *buf = malloc(length * sizeof(char));
	if (buf == 0) {
	    freeaddrinfo(res);
	    return +1;
	}
	printf("%s %hu %d %d\n", inet_ntop(rp->ai_addr->sa_family, addr, buf, (socklen_t)(length * sizeof(char))), ntohs(port), rp->ai_socktype, rp->ai_protocol);
	free(buf);
    }
    printf("\n");
    freeaddrinfo(res);
    return 0;
}

void lol()
{
    //__real_exit(127);
    exit(127);
    printf("Salut\n");
}

void verbose()
{
    printf("Coucou\n");
    /*
    int err = 0;
    for (int i = 0; i < 4*1000; i++) {
	err = printf("Printing line %d\n", i);
	if (err <= 0)
	    fprintf(stderr, "Error at %d: %d\n", i, err);
    }
    */
}

int report_address_info(char *node, char *service, struct addrinfo **answer, int force_numeric_host, int force_numeric_serv) {
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = 0;
    hints.ai_protocol = 0;
    hints.ai_flags = AI_CANONNAME;
    if (node == NULL) {
	// serveur
	hints.ai_flags = AI_PASSIVE;
    }
    if (force_numeric_host) {
	hints.ai_flags |= AI_NUMERICHOST;
    }
    if (force_numeric_serv) {
	hints.ai_flags |= AI_NUMERICSERV;
    }
    int ret = getaddrinfo(node, service, &hints, &res);
    if (ret != 0) {
	errno = ret;
	return -2;
    }
    *answer = res;
    return 0;
}

