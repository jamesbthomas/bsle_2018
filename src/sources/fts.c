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

#define TOTAL_SOCKETS 1001 	// Total number of sockets you want to create, should be (high port) - (low port) + 1
#define MAX_SIZE 1450		// Maximum expected message size
#define TIMEOUT 9000		// timeout for sockets
#define MAX_CMD 100		// max size of a cmd prompt command
#define MAX_LOG_ENTRY 512	// max size of a log entry

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
void threadClose(int socketsIndex,int sock,Pattern * parsed);
void freePattern(Pattern * parsed);

int reverseRead(int total);

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
	// Initialize the ready variable
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
	tids = calloc(TOTAL_SOCKETS+1,sizeof(pthread_t));
	pthread_mutex_lock(&numthrd_lock);
	numthrds = 1;
	pthread_mutex_unlock(&numthrd_lock);
	// Create the listener thread
	pthread_create(&tids[0],NULL,listener,NULL);
	  // Detach the listener so that it can release its own resources
	pthread_detach(tids[0]);
	// Wait until the listener reports that it is ready
	int status = 0;
	while (!status){
		pthread_mutex_lock(&ready_lock);
		status = ready;
		pthread_mutex_unlock(&ready_lock);
	}
	// Allocate space to store commands
	char * cmd = calloc(MAX_CMD,sizeof(char));
	// Endless loop to enable the command prompt
	while (1){
		// Receive commands
		fprintf(stdout,"fts> ");
		if (fgets(cmd,MAX_CMD,stdin) == NULL){
			continue;
		}
		// Search the log
		else if (strncmp(cmd,"search ",7) == 0){
			char * data = NULL;
			// Capture the search term from the string and null terminate it
			char * term = calloc(strlen(cmd)-7,sizeof(char *));
			memcpy(term,cmd+7,strlen(cmd)-7);
			term[strlen(term)-1] = '\0';
			int read = 0;
			size_t len = 0;
			pthread_mutex_lock(&log_lock);
			// Read the file line by line
			FILE * log = fopen(logfile,"rb");
			while ((read = getline(&data,&len,log)) != -1){
				data[read-1] = '\0';
				unsigned char * dline = decode64((unsigned char *) data);
				// If the decoded line contains the search term, print it
				if (strstr((char *) dline,term) != NULL){
					printf("%s\n",dline);
				}
				free(dline);
			}
			pthread_mutex_unlock(&log_lock);
			fclose(log);
			free(data);
			free(term);
		}
		// Read the log
		else if (strncmp(cmd,"head ",5) == 0){
			// pick out the number of lines we need to read
			int total = 0;
			for (int i = strlen(cmd)-2;i > 4;i--){
				// if this is the first iteration
				if (i == strlen(cmd)-2){
					total += cmd[i]-'0';
				}
				else {
					total += (cmd[i]-'0')*((int) pow(10,(((int) strlen(cmd)-2)-i)));
				}
			}
			reverseRead(total);
		}
		// Close the server
		else if (strncmp(cmd,"exit\n",5) == 0){
			printf("Closing threads . . . \n");
			// set ready to 0, telling the listener it needs to start closing
			pthread_mutex_lock(&ready_lock);
			ready = 0;
			pthread_mutex_unlock(&ready_lock);
			// Join the listener
			printf("Joining Listener . . .\n");
			pthread_join(tids[0],NULL);
			// suuuuuper weird race condition on the join above, itll leak some of the memory allocated by the listener's pthread_create without a break
			sleep(1);
			pthread_mutex_unlock(&numthrd_lock);
			// Join all of the session threads
			pthread_mutex_lock(&numthrd_lock);
			printf("Joining remaining connections . . .\n");
			for (int i = 1;i < TOTAL_SOCKETS+1;i++){
				pthread_join(tids[i],NULL);
			}
			printf(" . . . done!\nBye!\n");
			// Free resources
			free(tids);
			free(logfile);
			free(path);
			free(sockets);
			free(cmd);
			exit(0);
		}
		// Print the status
		else if (strncmp(cmd,"status",6) == 0){
			// Get the number of active sessions
			pthread_mutex_lock(&numthrd_lock);
			printf("Active Connections - %d\n",numthrds-1);
			pthread_mutex_unlock(&numthrd_lock);
			// Capture the status of the listener
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
			// Catch blank lines
			continue;
		}
		// Print he help menu
		else if (strncmp(cmd,"help",4) == 0){
			// Print the help menu
			printf("FTS Help Menu\n");
			printf(" - search <term> \tread the log and return only those entries that contain <term>\n");
			printf(" - head <num> \tread the <num> most recent entries in the log\n");
			printf(" - status \tprint information on the current status of the server\n");
			printf(" - exit \tjoin all child threads, clean up, and close the server\n");
		}
		else {
			// Catch unrecognized commands
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
			pthread_detach(pthread_self());
			return NULL;
		}
		sockets[i] = s;
	}
	// track the last created file descriptor
	int maxfd = sockets[TOTAL_SOCKETS-1];
	// Tell the main thread that its ready to start listening
	pthread_mutex_lock(&ready_lock);
	ready = 1;
	pthread_mutex_unlock(&ready_lock);
	printf("Ready!\n");
	// loop for as long as main wants us to stay up
	int status = 1;
	while (status) {
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
		// Check if main wants us to close
		pthread_mutex_lock(&ready_lock);
		status = ready;
		pthread_mutex_unlock(&ready_lock);
	}
	return NULL;
}

