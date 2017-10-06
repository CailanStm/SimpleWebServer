#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <arpa/inet.h>

#include "helpers.h"

enum status_codes {OK, BAD_REQUEST, NOT_FOUND};
enum send_type {SEND_TO, WRITE};

// Parses the request header, to extract the method, uri, and version, or to determine if the request was invalid
enum status_codes parse_headers(const char* request, size_t request_length, char* method, char* uri, char* version, size_t small_bufsize)
{	
	const char delimiter = ' ';
	const char* end_of_line = "\r\n";
	const char* blank_line = "\r\n\r\n";
	const char terminater = '\0';	
	
	char first_line[request_length];
	strncpy(first_line, request, request_length);
	char* end_of_first_line = strstr(first_line, end_of_line);
	
	int request_ok = 1;
	
	if (end_of_first_line == NULL)
	{
		request_ok = 0;
	}
	
	// Ensure that request header includes a blank line somewhere after the first header line
	// That indicates the end of the request.
	if (request_ok == 1)
	{
		char* blank_linePtr = strstr(end_of_first_line, blank_line);
		if (blank_linePtr == NULL)
		{
			request_ok = 0;
		}
		
		*end_of_first_line = terminater;
	}
	
	char* token = strtok(first_line, &delimiter);
	size_t tokenNumber = 1;
	
	while (token != NULL)
	{
		if (strlen(token) > small_bufsize)
		{
			request_ok = 0;
		}

		if (tokenNumber == 1)
		{
			strncpy(method, token, small_bufsize);
		}
		else if (tokenNumber == 2)
		{
			strncpy(uri, token, small_bufsize);
		}
		else if (tokenNumber == 3)
		{
			strncpy(version, token, small_bufsize);
		}
		else
		{
			break;
		}
		
		token = strtok(NULL, &delimiter);
		tokenNumber++;
	}
	
	if (request_ok == 1)
	{
		return OK;
	}
	else
	{
		return BAD_REQUEST;
	}
}

// Computes the appropriate HTTP response based on the important parts of the request header
// This function includes storing the contents of the requested file into memory
char* compute_response(const struct sockaddr_in* source_address_ptr, const char* method, const char* uri, const char* version, const char* full_file_path, enum status_codes response_status)
{	
	const char* required_method = "GET";
	const char* required_version = "HTTP/1.0";
	
	if (strcmp(method, required_method) != 0)
	{
		response_status = BAD_REQUEST;
	}
	if (strcmp(version, required_version) != 0)
	{
		response_status = BAD_REQUEST;
	}
	if (strstr(uri, "..") != NULL)
	{
		response_status = NOT_FOUND;
	}
	
	size_t response_header_length = 256;
	char response_header[response_header_length];
	strcpy(response_header, "HTTP/1.0 ");
			
	long file_length = 0;
	char* file_data = NULL;

	if (response_status == OK)
	{
		FILE* file_to_serve = fopen(full_file_path, "r");
		if (file_to_serve)
		{
			fseek(file_to_serve, 0, SEEK_END);
			file_length = ftell(file_to_serve);
			fseek(file_to_serve, 0, SEEK_SET);

			file_data = (char*) malloc(file_length);
			fread(file_data, file_length, 1, file_to_serve);
		}
		else
		{
			response_status = NOT_FOUND;
		}
	}
	
	switch (response_status)
	{
		case OK:
		{			
			strcat(response_header, "200 OK");
			break;
		}
		case BAD_REQUEST:
		{
			strcat(response_header, "400 BAD REQUEST");
			break;
		}
		case NOT_FOUND:
		{
			strcat(response_header, "404 NOT FOUND");
			break;
		}
	}
	
	time_t rawtime;
	struct tm* time_info;
	time(&rawtime);
	time_info = localtime(&rawtime);
	
	char time_string[32];
	strftime(time_string, 32, "%b %d %T", time_info);
	
	fflush(stdout);
	printf("%s %s:%d %s %s %s; %s; %s\n", time_string, inet_ntoa(source_address_ptr->sin_addr), ntohs(source_address_ptr->sin_port), method, uri, version, response_header, full_file_path);
	
	strcat(response_header, "\r\n\r\n");
	
	size_t full_response_size = strlen(response_header) + file_length + 1;
	
	char* full_response = (char*) malloc(full_response_size);
	strcpy(full_response, response_header);			
	if (file_data != NULL)
	{
		strncat(full_response, file_data, file_length);
	}
	
	free(file_data);
	
	return full_response;
}

