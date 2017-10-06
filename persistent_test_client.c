#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

// TCP Client
// Developed by UVic TAs for CSC 361
// Modified by Cailan St Martin
 
int main(int argc,char **argv)
{
    if (argc != 2)
    {
       printf("Please provide a port number\n");
       exit(EXIT_FAILURE);
    }
    int sock_fd,n;
    char sendline[100];
    char recvline[1024];
    struct sockaddr_in servaddr;
 
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    bzero( &servaddr, sizeof(servaddr));
 
    servaddr.sin_family = AF_INET;
    int port = atoi(argv[1]);
    servaddr.sin_port = htons(port);
 
    inet_pton(AF_INET,"10.10.1.100", &(servaddr.sin_addr));
 
    connect(sock_fd, (struct sockaddr *) &servaddr, sizeof(servaddr));
 
    while(1)
    {
		fd_set read_fds;
		FD_ZERO(&read_fds);
		FD_SET(STDIN_FILENO, &read_fds);
		FD_SET(sock_fd, &read_fds);
		int num_fds = (sock_fd > STDIN_FILENO) ? sock_fd + 1 : STDIN_FILENO + 1;
		
		int select_result = select(num_fds, &read_fds, NULL, NULL, NULL);
		
		if (FD_ISSET(STDIN_FILENO, &read_fds))
		{
			bzero( sendline, 100);
			bzero( recvline, 1024);
			fgets(sendline,100,stdin); /*stdin = 0 , for standard input */
		   
			sendline[strlen(sendline) - 1] = '\0'; //removing trailing newline 
			strcat(sendline, "\r\n\r\n");
			
			write(sock_fd,sendline,strlen(sendline)+1);
		}
		else if (FD_ISSET(sock_fd, &read_fds))
		{
			read(sock_fd,recvline,1024);
			printf("%s", recvline);
			bzero(recvline, 1024);
		}
    }
 
}
