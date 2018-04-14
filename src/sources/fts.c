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
#include <math.h>
#include <linux/limits.h>
#include "../headers/encoder.h"
#include "../headers/udpHandler.h"
#include "../headers/tcpHandler.h"

#define TOTAL_SOCKETS 1001 // Total number of sockets you want to create, should be (high port) - (low port) + 1
#define MAX_SIZE 1450	// Maximum expected message size
#define TIMEOUT 9000	// timeout for sockets
#define MAX_CMD 100	// max size of a cmd prompt command

void * transferSession(void * in);
void * listener(void * in);
typedef struct Session{
	int sport;	// Port on the C2
	int dport;	// Port on the FTS
	unsigned char * packet;	// 0x00 packet contents
	char * saddr;	// C2 Address
	int size;	// Size of the packet
	int sockNum;	// Index of the FD for this session
} session;
void threadClose(int socketsIndex,int sock);

pthread_mutex_t sockets_lock;
int * sockets;
pthread_mutex_t log_lock;
char * logfile;
pthread_t * tids;
fd_set fds;

pthread_mutex_t numthrd_lock;
int numthrds;

pthread_mutex_t ready_lock;
int ready;

int main(int argc, char ** argv){
	printf("Starting FTS module...\n");
	pthread_mutex_lock(&ready_lock);
	ready = 0;
	pthread_mutex_unlock(&ready_lock);
	// Create an array to hold the file descriptors
	sockets = calloc(TOTAL_SOCKETS,sizeof(int));
	// Find the path to the log file
	logfile = calloc(PATH_MAX,sizeof(char));
	char * path = calloc(PATH_MAX,sizeof(char));
	memset(path,0,strlen(path));
	pid_t pid = getpid();
	sprintf(path,"/proc/%d/exe",pid);
	if (readlink(path,logfile,PATH_MAX) == -1){
		perror("Read Link");
		exit(2);
	}
	time_t st = time(0);
	struct tm* tm_info = localtime(&st);
	char time_str[30];
	strftime(time_str,30,"ftsLogs/fts_%d-%m-%y.log",tm_info);
	memcpy(logfile+strlen(logfile)-3,time_str,strlen(time_str));
	// Create the file
	FILE * log = fopen(logfile,"a");
	fclose(log);
	// Create an array to track the thread IDs
	tids = calloc(TOTAL_SOCKETS,sizeof(pthread_t));
	pthread_mutex_lock(&numthrd_lock);
	numthrds = 1;
	pthread_mutex_unlock(&numthrd_lock);
	pthread_create(&tids[0],NULL,listener,NULL);
	while (!ready){
		continue;
	}
	char * cmd = calloc(MAX_CMD,sizeof(char));
	while (1){
		fprintf(stdout,"fts> ");
		if (fgets(cmd,MAX_CMD,stdin) == NULL){
			continue;
		}
		else if (strncmp(cmd,"log ",4) == 0){
			printf(" -- Prototype for reading the log for specific parameters\n");
		}
		else if (strncmp(cmd,"head ",5) == 0){
			char * line = calloc(256,sizeof(unsigned char));
			int total = 0;
			for (int i = strlen(cmd)-2;i > 4;i--){
				if (i == strlen(cmd)-2){
					total += cmd[i]-'0';
				}
				else {
					total += (cmd[i]-'0')*((int) pow(10,(((int) strlen(cmd)-2)-i)));
				}
			}
			FILE * readLog = fopen(logfile,"rb");
			pthread_mutex_lock(&log_lock);
			while (total > 0){
				fgets(line,256,readLog);
				if (feof(readLog)){
					total = 0;
					printf("<< END OF LOG >>\n");
					continue;
				}
				line[strcspn(line,"\n")] = '\0';
				unsigned char * dline = decode64((unsigned char *) line);
				printf("%s\n",dline);
				free(dline);
				total -= 1;
			}
			pthread_mutex_unlock(&log_lock);
			fclose(readLog);
			free(line);
		}
		else if (strncmp(cmd,"exit\n",5) == 0){
			printf("Closing threads . . . ");
			pthread_mutex_lock(&ready_lock);
			ready = 0;
			pthread_mutex_unlock(&ready_lock);
			pthread_mutex_lock(&numthrd_lock);
			for (int i = 0;i < numthrds;i++){
				pthread_join(tids[i],NULL);
			}
			pthread_mutex_unlock(&numthrd_lock);
			printf("done!\n");
			free(tids);
			free(logfile);
			free(path);
			free(sockets);
			free(cmd);
			exit(0);
		}
		else if (strncmp(cmd,"status",6) == 0){
			pthread_mutex_lock(&numthrd_lock);
			printf("Active Connections - %d\n",numthrds-1);
			pthread_mutex_unlock(&numthrd_lock);
			pthread_mutex_lock(&ready_lock);
			if (!ready){
				printf("WARNING - Listener not active\n");
			}
			else {
				printf("Listener is active\n");
			}
			pthread_mutex_unlock(&ready_lock);
		}
		else if (strncmp(cmd,"\n",1) == 0){
			continue;
		}
		else {
			printf("Unrecognized Command - %s",cmd);
		}
	}
}