// Processes the request. Takes in a request, source address and directory to serve, and returns the appropriate response
// Note that this will return a pointer to memory allocated on the heap, which should be freed after it is used
char* process_request(const struct sockaddr_in* source_address_ptr, char* request_buffer, size_t receive_size, const char* server_directory)
{
	enum status_codes response_status = OK; 
	
	if (receive_size < 0)
	{
		print_error("accept packets");
		exit(EXIT_FAILURE);
	}
	
	size_t small_bufsize = 64;
	char method[small_bufsize];
	char uri[small_bufsize];
	char version[small_bufsize];
	memset(method, '\0', small_bufsize);
	memset(uri, '\0', small_bufsize);
	memset(version, '\0', small_bufsize);
	
	response_status = parse_headers(request_buffer, receive_size, method, uri, version, small_bufsize);
	
	stringToUpper(method, strlen(method));
	stringToUpper(version, strlen(version));
	
	// ensure that there is space for index.html
	size_t full_path_size = strlen(server_directory) + strlen(uri) + 32;
	
	char full_file_path[full_path_size];
	strcpy(full_file_path, server_directory);
	strcat(full_file_path, uri);
	if (strcmp(uri, "/") == 0)
	{
		strcat(full_file_path, "index.html");
	}
	
	return compute_response(source_address_ptr, method, uri, version, full_file_path, response_status);
}

// Handles the command line arguments, including whether the user has run the server in persistent mode
int handle_arguments(int argc, char* argv[], int* server_port, char* full_server_directory)
{
	int persistent = 0;
	if (argc == 4)
	{
		if (strcmp(argv[1], "-persistent") != 0)
		{
			printf("Incorrect syntax for starting server.\nPlease use: ./sws [-persistent] <port> <directory>\nExiting.\n");
			exit(EXIT_FAILURE);
		}
		else
		{
			persistent = 1;
		}
	}
	
	int valid_num_arguments = (persistent == 0) ? 3 : 4;
	
	if (argc != valid_num_arguments)
	{
		printf("Incorrect syntax for starting server.\nPlease use: ./sws [-persistent] <port> <directory>\nExiting.\n");
		exit(EXIT_FAILURE);
	}
	
	int server_port_index = 1 + persistent;
	int server_directory_index = 2 + persistent;
	
	*server_port = atoi(argv[server_port_index]);
	if (server_port <= 0)
	{
		printf("Invalid port number specified.\nEnsure that syntax is correct: ./sws [-persistent] <port> <directory>\nExiting.\n");
		exit(EXIT_FAILURE);
	}	
	
	const char* server_directory_in = argv[server_directory_index];	
	char server_relative_directory[strlen(server_directory_in)+2];
	strcpy(server_relative_directory, ".");
	strcat(server_relative_directory, server_directory_in);
	
	if (realpath(server_relative_directory, full_server_directory) == NULL)
	{
		print_error("server directory lookup");
		exit(EXIT_FAILURE);
	}
	
	char protocol[4];
	if (persistent == 1)
	{
		strcpy(protocol, "TCP");
	}
	else
	{
		strcpy(protocol, "UDP");
	}
	
	printf("sws is running on %s port %d and serving %s\npress 'q' to quit ...\n", protocol, *server_port, full_server_directory);

	return persistent;
}

// Quits the program if the user input was a q
void accept_user_quit()
{
	char user_input[32] = "";
	fgets(user_input, 32, stdin);
	if (strcmp(user_input, "q\n") == 0)
	{
		printf("Goodbye!\n");
		exit(EXIT_SUCCESS);
	}
}

// Sends the response (which is passed in as a parameter) to the client
// Handles breaking the response into chunks if the response is too large
// The send_type is used to differentiate between TCP responses (using write) and UDP responses (using sendto)
void send_response(int socket_fd, const struct sockaddr_in* source_address_ptr, const char* full_response, size_t full_response_len, size_t max_response_size, enum send_type send_function)
{
	int index = 0;
	socklen_t source_addr_len = 0;
	if (source_address_ptr != NULL)
	{
		source_addr_len = sizeof(*source_address_ptr);
	}
	
	if (full_response_len <= max_response_size)
	{
		if (send_function == SEND_TO)
		{
			if (sendto(socket_fd, (void*)full_response, full_response_len, 0, (const struct sockaddr*)source_address_ptr, source_addr_len) == -1)
			{
				print_error("sendto");
			}
		}
		else if (send_function == WRITE)
		{
			if (write(socket_fd, full_response, full_response_len) == -1)
			{
				print_error("write");
			}
		}
	}
	else
	{
		while (index < full_response_len)
		{
			const char* response_segment = &full_response[index];
			size_t segment_length = strlen(response_segment);
			size_t send_length = max_response_size;
			if (segment_length < max_response_size)
			{
				send_length = segment_length;
			}
			if (send_function == SEND_TO)
			{
				if (sendto(socket_fd, (void*)response_segment, send_length, 0, (const struct sockaddr*)source_address_ptr, source_addr_len) == -1)
				{					
					print_error("sendto");
				}
			}
			else if (send_function == WRITE)
			{
				if (write(socket_fd, (void*)response_segment, send_length) == -1)
				{
					print_error("write");
				}
			}
			index += max_response_size;
		}
	}
}

