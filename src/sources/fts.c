// Main function for the File Transfer Service module
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "../headers/encoder.h"
#include "../headers/packetCrafter.h"
#include "../headers/sessionHandler.h"

#define TOTAL_SOCKETS 1001 // Total number of sockets you want to create, should be (high port) - (low port) + 1
#define LOW_PORT 16000	// Lowest port to use
#define MAX_SIZE 1450	// Maximum expected message size

void * transferSession(void * in);
int restartSocket(int index,int port);

pthread_mutex_t sockets_lock;
int * sockets;

int main(int argc, char ** argv){
	// Create an array to hold the file descriptors
	sockets = calloc(TOTAL_SOCKETS,sizeof(int));
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
			pthread_mutex_lock(&sockets_lock);
			int fd = sockets[i];
			pthread_mutex_unlock(&sockets_lock);
			if (fd != 0){
				FD_SET(fd,&fds);
			}
		}
		// Scan our sockets and until we get one that is ready
		int rcvd = select(maxfd+1,&fds,NULL,NULL,NULL);
		for (int x = 0;x < TOTAL_SOCKETS;x++){
			// Iterate through all of our sockets and see which one is ready
			pthread_mutex_lock(&sockets_lock);
			int socket = sockets[x];
			pthread_mutex_unlock(&sockets_lock);
			if (FD_ISSET(socket,&fds) && socket != 0){
				// Read from the socket
				struct sockaddr_in from;
				socklen_t fromLen = sizeof(from);
				int n = recvfrom(socket,dgram,MAX_SIZE,0,(struct sockaddr *) &from,&fromLen);
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
				newSession->size = n;
				newSession->sockNum = x;
				// Spin off a thread to handle this
				pthread_create(&tids[currThread],NULL,transferSession,(void *) newSession);
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

// Function called to send the type 0x01 message, receive the 0x02 message, send the 0x03 message, and conduct the TCP data transfer
void * transferSession(void * in){
        session * s = (session *) in;
        // Grab some admin info
        int ctwoPort = s->sport;
        int ftsPort = s->dport;
        char * ctwoAddr = s->saddr;
        int socketsIndex = s->sockNum;
	printf("%d\t%s\t%d\n",ctwoPort,ctwoAddr,ftsPort);
	// Close the socket that this packet came in on
	pthread_mutex_lock(&sockets_lock);
	int sock = sockets[socketsIndex];
	sockets[socketsIndex] = 0;
	pthread_mutex_unlock(&sockets_lock);
	shutdown(sock,SHUT_RDWR);
	close(sock);
	errno = 0;
        // Verify this is the right packet type
        if (s->packet[0] != 0){
                // Not packet type 0x00
		while (restartSocket(socketsIndex,ftsPort) == 0){
			continue;
		}
                return NULL;
        }
        // Scrape the address of the FSS
        char * fssAddr = calloc(16,sizeof(char));
        for (int i = 1;i < 5;i++){
                if (s->packet[i] < 0 || s->packet[i] > 255){
                        // Invalid octet
			while (restartSocket(socketsIndex,ftsPort) == 0){
				continue;
			}
                        return NULL;
                }
        }
        sprintf(fssAddr,"%d.%d.%d.%d",s->packet[1],s->packet[2],s->packet[3],s->packet[4]);
        if (strncmp(fssAddr,"0.0.0.0",strlen(fssAddr)) == 0 || strncmp(fssAddr,"255.255.255.255",strlen(fssAddr)) == 0){
                // Invalid IP address, won't send to broadcast
		while (restartSocket(socketsIndex,ftsPort) == 0){
			continue;
		}
                return NULL;
        }
        // Scrape FSS Port
        int fssPort = s->packet[6] | s->packet[5] << 8;
        if (fssPort < 1 || fssPort > 65535){
                // Invalid port number
		while (restartSocket(socketsIndex,ftsPort) == 0){
			continue;
		}
                return NULL;
        }
        // Scrape length of pattern and pattern
        int patternLength = s->packet[8] | s->packet[7] << 8;
        unsigned char * pattern = calloc(patternLength+1,sizeof(unsigned char));
        int i;
        for (i = 0;i < patternLength;i++){
                pattern[i] = s->packet[9+i];
        }
        Pattern * parsed = calloc(1,sizeof(Pattern));
        char * charPattern = calloc(patternLength,sizeof(char *));
        sprintf(charPattern,"%s",pattern);
        if (patternValidate(charPattern,parsed) != 0){
                // Invalid pattern
		while (restartSocket(socketsIndex,ftsPort) == 0){
			continue;
		}
                return NULL;
        }
        // Scrape initialization message
        int messageStart = i+9;
        unsigned char * message = calloc(s->size-(i+9),sizeof(unsigned char));
	int x;
        for (x = 0;x < s->size-messageStart;x++){
                message[x] = s->packet[messageStart+x];
        }
        printf("%s:%d\t%d\t%s\t%s\n",fssAddr,fssPort,messageStart,pattern,message);
        // craft Init
	unsigned char * init = calloc(1+x,sizeof(unsigned char));
	init[0] = '1';
	memcpy(init+1,message,x);
        // Send Init
	  // Create a new socket on this port
	  // necessary to avoid getting a bunch of different threads on the same port at the same time
	  // closing the port and taking it out of sockets[] keeps main from scanning it for activity
        int fssSocket = socket(AF_INET,SOCK_DGRAM,0);
        if (fssSocket == -1){
                // Failed to open
		while (restartSocket(socketsIndex,ftsPort) == 0){
			continue;
		}
                return NULL;
        }
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(ftsPort);
	addr.sin_addr.s_addr = INADDR_ANY;
        struct timeval t;
        t.tv_usec = TIMEOUT;
        if (setsockopt(fssSocket,SOL_SOCKET,SO_RCVTIMEO,(char *) &t,sizeof(t)) == -1){
                // Failed to set timeout
		close(fssSocket);
		while (restartSocket(socketsIndex,ftsPort) == 0){
			continue;
		}
                return NULL;
        }
        if (bind(fssSocket,(struct sockaddr *) &addr,sizeof(addr)) == -1){
                // Failed to bind
		close(fssSocket);
		while (restartSocket(socketsIndex,ftsPort) == 0){
			continue;
		}
                return NULL;
        }
	  // setup the destination information
	struct sockaddr_in dest;
	dest.sin_family = AF_INET;
	dest.sin_port = htons(fssPort);
	if (inet_aton(fssAddr,&(dest.sin_addr)) < 0){
		close(fssSocket);
		while(restartSocket(socketsIndex,ftsPort) == 0){
			continue;
		}
		return NULL;
	}
	 // send the packet
	int pktSize = (int) strlen((char *) init);
	while (sendto(fssSocket,init,pktSize,0,(struct sockaddr *) &dest,sizeof(dest)) < pktSize){
		// continue resending until we've sent everything, might introduce a weird bug on the FSS end if it sends some of the packet but not all of it
		continue;
	}
        // TODO receive 0x02, timeout causes thread to exit
        // TODO craft 0x03
        // TODO send 0x03
        // TODO start TCP handler
        close(fssSocket);
	// Remage the socket
	while (restartSocket(socketsIndex,ftsPort) == 0){
		continue;
	}
	printf("done\n");
        return NULL;
}

// Function to restart a socket once a thread has reached the end of its execution
// Returns 1 on success, 0 if error
// Duplicated outside of main to allow main to provide more verbose output
// This should only be called from a child thread, so getting output from this is not as important as getting it from main
int restartSocket(int index,int port){
	// Verify that inputs are within the acceptable range
	if (port < 1 || port > 65535){
		return 0;
	}
	if (index < 0 || index > TOTAL_SOCKETS-2){
		return 0;
	}
	// remake the file descriptor
	int fd = socket(AF_INET,SOCK_DGRAM,0);
	if (fd == -1){
		return 0;
	}
	// set timeout
	struct timeval t;
	t.tv_usec = TIMEOUT;
	if (setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,(char *) &t,sizeof(t)) == -1){
		return 0;
	}
	// bind
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;
	if (bind(fd,(struct sockaddr *) &addr,sizeof(addr)) == -1){
		return 0;
	}
	// add FD to sockets[]
	pthread_mutex_lock(&sockets_lock);
	sockets[index] = fd;
	pthread_mutex_unlock(&sockets_lock);
	return 1;
}
