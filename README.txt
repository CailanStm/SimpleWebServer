Simple Web Server
Developed for CSC 361 at UVic by:
Cailan St Martin
V00826057
Lab section B03
February 2017

Building the code:
	- run "make" to build the executable
	- run "make clean" to clear the executable

Running the server:
	- to run the server with basic features, enter the command ./sws [port number] [serving directory]
	For example, execute the following to run the server on port 8080, serving the current directory:
	./sws 8080 /
 
	- to run the server with the bonus feature (persistent HTTP), enter the command ./sws -persistent [port number] [serving directory]

Basic code structure:
	The simple web server supports all the basic features outlined in the assignment specification.
	This includes handling basic HTTP 1.0 requests, generating each of the three required types of 
	HTTP responses, the desired log messages, pressing q to quit, and sending large files, among others.
 
	The majority of the logic is in the file sws.c, which is broken up into functions that handle
	the most important logical components of request/response handling. These functions are written 
	so that they can be used by both basic UDP and by persistent TCP (which is the bonus feature). 
	The main loop determines the type of socket based on whether the user passed "-persistent" in 
	to the command line, and then handles requests and responses slightly differently depending on 
	which type of socket is being used. Since the basic request/response handling is the same for both 
	methods, the UDP and TCP handling both call the same functions, just with differing parameters. This
	made the bonus feature much easier to implement, since not very much code had to be copied/rewritten.
 
	Some helper functions reside in helpers.c, which is included into sws.c via helpers.h

Testing basic features:
	A sample test file has been provided, testfile.sh. This runs 5 test cases against the server on port 8000.
	The expected responses are as follows:
	1) Return HTTP/1.0 200 OK followed by the contents of index.html
	2) Return HTTP/1.0 400 BAD REQUEST
	3) Return HTTP/1.0 404 NOT FOUND
	4) Return HTTP/1.0 200 OK followed by the contents of index.html
	5) Return HTTP/1.0 404 NOT FOUND

Bonus feature:
	The bonus feature I implemented, as discussed with Dr. Pan, is persistent HTTP. This had to be done
	using TCP, since UDP does not have the concept of a "connection". To do this, if the user passes in
	the "persistent" flag, the server creates a TCP "listener" socket, whose file descriptor is then 
	passed in to the select function. Once this receives a request, it creates a "connection" socket, and
	sets the "currently_connected" flag to 1. The next time through the main loop, the "connection" socket's
	file descriptor is passed to the select function, instead of the listener. This can then be used to 
	receive and handle incoming requests from the client. If the client closes the connection (by sending
	a packet with no data) or if the connection times out (a 10 second timeout is used server-side), then
	"currently_connected" is set to 0, and the original "listener" socket is then used to listen for
	incoming connection requests again.

Testing bonus feature:
	To test the bonus feature, a TCP client is required to set up a connection. Using code provided to us
	in the labs, I created a simple TCP test client. This is included in the tar file - "persistent_test_client.c"
 
	First, ensure that you have run the server with the "-persistent" flag - the server should log that it
	is listening on a TCP port, rather than a UDP port
 
	To use the test client, first compile it using "gcc -o persistent_test_client persistent_test_client.c"
 
	Next, run it with the port that your server is listening on, using:
		"./persistent_test_client [port number]"
	
	When the client is run, the server should log "TCP connection accepted" - this means the client has
	successfully opened a TCP connection
 
	Now, type in any HTTP request, as you would for the basic feature (except without \r\n\r\n)
		eg: GET / HTTP/1.0
	The server should respond the same as if this were a UDP connection: the client will receive the
	appropriate response. 
 
	Now, if you type multiple requests, they will be served over the same TCP connection.
 
	To close the connection, simply ctrl+C on the client, or wait 10 seconds without sending a request.
	The server will log that the connection has been closed (or timed out).
 
	To verify that the server is actually processing these requests over a single TCP connection: Assuming
	that you are running on a lab computer, you can ssh into the connected router:
		ssh csc361@192.168.1.1
	and then run tcpdump one the relevant interfaces:
		tcpdump -ibr0 tcp port [port server is listening on]
		
	Now, if you run the server and then connect with the client, you will see a TCP packet with the S flag -
	this indicates a connection packet! Subsequent requests sent by the client will have the P flag (for push),
	and then closing the connection with ctrl+C will send a packet with the F flag (for finish). This demonstrates
	that the server and client have truly been communicating over a persistent TCP connection!
 
References:
	www.tutorialspoint.com for help with coding in C
	www.cs.colby.edu/maxwell/courses/tutorials/maketutor/ for help with makefiles
	linux man pages for help with system calls
	CSC 361 lab code and resources for help, and specifically tcp_client.c was largely copied to create the persistent HTTP testing client