// Main function for the File Transfer Service module
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "../headers/encoder.h"
#include "../headers/udpHandler.h"

#define TOTAL_SOCKETS 1001 // Total number of sockets you want to create, should be (high port) - (low port) + 1
#define LOW_PORT 16000	// Lowest port to use
#define TIMEOUT 2000	// Timeout value when reading from a socket in microseconds
#define MAX_SIZE 1450	// Maximum expected message size

int main(int argc, char ** argv){
	// Create an array to hold the file descriptors
	int * sockets = calloc(TOTAL_SOCKETS,sizeof(int));
	// For each socket we want to create
	for (int i = 0;i < TOTAL_SOCKETS;i++){
		// Create the file descriptor
		int fd = socket(AF_INET,SOCK_DGRAM,0);
		// Calculate the port
		int port = i + LOW_PORT;
		// Make sure the socket succeeded
		if (fd == -1){
			perror("Error: Failed to create file descriptor");
			exit(2);
		}
		// Make a struct to contain the address/port for the socket
		struct sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		addr.sin_addr.s_addr = INADDR_ANY;
		// Create the struct to hold the timeout
		struct timeval t;
		t.tv_sec = 0;
		t.tv_usec = TIMEOUT;
		// Set the timeout to the socket
		if (setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,(char *)&t,sizeof(t)) == -1){
			perror("Failed to apply timeout");
			exit(2);
		}
		// Bind the file descriptor to the socket address
		if (bind(fd,(struct sockaddr *) &addr,sizeof(addr)) == -1){
			perror("Failed to bind");
			exit(2);
		}
		// Add the file descriptor to the array
		sockets[i] = fd;
	}
	// track the last created file descriptor
	int maxfd = sockets[TOTAL_SOCKETS-1];
	// Allocate a buffer to hold the messages
	unsigned char * dgram = calloc(MAX_SIZE,sizeof(unsigned char));
	// Allocate an FD set to keep track of the file descriptor statuses
	fd_set fds;
	// Create an array to track the thread IDs
	pthread_t * tids = calloc(TOTAL_SOCKETS,sizeof(pthread_t));
	int currThread = 0;
	while (1) {
		// Zero out the statuses
		FD_ZERO(&fds);
		// Add each file descriptor to the set
		for (int i = 0;i < TOTAL_SOCKETS;i++){
			FD_SET(sockets[i],&fds);
		}
		// Scan our sockets and until we get one that is ready
		int rcvd = select(maxfd+1,&fds,NULL,NULL,NULL);
		for (int x = 0;x < TOTAL_SOCKETS;x++){
			// Iterate through all of our sockets and see which one is ready
			if (FD_ISSET(sockets[x],&fds)){
				// Read from the socket
				struct sockaddr_in from;
				socklen_t fromLen = sizeof(from);
				recvfrom(sockets[x],dgram,MAX_SIZE,0,(struct sockaddr *) &from,&fromLen);
//				dgram[n] = '\0';
				// Grab info from the packet
				struct sockaddr_in s;
				socklen_t len = sizeof(s);
				getsockname(sockets[x],(struct sockaddr *) &s,&len);
				session * newSession = calloc(1,sizeof(session));
				newSession->sport = ntohs(from.sin_port);
				newSession->dport = ntohs(s.sin_port);
				newSession->packet = dgram;
				newSession->saddr = inet_ntoa(from.sin_addr);
				// Spin off a thread to handle this
				pthread_create(&tids[currThread],NULL,startSession,(void *) newSession);
				currThread += 1;
				if (currThread > TOTAL_SOCKETS-1){
					currThread = 0;
				}
				// Remove one from the number of sockets we're looking for
				rcvd -= 1;
				// If we've found them all, break out and go back to listening
				if (rcvd == 0){
					break;
				}
			}
		}
	}
	return 0;
}