// Function called to manage the UDP Listeners and allow main to take commands indepedent of the operation of the server
void * listener(void * in){
	// Create the struct to hold the timeout
	struct timeval t;
	memset(&t,0,sizeof(t));
	t.tv_usec = TIMEOUT;
	// For each socket we want to create
	for (int i = 0;i < TOTAL_SOCKETS;i++){
		int s = makeSocket(i,&t);
		if (s == -1){
			perror("Listener - Make Failure");
			return NULL;
		}
		sockets[i] = s;
	}
	// track the last created file descriptor
	int maxfd = sockets[TOTAL_SOCKETS-1];
	pthread_mutex_lock(&ready_lock);
	ready = 1;
	pthread_mutex_unlock(&ready_lock);
	printf("Ready!\n");
	while (ready) {
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
				// Allocate a buffer to hold the message
				unsigned char * dgram = calloc(MAX_SIZE,sizeof(unsigned char));
				// Read from the socket
				struct sockaddr_in from;
				socklen_t fromLen = sizeof(from);
				int n = recvfrom(socket,dgram,MAX_SIZE,0,(struct sockaddr *) &from,&fromLen);
				// Catch any strange receive bugs
				if (n < 1){
					continue;
				}
				// Grab info from the packet
				struct sockaddr_in s;
				socklen_t len = sizeof(s);
				getsockname(sockets[x],(struct sockaddr *) &s,&len);
				session * newSession = calloc(1,sizeof(session));
				newSession->sport = ntohs(from.sin_port);
				newSession->dport = ntohs(s.sin_port);
				newSession->packet = calloc(n,sizeof(unsigned char));
				memcpy(newSession->packet,dgram,n);
				newSession->saddr = inet_ntoa(from.sin_addr);
				newSession->size = n;
				newSession->sockNum = x;
				free(dgram);
				// Spin off a thread to handle this
				pthread_mutex_lock(&numthrd_lock);
				// use x for the thread id, guaranteed to be available because of how sockets[] works and is always made available again once the thread has ended
				pthread_create(&tids[x],NULL,transferSession,(void *) newSession);
//				pthread_detach(tids[numthrds]);
				numthrds += 1;
				pthread_mutex_unlock(&numthrd_lock);
				// Remove one from the number of sockets we're looking for
				rcvd -= 1;
				// If we've found them all, break out and go back to listening
				if (rcvd == 0){
					break;
				}
			}
		}
	}
	return NULL;
}

