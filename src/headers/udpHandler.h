// C Header File Containing Declarations for all UDP Related Functions
#ifndef UDP_HEADER
#define UDP_HEADER

int makeSocket(int socketNum,struct timeval * t);

int scrapeAddr(char * addr,unsigned char * pkt);
int scrapePort(unsigned char * pkt);
int scrapePattern(Pattern * parsed, unsigned char * pkt);
int scrapeMessage(unsigned char * message,unsigned char * pkt,int start,int size,Pattern * parsed,unsigned char * init);

int unpackResponse(unsigned char * pkt,Pattern * parsed,unsigned char * message,unsigned char * decoded,int len);

#endif
