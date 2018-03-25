// C Header File containing declarations for all TCP Related Functions
#ifndef TCP_HEADER
#define TCP_HEADER

int makeTCPSocket(int port);

int craftValidation(unsigned char * validation,unsigned char * decoded,int port, int len);

#endif