// Function called to send the type 0x01 message, receive the 0x02 message, send the 0x03 message, and conduct the TCP data transfer
void * transferSession(void * in){
	// Increase the number of active threads to reflect this one
	pthread_mutex_lock(&numthrd_lock);
	numthrds += 1;
	pthread_mutex_unlock(&numthrd_lock);
	// Grab the start time for the log
	time_t start = time(0);
	char * start_str = ctime(&start);
	start_str[strlen(start_str)-1] = '\0';
        session * s = (session *) in;
        // Grab some admin info from the session struct that was used as input
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
		free(s->packet);
		free(s);
		threadClose(socketsIndex,sock,NULL);
                pthread_detach(pthread_self());
		// Log it
		char * entry = calloc(MAX_LOG_ENTRY,sizeof(char));
		snprintf(entry,MAX_LOG_ENTRY,"%s:%s->UNK:%d:Invalid Packet Type\n",start_str,ctwoAddr,totalBytes);
		pthread_mutex_lock(&log_lock);
		FILE * log = fopen(logfile,"a");
		unsigned char * encEntry = encode64((unsigned char *) entry);
		char * cencEntry = (char *) encEntry;
		fputs(cencEntry,log);
		free(entry);
		free(encEntry);
		fclose(log);
		pthread_mutex_unlock(&log_lock);
		return NULL;
        }
        // Scrape the address of the FSS
        char * fssAddr = calloc(16,sizeof(char));
	if (scrapeAddr(fssAddr,s->packet)){
		// Not a valid FSS Address
		free(s->packet);
		free(s);
		threadClose(socketsIndex,sock,NULL);
		pthread_detach(pthread_self());
		// log it
		char * entry = calloc(MAX_LOG_ENTRY,sizeof(char));
		snprintf(entry,MAX_LOG_ENTRY,"%s:%s->UNK:%d:Invalid FSS Address\n",start_str,ctwoAddr,totalBytes);
		pthread_mutex_lock(&log_lock);
		FILE * log = fopen(logfile,"a");
		unsigned char * encEntry = encode64((unsigned char *) entry);
		char * cencEntry = (char *) encEntry;
		fputs(cencEntry,log);
		free(entry);
		free(fssAddr);
		free(encEntry);
		fclose(log);
		pthread_mutex_unlock(&log_lock);
		return NULL;
	}
        // Scrape FSS Port
        int fssUDPPort = scrapePort(s->packet);
        if (fssUDPPort == -1){
                // Invalid port number
		free(s->packet);
		free(s);
		threadClose(socketsIndex,sock,NULL);
                pthread_detach(pthread_self());
		// Log it
		char * entry = calloc(MAX_LOG_ENTRY,sizeof(char));
		snprintf(entry,MAX_LOG_ENTRY,"%s:%s->%s:%d:Invalid FSS UDP Port\n",start_str,ctwoAddr,fssAddr,totalBytes);
		pthread_mutex_lock(&log_lock);
		FILE * log = fopen(logfile,"a");
		unsigned char * encEntry = encode64((unsigned char *) entry);
		char * cencEntry = (char *) encEntry;
		fputs(cencEntry,log);
		free(entry);
		free(encEntry);
		free(fssAddr);
		fclose(log);
		pthread_mutex_unlock(&log_lock);
		return NULL;
        }
        // Scrape length of pattern and pattern
        Pattern * parsed = calloc(1,sizeof(Pattern));
	int patternLen = scrapePattern(parsed,s->packet);
        if (patternLen == -1){
                // Invalid pattern
		free(s->packet);
		free(s);
		threadClose(socketsIndex,sock,parsed);
                pthread_detach(pthread_self());
		// Log it
		char * entry = calloc(MAX_LOG_ENTRY,sizeof(char));
		snprintf(entry,MAX_LOG_ENTRY,"%s:%s->%s:%d:Invalid Encoding Pattern\n",start_str,ctwoAddr,fssAddr,totalBytes);
		pthread_mutex_lock(&log_lock);
		FILE * log = fopen(logfile,"a");
		unsigned char * encEntry = encode64((unsigned char *) entry);
		char * cencEntry = (char *) encEntry;
		fputs(cencEntry,log);
		free(entry);
		free(fssAddr);
		free(encEntry);
		fclose(log);
		pthread_mutex_unlock(&log_lock);
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
		// Failed to convert FSS Address
		free(s);
		free(init);
		free(message);
		threadClose(socketsIndex,sock,parsed);
		pthread_detach(pthread_self());
		// Log it
		char * entry = calloc(MAX_LOG_ENTRY,sizeof(char));
		snprintf(entry,MAX_LOG_ENTRY,"%s:%s->%s:%d:Internal - Failed to convert FSS Address\n",start_str,ctwoAddr,fssAddr,totalBytes);
		pthread_mutex_lock(&log_lock);
		FILE * log = fopen(logfile,"a");
		unsigned char * encEntry = encode64((unsigned char *) entry);
		char * cencEntry = (char *) encEntry;
		fputs(cencEntry,log);
		free(entry);
		free(encEntry);
		free(fssAddr);
		fclose(log);
		pthread_mutex_unlock(&log_lock);
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
	  // Allocate space for the response
	unsigned char * response = calloc(MAX_SIZE,sizeof(unsigned char));
	  // Allocate space for the response address
	struct sockaddr_in resp;
	memset(&resp,0,sizeof(resp));
	socklen_t respLen = sizeof(resp);
	// Allocate space for the decoded message and some other admin values
	int fssTCPPort;
	unsigned char * decoded = calloc(messageLen,sizeof(unsigned char));
	int fails = 0;	// keep track of how many packets come through that aren't responses
		// protection against someone trying to start a session, hitting this port, and causing the thread to exit before the 0x02 comes in
	// Build the address struct for the C2
	struct sockaddr_in ctwo;
	ctwo.sin_family = AF_INET;
	ctwo.sin_port = htons(ctwoPort);
	if (inet_aton(ctwoAddr,&(ctwo.sin_addr)) < 0){
		// Failed to convert C2 address
		free(decoded);
		threadClose(socketsIndex,sock,parsed);
		pthread_detach(pthread_self());
		// Log it
		char * entry = calloc(MAX_LOG_ENTRY,sizeof(char));
		snprintf(entry,MAX_LOG_ENTRY,"%s:%s->%s:%d:Internal - Failed to convert C2 Address\n",start_str,ctwoAddr,fssAddr,totalBytes);
		pthread_mutex_lock(&log_lock);
		FILE * log = fopen(logfile,"a");
		unsigned char * encEntry = encode64((unsigned char *) entry);
		char * cencEntry = (char *) encEntry;
		fputs(cencEntry,log);
		free(entry);
		free(fssAddr);
		free(encEntry);
		fclose(log);
		pthread_mutex_unlock(&log_lock);
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
				// If the response was packet type 0x04 indicating that the file already exists on the FSS
				// Send the 0x04 packet to the C2
				sendto(sock,response,1,0,(struct sockaddr *) &ctwo,sizeof(ctwo));
				free(decoded);
				free(response);
				free(message);
				threadClose(socketsIndex,sock,parsed);
				pthread_detach(pthread_self());
				// Log it
				char * entry = calloc(MAX_LOG_ENTRY,sizeof(char));
				snprintf(entry,MAX_LOG_ENTRY,"%s:%s->%s:%d:File Already Exists\n",start_str,ctwoAddr,fssAddr,totalBytes);
				pthread_mutex_lock(&log_lock);
				FILE * log = fopen(logfile,"a");
				unsigned char * encEntry = encode64((unsigned char *) entry);
				char * cencEntry = (char *) encEntry;
				fputs(cencEntry,log);
				free(entry);
				free(encEntry);
				free(fssAddr);
				fclose(log);
				pthread_mutex_unlock(&log_lock);
				return NULL;
			}
			// Unpack the response, making sure it was a valid packet
			fssTCPPort = unpackResponse(response,parsed,message,decoded,messageLen);
			if (fssTCPPort != -1){
				// Add the number of bytes we received and break out of the receive loop
				totalBytes += respSize;
				break;
			}
			fails += 1;
		}
	}
	if (fails == 3){
		// If the loop ended because it failed
		free(decoded);
		free(message);
		free(response);
		threadClose(socketsIndex,sock,parsed);
		pthread_detach(pthread_self());
		// Log it
		char * entry = calloc(MAX_LOG_ENTRY,sizeof(char));
		snprintf(entry,MAX_LOG_ENTRY,"%s:%s->%s:%d:Timeout waiting for FSS\n",start_str,ctwoAddr,fssAddr,totalBytes);
		pthread_mutex_lock(&log_lock);
		FILE * log = fopen(logfile,"a");
		unsigned char * encEntry = encode64((unsigned char *) entry);
		char * cencEntry = (char *) encEntry;
		fputs(cencEntry,log);
		free(entry);
		free(encEntry);
		fclose(log);
		pthread_mutex_unlock(&log_lock);
		free(fssAddr);
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
		// Socket create error
		free(validation);
		close(ctwoListenSock);
		threadClose(socketsIndex,sock,parsed);
		pthread_detach(pthread_self());
		// Log it
		char * entry = calloc(MAX_LOG_ENTRY,sizeof(char));
		snprintf(entry,MAX_LOG_ENTRY,"%s:%s->%s:%d:Internal - Failed to create TCP Socket\n",start_str,ctwoAddr,fssAddr,totalBytes);
		pthread_mutex_lock(&log_lock);
		FILE * log = fopen(logfile,"a");
		unsigned char * encEntry = encode64((unsigned char *) entry);
		char * cencEntry = (char *) encEntry;
		fputs(cencEntry,log);
		free(fssAddr);
		free(entry);
		free(encEntry);
		fclose(log);
		pthread_mutex_unlock(&log_lock);
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
	// Accept connections until we get one from the C2
	int ctwoSock;
	while (1){
		ctwoSock = accept(ctwoListenSock,(struct sockaddr *) &incoming,&incSize);
		if (incoming.sin_family == AF_INET || inet_ntoa(incoming.sin_addr) == inet_ntoa(ctwo.sin_addr)){
			// connection is from the right address
			break;
		}
		else{
			close(ctwoSock);
		}
	}
	if (ctwoSock < 0){
		// catch any random errors that might pop out of the accept call
		close(ctwoListenSock);
		threadClose(socketsIndex,sock,parsed);
		pthread_detach(pthread_self());
		// log it
		char * entry = calloc(MAX_LOG_ENTRY,sizeof(char));
		snprintf(entry,MAX_LOG_ENTRY,"%s:%s->%s:%d:TCP Handshake Error with C2\n",start_str,ctwoAddr,fssAddr,totalBytes);
		pthread_mutex_lock(&log_lock);
		FILE * log = fopen(logfile,"a");
		unsigned char * encEntry = encode64((unsigned char *) entry);
		char * cencEntry = (char *) encEntry;
		fputs(cencEntry,log);
		free(entry);
		free(fssAddr);
		free(encEntry);
		fclose(log);
		pthread_mutex_unlock(&log_lock);
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
			// catch errors thrown by fputs
			free(data);
			free(tmpfile);
			fclose(temp);
			close(ctwoListenSock);
			threadClose(socketsIndex,sock,parsed);
			pthread_detach(pthread_self());
			// log it
			char * entry = calloc(MAX_LOG_ENTRY,sizeof(char));
			snprintf(entry,MAX_LOG_ENTRY,"%s:%s->%s:%d:Internal - Error Writing to temporary file\n",start_str,ctwoAddr,fssAddr,totalBytes);
			pthread_mutex_lock(&log_lock);
			FILE * log = fopen(logfile,"a");
			unsigned char * encEntry = encode64((unsigned char *) entry);
			char * cencEntry = (char *) encEntry;
			fputs(cencEntry,log);
			free(entry);
			free(fssAddr);
			free(encEntry);
			fclose(log);
			pthread_mutex_unlock(&log_lock);
			return NULL;
		}
		totalrcvd += rcvd;
		// zero out the response array
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
		// Catch errors converting the fss TCP address
		free(tmpfile);
		fclose(temp);
		close(ctwoListenSock);
		threadClose(socketsIndex,sock,parsed);
		pthread_detach(pthread_self());
		// Log it
		char * entry = calloc(MAX_LOG_ENTRY,sizeof(char));
		snprintf(entry,MAX_LOG_ENTRY,"%s:%s->%s:%d:Internal - Failed to convert FSS TCP Address\n",start_str,ctwoAddr,fssAddr,totalBytes);
		pthread_mutex_lock(&log_lock);
		FILE * log = fopen(logfile,"a");
		unsigned char * encEntry = encode64((unsigned char *) entry);
		char * cencEntry = (char *) encEntry;
		fputs(cencEntry,log);
		free(entry);
		free(encEntry);
		free(fssAddr);
		fclose(log);
		pthread_mutex_unlock(&log_lock);
		return NULL;
	}
	if (connect(ctwoListenSock,(struct sockaddr *) &fssTCP,sizeof(fssTCP)) < 0){
		// Catch errors connection to the FSS
		free(tmpfile);
		fclose(temp);
		close(ctwoListenSock);
		threadClose(socketsIndex,sock,parsed);
		pthread_detach(pthread_self());
		// Log it
		char * entry = calloc(MAX_LOG_ENTRY,sizeof(char));
		snprintf(entry,MAX_LOG_ENTRY,"%s:%s->%s:%d:TCP Handshake Error with FSS\n",start_str,ctwoAddr,fssAddr,totalBytes);
		pthread_mutex_lock(&log_lock);
		FILE * log = fopen(logfile,"a");
		unsigned char * encEntry = encode64((unsigned char *) entry);
		char * cencEntry = (char *) encEntry;
		fputs(cencEntry,log);
		free(entry);
		free(encEntry);
		fclose(log);
		pthread_mutex_unlock(&log_lock);
		return NULL;
	}
	// Close the write file
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
			// catch send errors
			free(tmpfile);
			fclose(tmp);
			close(ctwoListenSock);
			threadClose(socketsIndex,sock,parsed);
			pthread_detach(pthread_self());
			// log it
			char * entry = calloc(MAX_LOG_ENTRY,sizeof(char));
			snprintf(entry,MAX_LOG_ENTRY,"%s:%s->%s:%d:Internal - Send Error\n",start_str,ctwoAddr,fssAddr,totalBytes);
			pthread_mutex_lock(&log_lock);
			FILE * log = fopen(logfile,"a");
			unsigned char * encEntry = encode64((unsigned char *) entry);
			char * cencEntry = (char *) encEntry;
			fputs(cencEntry,log);
			free(entry);
			free(encEntry);
			fclose(log);
			pthread_mutex_unlock(&log_lock);
			return NULL;
		}
		sent += pktSize;
		memset(&raw[0],0,read);
	}
	totalBytes += sent;
	free(raw);
	// close the file and delete it
	fclose(tmp);
	remove(tmpfile);
	free(tmpfile);
	// clean up now that the threads work is done
	close(ctwoListenSock);
	threadClose(socketsIndex,sock,parsed);
	 // log
	char * entry = calloc(MAX_LOG_ENTRY,sizeof(char));
	snprintf(entry,MAX_LOG_ENTRY,"%s:%s->%s:%d:Success\n",start_str,ctwoAddr,fssAddr,totalBytes);
	pthread_mutex_lock(&log_lock);
	FILE * log = fopen(logfile,"a");
	unsigned char * encEntry = encode64((unsigned char *) entry);
	char * cencEntry = (char *) encEntry;
	fputs(cencEntry,log);
	free(entry);
	free(encEntry);
	fclose(log);
	pthread_mutex_unlock(&log_lock);
	free(fssAddr);
	// decrement the number of threads
	pthread_mutex_lock(&numthrd_lock);
	numthrds -= 1;
	pthread_mutex_unlock(&numthrd_lock);
	// detach to allow it to release its own resources
        pthread_detach(pthread_self());
	return NULL;
}