// Creates and binds the socket using the port and protocol specified by the user
int setup_socket(int server_port, int persistent)
{
	int sock = 0;
	if (persistent == 1)
	{
		sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	}
	else
	{
		sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	}
	
	struct sockaddr_in sockAddressIN;
	memset(&sockAddressIN, 0, sizeof(sockAddressIN));
	sockAddressIN.sin_family = AF_INET;
	sockAddressIN.sin_addr.s_addr = htonl(INADDR_ANY);
	sockAddressIN.sin_port = htons(server_port);
	
	int enable = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) < 0)
	{
		print_error("setsockopt");
	}
	
	if (bind(sock, (struct sockaddr*)&sockAddressIN, sizeof sockAddressIN) == -1)
	{
		print_error("bind");
		close(sock);
		exit(EXIT_FAILURE);
	}
	
	if (persistent == 1)
	{
		if (listen(sock, 10) == -1)
		{
			print_error("listen");
			close(sock);
			exit(EXIT_FAILURE);
		}
	}
	
	return sock;
}

int main(int argc, char* argv[])
{	
	int server_port = 0;
	char full_server_path[256];
	int persistent = handle_arguments(argc, argv, &server_port, full_server_path);	
	
	int sock = setup_socket(server_port, persistent);	
	
	int currently_connected = 0; // will be 1 if a TCP connection has been opened
	int listening_socket = -1;
	int connection_fd = -1;

	size_t max_response_size = 512;
	struct sockaddr_in sourceAddress;
	
	for(;;)
	{
		if (currently_connected == 0)
		{
			// don't want to reset this if a TCP connection is still open
			memset(&sourceAddress, 0, sizeof sourceAddress);
		}
		socklen_t sourceAddrLen = sizeof(sourceAddress);
		char request_buffer[1024] = "";	
		
		// Set whether to listen at the listener socket, or at the connected
		// socket (in the case of an ongoing TCP connection)
		listening_socket = (currently_connected == 1) ? connection_fd : sock;
		
		fd_set read_fds;
		FD_ZERO(&read_fds);
		FD_SET(STDIN_FILENO, &read_fds);
		FD_SET(listening_socket, &read_fds);
		
		int num_fds = (sock > STDIN_FILENO) ? listening_socket + 1 : STDIN_FILENO + 1;
		
		struct timeval timeout;
		int select_result = -1;
		if (currently_connected == 1)
		{
			// only keep TCP connections alive for a short period of time
			timeout.tv_sec = 10;
			select_result = select(num_fds, &read_fds, NULL, NULL, &timeout);
		}
		else
		{
			select_result = select(num_fds, &read_fds, NULL, NULL, NULL);
		}
		
		if (select_result == 0)
		{
			// timeout occurred - this can only happen on an active TCP connection
			printf("TCP connection timed out\n");
			close(connection_fd);
			currently_connected = 0;
			continue;
		}
		else if (select_result == -1)
		{
			printf("Unexpected failure with select, exiting.\n");
			exit(EXIT_FAILURE);
		}
		
		if (FD_ISSET(STDIN_FILENO, &read_fds))
		{
			accept_user_quit();
		}
		else if (FD_ISSET(sock, &read_fds))
		{
			if (persistent == 0)
			{
				// Handle as a UDP request
				
				ssize_t receive_size;
				receive_size = recvfrom(sock, (void*)request_buffer, sizeof(request_buffer), 0, 
										(struct sockaddr*)&sourceAddress, &sourceAddrLen);
				
				char* full_response = process_request(&sourceAddress, request_buffer, receive_size, full_server_path);
				
				send_response(sock, &sourceAddress, full_response, strlen(full_response), max_response_size, SEND_TO);
				
				free(full_response);
			}
			else
			{
				// Handle as a TCP connection request
				
				printf("TCP connection accepted\n");
				//opens the TCP connection
				connection_fd = accept(sock, (struct sockaddr*)&sourceAddress, &sourceAddrLen);
				currently_connected = 1;
			}
		}
		else if (FD_ISSET(connection_fd, &read_fds))
		{
			// Handle as a TCP data request
			
			int receive_size = 0;
			receive_size = recv(connection_fd, (void*)request_buffer, sizeof(request_buffer), 0);
			if (receive_size == -1)
			{
				print_error("recv");
				exit(EXIT_FAILURE);
			}
			if (receive_size == 0)
			{
				printf("TCP connection closed\n");
				currently_connected = 0;
				close(connection_fd);
				continue;
			}
						
			char* full_response = process_request(&sourceAddress, request_buffer, receive_size, full_server_path);
			
			send_response(connection_fd, NULL, full_response, strlen(full_response), max_response_size, WRITE);
			
			free(full_response);
		}
		
	}
}
