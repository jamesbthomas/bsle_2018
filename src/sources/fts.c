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
#include "../headers/udpHandler.h"
#include "../headers/tcpHandler.h"

#define TOTAL_SOCKETS 1001 // Total number of sockets you want to create, should be (high port) - (low port) + 1
#define LOW_PORT 16000	// Lowest port to use
#define MAX_SIZE 1450	// Maximum expected message size
#define TIMEOUT 2000	// timeout for sockets

void * transferSession(void * in);
typedef struct Session{
	int sport;	// Port on the C2
	int dport;	// Port on the FTS
	unsigned char * packet;	// 0x00 packet contents
	char * saddr;	// C2 Address
	int size;	// Size of the packet
	int sockNum;	// Index of the FD for this session
} session;

pthread_mutex_t sockets_lock;
int * sockets;
fd_set fds;

int main(int argc, char ** argv){
	// Create an array to hold the file descriptors
	sockets = calloc(TOTAL_SOCKETS,sizeof(int));
	// Create the struct to hold the timeout
	struct timeval t;
	t.tv_usec = TIMEOUT;
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
		int rcvd = select(maxfd+1,&fds,NULL,NULL,&t);
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
	// Close the socket that this packet came in on
	pthread_mutex_lock(&sockets_lock);
	int sock = sockets[socketsIndex];
	sockets[socketsIndex] = 0;
	pthread_mutex_unlock(&sockets_lock);
        // Verify this is the right packet type
        if (s->packet[0] != 0){
                // Not packet type 0x00
		pthread_mutex_lock(&sockets_lock);
		sockets[socketsIndex] = sock;
		pthread_mutex_unlock(&sockets_lock);
                return NULL;
        }
        // Scrape the address of the FSS
        char * fssAddr = calloc(16,sizeof(char));
        for (int i = 1;i < 5;i++){
                if (s->packet[i] < 0 || s->packet[i] > 255){
                        // Invalid octet
			pthread_mutex_lock(&sockets_lock);
			sockets[socketsIndex] = sock;
			pthread_mutex_unlock(&sockets_lock);
                        return NULL;
                }
        }
        sprintf(fssAddr,"%d.%d.%d.%d",s->packet[1],s->packet[2],s->packet[3],s->packet[4]);
        if (strncmp(fssAddr,"0.0.0.0",strlen(fssAddr)) == 0 || strncmp(fssAddr,"255.255.255.255",strlen(fssAddr)) == 0){
                // Invalid IP address, won't send to broadcast
		pthread_mutex_lock(&sockets_lock);
		sockets[socketsIndex] = sock;
		pthread_mutex_unlock(&sockets_lock);
                return NULL;
        }
        // Scrape FSS Port
        int fssUDPPort = s->packet[6] | s->packet[5] << 8;
        if (fssUDPPort < 1 || fssUDPPort > 65535){
                // Invalid port number
		pthread_mutex_lock(&sockets_lock);
		sockets[socketsIndex] = sock;
		pthread_mutex_unlock(&sockets_lock);
                return NULL;
        }
        // Scrape length of pattern and pattern
        int patternLength = s->packet[8] | s->packet[7] << 8;
        unsigned char * pattern = calloc(patternLength+1,sizeof(unsigned char));
        int patternSize;
        for (patternSize = 0;patternSize < patternLength;patternSize++){
                pattern[patternSize] = s->packet[9+patternSize];
        }
        Pattern * parsed = calloc(1,sizeof(Pattern));
        char * charPattern = calloc(patternLength,sizeof(char *));
        sprintf(charPattern,"%s",pattern);
        if (patternValidate(charPattern,parsed) != 0){
                // Invalid pattern
		pthread_mutex_lock(&sockets_lock);
		sockets[socketsIndex] = sock;
		pthread_mutex_unlock(&sockets_lock);
                return NULL;
        }
        // Scrape initialization message
        int messageStart = patternSize+9;
        unsigned char * message = calloc(s->size-(patternSize+9),sizeof(unsigned char));
	int messageLen;
        for (messageLen = 0;messageLen < s->size-messageStart;messageLen++){
                message[messageLen] = s->packet[messageStart+messageLen];
        }
        // craft Init
	unsigned char * init = calloc(1+messageLen,sizeof(unsigned char));
	init[0] = '1';
	unsigned char * encoded = encode(message,parsed);
	memcpy(init+1,encoded,messageLen);
        // Send Init
	  // setup the destination information
	struct sockaddr_in fss;
	fss.sin_family = AF_INET;
	fss.sin_port = htons(fssUDPPort);
	if (inet_aton(fssAddr,&(fss.sin_addr)) < 0){
		// Failed to convert FSS Address
		pthread_mutex_lock(&sockets_lock);
		sockets[socketsIndex] = sock;
		pthread_mutex_unlock(&sockets_lock);
	}
	 // send the packet
	int initSize = (int) strlen((char *) init);
	while (sendto(sock,init,initSize,0,(struct sockaddr *) &fss,sizeof(fss)) < initSize){
		// continue resending until we've sent everything, might introduce a weird bug on the FSS end if it sends some of the packet but not all of it
		continue;
	}
        // Receive 0x02, timeout causes thread to exit
	unsigned char * response = calloc(MAX_SIZE,sizeof(unsigned char));
	struct sockaddr_in resp;
	socklen_t respLen = sizeof(resp);
	int fails = 0;	// keep track of how many packets come through that aren't responses
	// protection against someone trying to start a session, hitting this port, and causing the thread to exit before the 0x02 comes in
	while (fails < 3){
		int respSize = recvfrom(sock,response,MAX_SIZE,0,(struct sockaddr *) &resp,&respLen);
		if (respSize == 0 || resp.sin_family != AF_INET || ntohs(fss.sin_port) != ntohs(resp.sin_port) || inet_ntoa(fss.sin_addr) != inet_ntoa(resp.sin_addr) || response[0] != 2){
			// socket timed out, not from the right source, or not the right packet type
			response[0] = '\0';
			fails += 1;
		}
		else {
			break;
		}
	}
	if (fails == 3){
		// If the loop ended because it failed
		pthread_mutex_lock(&sockets_lock);
		sockets[socketsIndex] = sock;
		pthread_mutex_unlock(&sockets_lock);
		return NULL;
	}
	int fssTCPPort = response[1] << 8 | response[2];
	unsigned char * respMessage = calloc(messageLen,sizeof(unsigned char));
	for (int i = 0;i < messageLen;i++){
		respMessage[i] = response[i+3];
	}
	unsigned char * decoded = decode(respMessage,parsed);
	if (memcmp(decoded,message,messageLen) != 0){
		// Decode failure
		pthread_mutex_lock(&sockets_lock);
		sockets[socketsIndex] = sock;
		pthread_mutex_unlock(&sockets_lock);
		return NULL;
	}
        // Craft 0x03
	unsigned char * validation = calloc(3+messageLen,sizeof(unsigned char));
	validation[0] = 0x03;
	int ftsTCPPort = ftsPort + 1000;
	validation[1] = ftsTCPPort >> 8;
	validation[2] = ftsTCPPort & 0xff;
	memcpy(validation+3,decode(respMessage,parsed),messageLen);
        // Start TCP Sockets
	int tcpPort = ftsPort+TOTAL_SOCKETS-1; // TCP Ports begin immediately following the UDP port range, ie port 16000 UDP becomes 17000 TCP
	int ctwoListenSock = socket(AF_INET,SOCK_STREAM,0);
	if (ctwoListenSock < 0){
		perror("Socket Error: ");
		pthread_mutex_lock(&sockets_lock);
		sockets[socketsIndex] = sock;
		pthread_mutex_unlock(&sockets_lock);
		FD_SET(sock,&fds);
		return NULL;
	}
	int reuse = 1;
	if (setsockopt(ctwoListenSock,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(int)) < 0){
		perror("Socket Option Error");
	}
	struct sockaddr_in tcpAddr;
	tcpAddr.sin_family = AF_INET;
	tcpAddr.sin_port = htons(tcpPort);
	tcpAddr.sin_addr.s_addr = INADDR_ANY;
	if (bind(ctwoListenSock,(struct sockaddr *) &tcpAddr,sizeof(tcpAddr)) < 0){
		perror("Bind Error: ");
	}
	if (listen(ctwoListenSock,1) < 0){
		perror("Listen Error: ");
	}
        // Send 0x03
	struct sockaddr_in ctwo;
	ctwo.sin_family = AF_INET;
	ctwo.sin_port = htons(ctwoPort);
	if (inet_aton(ctwoAddr,&(ctwo.sin_addr)) < 0){
		// Failed to convert C2 address
		pthread_mutex_lock(&sockets_lock);
		sockets[socketsIndex] = sock;
		pthread_mutex_unlock(&sockets_lock);
	}
	while (sendto(sock,validation,3+messageLen,0,(struct sockaddr *) &ctwo,sizeof(ctwo)) < 3+messageLen){
		continue;
	}
	// Accept the connection from the C2
	struct sockaddr_in incoming;
	socklen_t incSize = sizeof(incoming);
	int ctwoSock = accept(ctwoListenSock,(struct sockaddr *) &incoming,&incSize);
	// TODO make sure its from the right IP
	if (ctwoSock < 0){
		perror("Handshake Error: ");
		pthread_mutex_lock(&sockets_lock);
		sockets[socketsIndex] = sock;
		pthread_mutex_unlock(&sockets_lock);
		FD_SET(sock,&fds);
	}
	// Write the data to a temporary file
	char * data = calloc(MAX_SIZE,sizeof(unsigned char));
	char * tmpfile = calloc(11,sizeof(char));
	sprintf(tmpfile,"/tmp/%d",tcpPort);
	int rcvd;
	int totalrcvd = 0;
	FILE * temp = fopen(tmpfile,"w");
	while ( (rcvd = read(ctwoSock,data,MAX_SIZE)) > 0){
		int res = fputs(data,temp);
		if (res != 1){
			perror("Write Error");
		}
		totalrcvd += rcvd;

		memset(&data[0],0,strlen(data));
	}
	// Close the incoming connection
	close(ctwoSock);
	shutdown(ctwoListenSock,SHUT_RDWR);
	// Connect to the FSS
	struct sockaddr_in fssTCP;
	fssTCP.sin_family = AF_INET;
	fssTCP.sin_port = htons(fssTCPPort);
	if (inet_aton(fssAddr,&(fssTCP.sin_addr)) < 0){
		perror("FSS TCP ADDR");
	}
	if (connect(ctwoListenSock,(struct sockaddr *) &fssTCP,sizeof(fssTCP)) < 0){
		perror("Connect");
	}
	fclose(temp);
	// Send the File
	FILE * tmp = fopen(tmpfile,"rb");
	unsigned char * raw = calloc(MAX_SIZE,sizeof(unsigned char));
	int sent = 0;
	while (sent < totalrcvd){
		int read = fread(raw,sizeof(unsigned char),MAX_SIZE,tmp);
		int pktSize = send(ctwoListenSock,encode(raw,parsed),read,0);
		if (pktSize != read){
			perror("Send Error");
		}
		sent += pktSize;
		memset(&raw[0],0,read);
	}
	close(ctwoListenSock);
	pthread_mutex_lock(&sockets_lock);
	sockets[socketsIndex] = sock;
	pthread_mutex_unlock(&sockets_lock);
	FD_SET(sock,&fds);
        return NULL;
}
