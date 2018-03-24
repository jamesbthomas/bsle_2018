// C Source file for child threads off the main FTS process
// Handles all aspects of a transfer session excluding receiving the type 0x00 packet
#ifndef SESSION_HEADER
#define SESSION_HEADER
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "../headers/encoder.h"
#include "../headers/packetCrafter.h"
#include "../headers/sessionHandler.h"

#define TIMEOUT 2000 // Timeout for sockets

// Struct that holds all of the information pertaining to a session at initialization
// Used passed to startSession at thread create time
typedef struct Session{
	int sport;
	int dport;
	unsigned char * packet;
	char * saddr;
	int size;
	int sockNum;
} session;

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
