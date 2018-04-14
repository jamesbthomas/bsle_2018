// C Source file containing function definitions for the TCPHandler module
#ifndef TCP_HEADER
#define TCP_HEADER

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

// Function to make a TCP socket on the provided port
// Returns the file descriptor for the socket, or -1 on error
int makeTCPSocket(int port){
	if (port < 1 || port > 65535){
		return -1;
	}
	// Create the socket
	int sock = socket(AF_INET,SOCK_STREAM,0);
	if (sock < 0){
		perror("Socket Create Error");
		return -1;
	}
	// Let the OS reuse this port, makes it possible to receive from the C2 and send to the FSS on the same socket
	int reuse = 1;
	if (setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(int)) < 0){
		perror("Set Socket Option Error");
		close(sock);
		return -1;
	}
	// Bind
	struct sockaddr_in addr;
	memset(&addr,0,sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;
	if (bind(sock,(struct sockaddr *) &addr,sizeof(addr)) < 0){
		perror("Bind Error");
		close(sock);
		return -1;
	}
	// Setup to listen
	if (listen(sock,1) < 0){
		perror("Listen Error");
		close(sock);
		return -1;
	}
	return sock;
}

// Function to craft the validation 0x03 packet given the decoded validation message and the port for the C2 to use for the transfer
// Returns 0 on success, -1 on error
int craftValidation(unsigned char * validation, unsigned char * decoded,int port,int len){
	// Set the packet type
	validation[0] = 0x03;
	// Set the port
	if (port < 1 || port > 65535){
		return -1;
	}
	validation[1] = port >> 8;
	validation[2] = port & 0xff;
	// Copy the message into the packet
	memcpy(validation+3,decoded,len);
	return 0;
}

#endif