// Function called to send the type 0x01 message, receive the 0x02 message, send the 0x03 message, and conduct the TCP data transfer
void * transferSession(void * in){
	pthread_mutex_lock(&numthrd_lock);
	numthrds += 1;
	pthread_mutex_unlock(&numthrd_lock);
	// Grab the start time for the log
	time_t start = time(0);
	char * start_str = ctime(&start);
	start_str[strlen(start_str)-1] = '\0';
        session * s = (session *) in;
        // Grab some admin info
        int ctwoPort = s->sport;
        int ftsPort = s->dport;
        char * ctwoAddr = s->saddr;
        int socketsIndex = s->sockNum;
	int totalBytes = s->size;
	int size = s->size;
	// Take this socket off of the list of listeners
	pthread_mutex_lock(&sockets_lock);
	int sock = sockets[socketsIndex];
	sockets[socketsIndex] = 0;
	pthread_mutex_unlock(&sockets_lock);
        // Verify this is the right packet type
        if (s->packet[0] != 0){
                // Not packet type 0x00
		printf("Error: Invalid Packet Type\n");
		free(s->packet);
		free(s);
		threadClose(socketsIndex,sock);
                return NULL;
        }
        // Scrape the address of the FSS
        char * fssAddr = calloc(16,sizeof(char));
	if (scrapeAddr(fssAddr,s->packet)){
		printf("Error: Invalid IP Address\n");
		free(s->packet);
		free(s);
		free(fssAddr);
		threadClose(socketsIndex,sock);
		return NULL;
	}
        // Scrape FSS Port
        int fssUDPPort = scrapePort(s->packet);
        if (fssUDPPort == -1){
                // Invalid port number
		printf("Error: Invalid Port Number\n");
		free(fssAddr);
		free(s->packet);
		free(s);
		threadClose(socketsIndex,sock);
                return NULL;
        }
        // Scrape length of pattern and pattern
        Pattern * parsed = calloc(1,sizeof(Pattern));
	int patternLen = scrapePattern(parsed,s->packet);
        if (patternLen == -1){
                // Invalid pattern
		printf("Error: Invalid Pattern\n");
		free(fssAddr);
		free(parsed);
		free(s->packet);
		free(s);
		threadClose(socketsIndex,sock);
                return NULL;
        }
        // Scrape initialization message and craft init
	unsigned char * message = calloc(size-(patternLen+8),sizeof(unsigned char));
	unsigned char * init = calloc(1+(size-(patternLen+9)),sizeof(unsigned char));
	int messageLen = scrapeMessage(message,s->packet,patternLen+9,size,parsed,init);
	free(s->packet);
        // Send Init
	  // setup the destination information
	struct sockaddr_in fss;
	fss.sin_family = AF_INET;
	fss.sin_port = htons(fssUDPPort);
	if (inet_aton(fssAddr,&(fss.sin_addr)) < 0){
		perror("Convert Address Failure");
		// Failed to convert FSS Address
		free(s);
		free(fssAddr);
		free(parsed);
		free(init);
		free(message);
		threadClose(socketsIndex,sock);
		return NULL;
	}
	 // send the packet
	int initSize = size-patternLen-9+1;
	free(s);
	totalBytes += initSize;
	while (sendto(sock,init,initSize,0,(struct sockaddr *) &fss,sizeof(fss)) < initSize){
		// continue resending until we've sent everything, might introduce a weird bug on the FSS end if it sends some of the packet but not all of it
		continue;
	}
	// free init
	free(init);
        // Receive 0x02, timeout causes thread to exit
	unsigned char * response = calloc(MAX_SIZE,sizeof(unsigned char));
	struct sockaddr_in resp;
	memset(&resp,0,sizeof(resp));
	socklen_t respLen = sizeof(resp);
	int fssTCPPort;
	unsigned char * decoded = calloc(messageLen,sizeof(unsigned char));
	int fails = 0;	// keep track of how many packets come through that aren't responses
	// protection against someone trying to start a session, hitting this port, and causing the thread to exit before the 0x02 comes in
	struct sockaddr_in ctwo;
	ctwo.sin_family = AF_INET;
	ctwo.sin_port = htons(ctwoPort);
	if (inet_aton(ctwoAddr,&(ctwo.sin_addr)) < 0){
		// Failed to convert C2 address
		perror("C2 Address Conversion Failure");
		free(decoded);
		free(fssAddr);
		free(parsed->opts);
		free(parsed->ops);
		free(parsed->vals);
		free(parsed->lens);
		free(parsed);
		threadClose(socketsIndex,sock);
		return NULL;
	}
	// Get the response
	while (fails < 3){
		int respSize = recvfrom(sock,response,MAX_SIZE,0,(struct sockaddr *) &resp,&respLen);
		if (respSize == 0 || resp.sin_family != AF_INET || ntohs(fss.sin_port) != ntohs(resp.sin_port) || inet_ntoa(fss.sin_addr) != inet_ntoa(resp.sin_addr)){
			// socket timed out, not from the right source, or not the right packet type
			memset(response,0,MAX_SIZE);
			fails += 1;
		}
		else {
			if (response[0] == 0x04){
				sendto(sock,response,1,0,(struct sockaddr *) &ctwo,sizeof(ctwo));
				free(decoded);
				free(fssAddr);
				free(parsed->opts);
				free(parsed->ops);
				free(parsed->vals);
				free(parsed->lens);
				free(parsed);
				free(response);
				free(message);
				threadClose(socketsIndex,sock);
				return NULL;
			}
			fssTCPPort = unpackResponse(response,parsed,message,decoded,messageLen);
			if (fssTCPPort != -1){
				totalBytes += strlen((char *) response);
				break;
			}
			fails += 1;
		}
	}
	if (fails == 3){
		// If the loop ended because it failed
		printf("Error: Timeout waiting for response\n");
		free(decoded);
		free(fssAddr);
		free(parsed->opts);
		free(parsed->ops);
		free(parsed->vals);
		free(parsed->lens);
		free(parsed);
		free(message);
		free(response);
		threadClose(socketsIndex,sock);
		return NULL;
	}
	free(response);
	free(message);
        // Craft 0x03
	unsigned char * validation = calloc(3+messageLen,sizeof(unsigned char));
	int tcpPort = ftsPort+TOTAL_SOCKETS-1; // TCP Ports begin immediately following the UDP port range, ie port 16000 UDP becomes 17000 TCP
	craftValidation(validation,decoded,tcpPort,messageLen);
	free(decoded);
        // Start TCP Socket
	int ctwoListenSock = makeTCPSocket(tcpPort);
	if (ctwoListenSock < 0){
		free(fssAddr);
		free(parsed->opts);
		free(parsed->ops);
		free(parsed->vals);
		free(parsed->lens);
		free(parsed);
		free(validation);
		close(ctwoListenSock);
		threadClose(socketsIndex,sock);
		return NULL;
	}
        // Send 0x03
	while (sendto(sock,validation,3+messageLen,0,(struct sockaddr *) &ctwo,sizeof(ctwo)) < 3+messageLen){
		continue;
	}
	totalBytes += 3+messageLen;
	free(validation);
	// Accept the connection from the C2
	struct sockaddr_in incoming;
	socklen_t incSize = sizeof(incoming);
	int ctwoSock = accept(ctwoListenSock,(struct sockaddr *) &incoming,&incSize);
	// TODO make sure its from the right IP
	if (ctwoSock < 0){
		perror("Handshake Error: ");
		free(fssAddr);
		free(parsed->opts);
		free(parsed->ops);
		free(parsed->vals);
		free(parsed->lens);
		free(parsed);
		close(ctwoListenSock);
		threadClose(socketsIndex,sock);
		return NULL;
	}
	// Write the data to a temporary file
	char * data = calloc(MAX_SIZE+1,sizeof(unsigned char));
	char * tmpfile = calloc(11,sizeof(char));
	sprintf(tmpfile,"/tmp/%d",tcpPort);
	int rcvd;
	int totalrcvd = 0;
	FILE * temp = fopen(tmpfile,"w");
	while ( (rcvd = read(ctwoSock,data,MAX_SIZE)) > 0){
		data[MAX_SIZE] = '\0';
		int res = fputs(data,temp);
		if (res != 1){
			perror("Write Error");
			free(fssAddr);
			free(parsed->opts);
			free(parsed->ops);
			free(parsed->vals);
			free(parsed->lens);
			free(parsed);
			free(data);
			free(tmpfile);
			fclose(temp);
			close(ctwoListenSock);
			threadClose(socketsIndex,sock);
			return NULL;
		}
		totalrcvd += rcvd;
		memset(&data[0],0,strlen(data));
	}
	free(data);
	// Close the incoming connection
	close(ctwoSock);
	shutdown(ctwoListenSock,SHUT_RDWR);
	// Connect to the FSS
	struct sockaddr_in fssTCP;
	fssTCP.sin_family = AF_INET;
	fssTCP.sin_port = htons(fssTCPPort);
	if (inet_aton(fssAddr,&(fssTCP.sin_addr)) < 0){
		perror("FSS TCP ADDR");
		free(fssAddr);
		free(parsed->opts);
		free(parsed->ops);
		free(parsed->vals);
		free(parsed->lens);
		free(parsed);
		free(tmpfile);
		fclose(temp);
		close(ctwoListenSock);
		threadClose(socketsIndex,sock);
		return NULL;
	}
	if (connect(ctwoListenSock,(struct sockaddr *) &fssTCP,sizeof(fssTCP)) < 0){
		perror("Connect");
		free(parsed->opts);
		free(parsed->ops);
		free(parsed->vals);
		free(parsed->lens);
		free(parsed);
		free(tmpfile);
		fclose(temp);
		close(ctwoListenSock);
		threadClose(socketsIndex,sock);
		return NULL;
	}
	fclose(temp);
	// Send the File
	FILE * tmp = fopen(tmpfile,"rb");
	unsigned char * raw = calloc(MAX_SIZE+1,sizeof(unsigned char));
	int sent = 0;
	while (sent < totalrcvd){
		int read = fread(raw,sizeof(unsigned char),MAX_SIZE,tmp);
		unsigned char * ncoded = encode(raw,parsed);
		int pktSize = send(ctwoListenSock,ncoded,read,0);
		free(ncoded);
		if (pktSize != read){
			perror("Send Error");
			free(parsed->opts);
			free(parsed->ops);
			free(parsed->vals);
			free(parsed->lens);
			free(parsed);
			free(tmpfile);
			fclose(tmp);
			close(ctwoListenSock);
			threadClose(socketsIndex,sock);
			return NULL;
		}
		sent += pktSize;
		memset(&raw[0],0,read);
	}
	totalBytes += sent;
	free(raw);
	free(parsed->opts);
	free(parsed->ops);
	free(parsed->vals);
	free(parsed->lens);
	free(parsed);
	free(tmpfile);
	fclose(tmp);
	close(ctwoListenSock);
	threadClose(socketsIndex,sock);
	char * entry = calloc(256,sizeof(char));
	snprintf(entry,256,"%s:%s->%s:%d:Success\n",start_str,ctwoAddr,fssAddr,totalBytes);
	pthread_mutex_lock(&log_lock);
	FILE * log = fopen(logfile,"a");
	char * encEntry = (char *) encode64((unsigned char *) entry);
	fputs(encEntry,log);
	free(entry);
	free(encEntry);
	fclose(log);
	pthread_mutex_unlock(&log_lock);
	free(fssAddr);
	pthread_mutex_lock(&numthrd_lock);
	numthrds -= 1;
	pthread_mutex_unlock(&numthrd_lock);
        return NULL;
}

// Function to handle thread clean up in the event a session handler thread needs to close
// Takes a socket file descriptor and that socket's index within the array of UDP listeners
void threadClose(int socketsIndex, int sock){
	pthread_mutex_lock(&sockets_lock);
	sockets[socketsIndex] = sock;
	pthread_mutex_unlock(&sockets_lock);
	pthread_mutex_lock(&numthrd_lock);
	numthrds -= 1;
	pthread_mutex_unlock(&numthrd_lock);
	FD_SET(sock,&fds);
}
