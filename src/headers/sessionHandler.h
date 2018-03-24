// C Header file for the Session Handler FTS Module
#ifndef SESSION_HEADER
#define SESSION_HEADER
#define TIMEOUT 3000 // Timeout for sockets
#define LOW_PORT 16000 // Lowest port to use
#define MAX_SIZE 1450	// Maximum expected message size
#define TOTAL_SOCKETS 1001	//Total number of sockets to create

typedef struct Session{
	int sport;
	int dport;
	unsigned char * packet;
	char * saddr;
	int size;
	int sockNum;
} session;

void * transferSession(void * in);

#endif
