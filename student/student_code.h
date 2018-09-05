#include <netdb.h>

/*
 * This function should return 0 in all cases.
 */
int report_address_info(char *node, char *service, struct addrinfo **answer, int force_numeric_host, int force_numeric_serv);

int print_address(char *node, char *service);

void verbose();

void lol();
