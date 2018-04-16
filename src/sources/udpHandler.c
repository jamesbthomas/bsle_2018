// C Source File for Function Definitions of the UDPHandler Module
#ifndef UDP_HEADER
#define UDP_HEADER
#include "../headers/udpHandler.h"
#include "../headers/encoder.h"
#include <sys/socket.h>
#include <sys/time.h>
#include <errno.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define LOW_PORT 16000 // Lowest port to use

// Creates a UDP socket with the specified timeout on the port derived from the socket number
// Returns the file descriptor on success, -1 on error
int makeSocket(int socketNum,struct timeval * t){
	// Create the file descriptor
	int fd = socket(AF_INET,SOCK_DGRAM,0);
	// Verify
	if (fd == -1){
		perror("Socket Create Failure");
		return -1;
	}
	// Calculate the port
	int port = socketNum + LOW_PORT;
	// Set the timeout
	if (setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,t,sizeof(*t)) == -1){
		perror("Failed to Apply Timeout");
		return -1;
	}
	// Bind
	struct sockaddr_in addr;
	memset(&addr,0,sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;
	if (bind(fd,(struct sockaddr *) &addr,sizeof(addr)) == -1){
		perror("Bind Failure");
		return -1;
	}
	// Return the file descriptor
	return fd;
}

// Scrapes the FSS address out of the pkt and stores it in addr
// Returns 0 on success, 1 otherwise
int scrapeAddr(char * addr,unsigned char * pkt){
	// pull out the octets
	for (int i = 1;i < 5;i++){
		// verify that each octet is within the acceptable range
		if (pkt[i] < 0 || pkt[i] > 255){
			return 1;
		}
	}
	sprintf(addr,"%d.%d.%d.%d",pkt[1],pkt[2],pkt[3],pkt[4]);
	if (strncmp(addr,"0.0.0.0",strlen(addr)) == 0 || strncmp(addr,"255.255.255.255",strlen(addr)) == 0){
		// verify that the client isnt trying to send to a broadcast address
		return 1;
	}
	return 0;
}

// Scrapes the FSS Port out of the pkt and returns it
// Returns -1 on error
int scrapePort(unsigned char * pkt){
	int port = pkt[6] | pkt[5] << 8;
	if (port < 1 || port > 65535){
		return -1;
	}
	return port;
}

// Scrapes the pattern length and validates the encoding pattern, storing it in parsed
// Returns the length of the pattern on success, or -1 on error
int scrapePattern(Pattern * parsed, unsigned char * pkt){
	int len = pkt[7] << 8 | pkt[8];
	unsigned char * pattern = calloc(len+1,sizeof(unsigned char));
	memcpy(pattern,pkt+9,len);
	char * cpattern = (char *) pattern;
	if (patternValidate(cpattern,parsed) != 0){
		free(pattern);
		return -1;
	}
	free(pattern);
	return len;
}

// Scrapes the message out of the packet and stores it in the provided pointer
// Also scrapes the filename before crafting the Init message and storing it in the provided pointer
// Returns the length of the message
int scrapeMessage(unsigned char * message,unsigned char * pkt,int start,int size, Pattern * parsed,unsigned char * init){
	// Get the Message
	int len = pkt[start+1] | pkt[start] << 8;
	memcpy(message,pkt+2+start,len);
	// Grab the file name
	int fnameLen = size-len-start-2;
	unsigned char * filename = calloc(fnameLen,sizeof(unsigned char));
	memcpy(filename,pkt+len+start+2,fnameLen);
	// Craft init
	 // Packet type 0x01
	init[0] = 0x01;
	 // encode the message
	unsigned char * encoded = encode(message,parsed);
	 // copy the message length into the packet
	init[1] = len >> 8;
	init[2] = len & 0xff;
	 // copy the message into the packet
	memcpy(init+3,encoded,len);
	 // copy the filename into the packet
	memcpy(init+3+len,filename,fnameLen);
	// free
	free(encoded);
	free(filename);
	return len;
}

// Unpacks the Response and validates the input
// Returns the TCP Port for the transfer on success, or -1 on error
int unpackResponse(unsigned char * pkt,Pattern * parsed,unsigned char * message,unsigned char * decoded,int len){
	if (pkt[0] != 0x02){
		return -1;
	}
	int port = pkt[2] | pkt[1] << 8;
	if (port < 1 || port > 65535){
		return -1;
	}
	unsigned char * responseMsg = calloc(len+1,sizeof(unsigned char));
	memcpy(responseMsg,pkt+3,len);
	responseMsg[len] = '\0';
	unsigned char * dcoded = decode(responseMsg,parsed);
	memcpy(decoded,dcoded,len);
	free(dcoded);
	if (memcmp(decoded,message,len) != 0){
		free(responseMsg);
		return -1;
	}
	free(responseMsg);
	return port;
}

#endif
