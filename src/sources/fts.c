// Main function for the File Transfer Service module
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include "../headers/encoder.h"

int main(int argc, char ** argv){
	struct timeval t;
	t.tv_sec = 100;
	fd_set fds;
	int * sockets = calloc(1000,sizeof(int));
	for (int i = 0;i < 0;i++){
		int fd = socket(AF_INET,SOCK_DGRAM,0);
		int port = i + 16000;
		if (fd == -1){
			printf("Error: Failed to Create Socket %d\n",port);
			exit(2);
		}
		struct sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		addr.sin_addr.s_addr = INADDR_ANY;
		if (bind(fd,(struct sockaddr *) &addr,sizeof(addr)) == -1){
			printf("Error: Failed to Bind Socket %d\n",port);
			exit(2);
		}
		sockets[i] = fd;
//		setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&t,sizeof(t));*/
	}
	while (1) {
		printf("cycle\n");
		FD_ZERO(&fds);
		int f = socket(AF_INET,SOCK_DGRAM,0);
		struct sockaddr_in a;
		a.sin_family = AF_INET;
		a.sin_port = htons(16500);
		a.sin_addr.s_addr = INADDR_ANY;
		bind(f,(struct sockaddr *) &a,sizeof(a));
		FD_SET(f,&fds);
//		for (int i = 0;i < 1000;i++){
//			printf("%d\t",sockets[i]);
//			FD_SET(sockets[i],&fds);
//		}
		int rcvd = select(4,&fds,NULL,NULL,&t);
		if (rcvd > 0){
			unsigned char * buf = calloc(20,sizeof(unsigned char));
			int n = recvfrom(f,buf,20,0,NULL,NULL);
			buf[n] = 0;
			printf("%s\n",buf);
			free(buf);
			close(f);
		}
	}
// Need to close each socket after receiving from it, or test with nc maybe
// figure out how to manage this for more than one socket
/*	int currSock = 0;
	while (1) {
		unsigned char * buf = calloc(1000,sizeof(unsigned char));
		struct sockaddr_in s;
		socklen_t len = sizeof(s);
		getsockname(sockets[currSock],(struct sockaddr *)&s,&len);
		int recvd = recvfrom(sockets[currSock],buf,1000,0,NULL,NULL);
		buf[recvd] = 0;
		printf("From %d - %s\n",ntohs(s.sin_port),buf);
		free(buf);
		currSock += 1;
	}
	for (int i = 0; i < 1000;i++){
		while (close(sockets[i]) != 0){
			continue;
		}
	}*/
	return 0;
}
