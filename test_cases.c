#include <stdlib.h>
#include <stdio.h>
//#include <errno.h>
#include <string.h>
#include <sys/socket.h>
//#include <sys/types.h>
#include <netinet/in.h>
//#include <unistd.h>
//#include <arpa/inet.h>

void print_result(int test_num, const char* request, const char* expected_response, const char* actual_response, int response_len)
{
	char actual_response_adjusted[response_len];
	strncpy(actual_response_adjusted, actual_response, response_len);
	
	
	printf("***** Test number %d *****\n", test_num);
	printf("expected length: %zu, string length %zu, receive size %d\n", strlen(expected_response), strlen(actual_response), response_len);
	printf("Request: %s\n", request);
	printf("Expected response: %s\n", expected_response);
	printf("Actual response: %s\n", actual_response_adjusted);
	char result[16];
	if (strncmp(expected_response, actual_response, response_len) == 0)
	{
		strcpy(result, "passed");
	}
	else
	{
		strcpy(result, "failed");
	}
	printf("Test %s.\n\n", result);
}

void test_basic_features()
{
	int sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	
	struct sockaddr_in dest_address;
	memset(&dest_address, 0, sizeof dest_address);
	dest_address.sin_family = AF_INET;
	dest_address.sin_addr.s_addr = inet_addr("10.10.1.100");
	dest_address.sin_port = htons(8080);
	socklen_t dest_address_len = sizeof(dest_address);
	
	char request[256];
	char expected_response[256];
	char actual_response[256];
	ssize_t receive_size = 0;
	
	// test one
	strcpy(request, "GET /test.txt HTTP/1.0\r\n\r\n");
	sendto(sock, request, strlen(request), 0, (struct sockaddr*)&dest_address, sizeof dest_address);
	receive_size = recvfrom(sock, (void*)actual_response, sizeof(actual_response), 0, (struct sockaddr*)&dest_address, &dest_address_len);
	strcpy(expected_response, "HTTP/1.0 200 OK\r\n\r\nHello, this is a test.\n");
	print_result(1, request, expected_response, actual_response, receive_size);
	
	// test two
	strcpy(request, "GET / HTTP/1.0\r\n\r\n");
	sendto(sock, request, strlen(request), 0, (struct sockaddr*)&dest_address, sizeof dest_address);
	receive_size = recvfrom(sock, (void*)actual_response, sizeof(actual_response), 0, (struct sockaddr*)&dest_address, &dest_address_len);
	strcpy(expected_response, "HTTP/1.0 404 NOT FOUND\r\n\r\n");
	print_result(2, request, expected_response, actual_response, receive_size);
	
	// test three
	strcpy(request, "GET /test.txt HTTP/1.1\r\n\r\n");
	sendto(sock, request, strlen(request), 0, (struct sockaddr*)&dest_address, sizeof dest_address);
	receive_size = recvfrom(sock, (void*)actual_response, sizeof(actual_response), 0, (struct sockaddr*)&dest_address, &dest_address_len);
	strcpy(expected_response, "HTTP/1.0 400 BAD REQUEST\r\n\r\n");
	print_result(3, request, expected_response, actual_response, receive_size);
	
	// test four
	strcpy(request, "GET /../p1/test.txt HTTP/1.1\r\n\r\n");
	sendto(sock, request, strlen(request), 0, (struct sockaddr*)&dest_address, sizeof dest_address);
	receive_size = recvfrom(sock, (void*)actual_response, sizeof(actual_response), 0, (struct sockaddr*)&dest_address, &dest_address_len);
	strcpy(expected_response, "HTTP/1.0 404 NOT FOUND\r\n\r\n");
	print_result(4, request, expected_response, actual_response, receive_size);
	
	// test five
	strcpy(request, "GET /test.txt HTTP/1.0");
	sendto(sock, request, strlen(request), 0, (struct sockaddr*)&dest_address, sizeof dest_address);
	receive_size = recvfrom(sock, (void*)actual_response, sizeof(actual_response), 0, (struct sockaddr*)&dest_address, &dest_address_len);
	strcpy(expected_response, "HTTP/1.0 400 BAD REQUEST\r\n\r\n");
	print_result(5, request, expected_response, actual_response, receive_size);
	
	// test five
	strcpy(request, "get /test.txt http/1.0\r\n\r\n");
	sendto(sock, request, strlen(request), 0, (struct sockaddr*)&dest_address, sizeof dest_address);
	receive_size = recvfrom(sock, (void*)actual_response, sizeof(actual_response), 0, (struct sockaddr*)&dest_address, &dest_address_len);
	strcpy(expected_response, "HTTP/1.0 200 OK\r\n\r\nHello, this is a test.\n");
	print_result(6, request, expected_response, actual_response, receive_size);	
}

void test_persistent_http()
{
	
}

int main(void)
{
	test_basic_features();
}