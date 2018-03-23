// C Header file for the UDP Handler Module
#ifndef UDP_HEADER
#define UDP_HEADER

typedef struct Session{
	int sport;
	int dport;
	unsigned char * packet;
	char * saddr;
} session;

void * startSession(void * in);

#endif
