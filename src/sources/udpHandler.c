// C Source file for the functions involved in handling the initial UDP communication between the C2 and FSS
#ifndef UDP_HEADER
#define UDP_HEADER
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include "../headers/udpHandler.h"

// Struct that holds all of the information pertaining to a session at initialization
// Used passed to startSession at thread create time
typedef struct Session{
	int sport;
	int dport;
	unsigned char * packet;
	char * saddr;
} session;

// Function called to send the type 0x01 message, receive the 0x02 message, send the 0x03 message, and start listening on the TCP port
void * startSession(void * in){
	session * s = (session *) in;
	if (s->packet[0] != 0){
		// Not packet type 0x00
		return NULL;
	}
	// Scrape the address of the FSS
	char * fssAddr = calloc(16,sizeof(char));
	sprintf(fssAddr,"%d.%d.%d.%d",s->packet[1],s->packet[2],s->packet[3],s->packet[4]);
	// TODO validate FSS address
	// Scrape FSS Port
	int fssPort = s->packet[5] | s->packet[6] << 8;
	printf("%s:%d\n",fssAddr,fssPort);
	return NULL;
}

// Function for interacting with this module directly
/*
int main(void){
	pthread_t * tids = calloc(3,sizeof(pthread_t));
	for (int i = 0;i < 3;i++){
		session * s = calloc(1,sizeof(session));
		s->sport = i;
		s->dport = 65535;
		s->packet = "test packet";
		s->saddr = "127.0.0.1";
		pthread_create(&tids[i],NULL,startSession,(void *) s);
	}
	printf("reaping\n");
	for (int i = 0;i < 3;i++){
		pthread_join(tids[i],NULL);
	}
	return 0;
}*/

#endif