// Function to handle thread clean up in the event a session handler thread needs to close
// Takes a socket file descriptor and that socket's index within the array of UDP listeners
void threadClose(int socketsIndex, int sock,Pattern * parsed){
	// if parsed is allocated, free all of its bits
	if (parsed != NULL){
		free(parsed->opts);
		free(parsed->ops);
		free(parsed->vals);
		free(parsed->lens);
		free(parsed);
	}
	// add the file descriptor for this socket back into the array of sockets
	pthread_mutex_lock(&sockets_lock);
	sockets[socketsIndex] = sock;
	FD_SET(sock,&fds);
	pthread_mutex_unlock(&sockets_lock);
	// Decrement the number of active threads
	pthread_mutex_lock(&numthrd_lock);
	numthrds -= 1;
	pthread_mutex_unlock(&numthrd_lock);
}

// Function to read only a certain number of lines starting at the end of the file
// AUTHOR NAME: utkarsh111 (geekfsforgeeks.com username)
// WEB ADDRESS: http://qa.geeksforgeeks.org/6416/how-to-read-last-n-line-from-a-text-file-in-c-language
// DATE ACCESSED: 15APR2018
// Altered slightly to match the behavior I need
int reverseRead(int total){
	pthread_mutex_lock(&log_lock);
	// Allocate space for each line
	char * line = calloc(MAX_LOG_ENTRY,sizeof(unsigned char));
	// open the log file
	FILE * log = fopen(logfile,"rb");
	// Get the total number of lines in the file
	int totalLines = 0;
	while (fgets(line,MAX_LOG_ENTRY,log)){
		totalLines += 1;
	}
	// Reset to the beginning of the file
	fseek(log,0,SEEK_SET);
	// Read
	int currLine = 0;
	while (fgets(line,MAX_LOG_ENTRY,log)){
		// null terminate the line
		line[strlen(line)-1] = '\0';
		currLine += 1;
		// if the line were looking at is one of the ones were looking for
		if (currLine > totalLines-total){
			// decode and print the line
			unsigned char * dline = decode64((unsigned char *) line);
			printf("%s\n",dline);
			free(dline);
		}
		// zero out the array that stores the line
		memset(line,0,MAX_LOG_ENTRY);
	}
	fclose(log);
	pthread_mutex_unlock(&log_lock);
	free(line);
	return 0;
}
