#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>

#include "helpers.h"

void printAddress(const struct sockaddr_in* sockAddress, const socklen_t length)
{
	printf("sock address: %d\n", sockAddress->sin_addr.s_addr);
	printf("sock address length: %d\n", length);
	printf("sock address family: %d\n", sockAddress->sin_family);
	printf("sock address port: %d\n", sockAddress->sin_port);
}

void stringToUpper(char* string, size_t length)
{
	int i;
	for (i = 0; i < length; i++)
	{
		string[i] = toupper(string[i]);
	}
}

void print_error(const char* error_function)
{
	fprintf(stderr, "ERROR: %s failed: %s\n", error_function, strerror(errno));
}