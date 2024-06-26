// C Source File for Testing the FTS Encoder Module

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include "CUnit/Basic.h"
#include "../headers/encoder.h"
#include "../headers/udpHandler.h"
#include "../headers/tcpHandler.h"

#define TIMEOUT 2000
#define TOTAL_SOCKETS 1001

Pattern * full;
unsigned char * messagesFull;
Pattern * repeat;
unsigned char * messagesRepeat;
char * pas;
unsigned char * encPas;
char * pass;
unsigned char * encPass;
char * passe;
unsigned char * encPasse;

Pattern * parsed;
unsigned char * lo;
unsigned char * fs;
unsigned char * mid;
unsigned char * zero;

int init_enc(void){
	full = calloc(1,sizeof(Pattern));
	char * patt = strdup("~:2;^2:2;ror2:2;rol2:2");
	int status = patternValidate(patt,full);
	free(patt);
	if (status != 0){
		printf("FATAL - Failed to validate encoding pattern 'full'\n");
		exit(1);
	}
	messagesFull = calloc(9,sizeof(unsigned char));
	messagesFull[0] = 0x92;
	messagesFull[1] = 0x9a;
	messagesFull[2] = 0x71;
	messagesFull[3] = 0x71;
	messagesFull[4] = 0x58;
	messagesFull[5] = 0xd9;
	messagesFull[6] = 0x95;
	messagesFull[7] = 0xcd;
	messagesFull[8] = '\0';

	repeat = calloc(1,sizeof(Pattern));
	patt = strdup("~:2;^2:2");
	status = patternValidate(patt,repeat);
	free(patt);
	if (status != 0){
		printf("FATAL - Failed to validate encoding pattern 'repeat'\n");
		exit(1);
	}
	messagesRepeat = calloc(9,sizeof(unsigned char));
	messagesRepeat[0] = 0x92;
	messagesRepeat[1] = 0x9a;
	messagesRepeat[2] = 0x71;
	messagesRepeat[3] = 0x71;
	messagesRepeat[4] = 0x9e;
	messagesRepeat[5] = 0x98;
	messagesRepeat[6] = 0x67;
	messagesRepeat[7] = 0x71;
	messagesRepeat[8] = '\0';

	pas = "pas";
	encPas = calloc(5,sizeof(unsigned char));
	encPas[0] = 0x63;
	encPas[1] = 0x47;
	encPas[2] = 0x46;
	encPas[3] = 0x7a;
	encPas[4] = '\0';
	pass = "pass";
	encPass = calloc(9,sizeof(unsigned char));
	encPass[0] = 0x63;
	encPass[1] = 0x47;
	encPass[2] = 0x46;
	encPass[3] = 0x7a;
	encPass[4] = 0x63;
	encPass[5] = 0x77;
	encPass[6] = 0x3d;
	encPass[7] = 0x3d;
	encPass[8] = '\0';
	passe = "passe";
	encPasse = calloc(9,sizeof(unsigned char));
	encPasse[0] = 0x63;
	encPasse[1] = 0x47;
	encPasse[2] = 0x46;
	encPasse[3] = 0x7a;
	encPasse[4] = 0x63;
	encPasse[5] = 0x32;
	encPasse[6] = 0x55;
	encPasse[7] = 0x3d;
	encPasse[8] = '\0';

	return 0;
}

int init_udp(void){
	parsed = calloc(1,sizeof(Pattern));
	lo = calloc(28,sizeof(unsigned char));
	lo[0] = 0x00;
	lo[1] = 0x7f;
	lo[2] = 0x00;
	lo[3] = 0x00;
	lo[4] = 0x01;
	lo[5] = 0x00;
	lo[6] = 0x01;
	lo[7] = 0x00;
	lo[8] = 0x07;
	lo[9] = 0x7e;
	lo[10] = 0x3a;
	lo[11] = 0x32;
	lo[12] = 0x3b;
	lo[13] = 0x7e;
	lo[14] = 0x3a;
	lo[15] = 0x32;
	lo[16] = 0x00;
	lo[17] = 0x04;
	lo[18] = 0x70;
	lo[19] = 0x61;
	lo[20] = 0x73;
	lo[21] = 0x73;
	lo[22] = 0x70;
	lo[23] = 0x61;
	lo[24] = 0x73;
	lo[25] = 0x73;
	lo[26] = 0x77;
	lo[27] = 0x64;
	fs = calloc(31,sizeof(unsigned char));
	memcpy(fs,lo,28);
	fs[1] = 0xff;
	fs[2] = 0xff;
	fs[3] = 0xff;
	fs[4] = 0xff;
	fs[5] = 0xff;
	fs[6] = 0xff;
	fs[8] = 0x09;
	fs[9] = 0x5e;
	fs[10] = 0x33;
	fs[11] = 0x3a;
	fs[12] = 0x31;
	fs[13] = 0x3b;
	fs[14] = 0x5e;
	fs[15] = 0x33;
	fs[16] = 0x3a;
	fs[17] = 0x31;
	fs[18] = 0x00;
	fs[19] = 0x05;
	fs[20] = 0x6d;
	fs[21] = 0x61;
	fs[22] = 0x73;
	fs[23] = 0x73;
	fs[24] = 0x65;
	fs[25] = 0x70;
	fs[26] = 0x61;
	fs[27] = 0x73;
	fs[28] = 0x73;
	fs[29] = 0x77;
	fs[30] = 0x64;
	mid = calloc(34,sizeof(unsigned char));
	memcpy(mid,lo,28);
	mid[1] = 0x77;
	mid[2] = 0x77;
	mid[3] = 0x77;
	mid[4] = 0x77;
	mid[5] = 0x77;
	mid[6] = 0x77;
	mid[8] = 0x0d;
	mid[9] = 0x72;
	mid[10] = 0x6f;
	mid[11] = 0x72;
	mid[12] = 0x39;
	mid[13] = 0x3a;
	mid[14] = 0x37;
	mid[15] = 0x3b;
	mid[16] = 0x72;
	mid[17] = 0x6f;
	mid[18] = 0x72;
	mid[19] = 0x39;
	mid[20] = 0x3a;
	mid[21] = 0x37;
	mid[22] = 0x00;
	mid[23] = 0x04;
	mid[24] = 0x70;
	mid[25] = 0x61;
	mid[26] = 0x73;
	mid[27] = 0x73;
	mid[28] = 0x70;
	mid[29] = 0x61;
	mid[30] = 0x73;
	mid[31] = 0x73;
	mid[32] = 0x77;
	mid[33] = 0x64;
	zero = calloc(26,sizeof(unsigned char));
	memcpy(zero,mid,26);
	zero[1] = 0x00;
	zero[2] = 0x00;
	zero[3] = 0x00;
	zero[4] = 0x00;
	zero[5] = 0x00;
	zero[6] = 0x00;
	zero[9] = 0x71;
	return 0;
}

int init_tcp(void){
	return 0;
}

int clean_enc(void){
	if (full != NULL){
		free(full->opts);
		free(full->ops);
		free(full->vals);
		free(full->lens);
		free(full);
	}
	if (messagesFull != NULL){
		free(messagesFull);
	}
	if (repeat != NULL){
		free(repeat->opts);
		free(repeat->ops);
		free(repeat->vals);
		free(repeat->lens);
		free(repeat);
	}
	if (messagesRepeat != NULL){
		free(messagesRepeat);
	}
	if (encPas != NULL){
		free(encPas);
	}
	if (encPass != NULL){
		free(encPass);
	}
	if (encPasse != NULL){
		free(encPasse);
	}
	return 0;
}

int clean_udp(void){
	if (parsed != NULL){
		free(parsed->opts);
		free(parsed->ops);
		free(parsed->vals);
		free(parsed->lens);
		free(parsed);
	}
	if (lo != NULL){
		free(lo);
	}
	if (fs != NULL){
		free(fs);
	}
	if (mid != NULL){
		free(mid);
	}
	if (zero != NULL){
		free(zero);
	}
	return 0;
}

int clean_tcp(void){
	return 0;
}

// 	ENCODER TESTS	//
// Function containing the tests of roll right (ror)
void testROR(void){
	CU_ASSERT(ror('m',1) == (unsigned char) 182);
	CU_ASSERT(ror('m',8) == (unsigned char) 109);
	CU_ASSERT(ror('m',9) == (unsigned char) 182);
	CU_ASSERT(ror('m',4) == (unsigned char) 214);
	CU_ASSERT(ror('m',0) == (unsigned char) 109);
}

// Function containing the tests of roll left (rol)
void testROL(void){
	CU_ASSERT(rol('m',1) == (unsigned char) 218);
	CU_ASSERT(rol('m',8) == (unsigned char) 109);
	CU_ASSERT(rol('m',9) == (unsigned char) 218);
	CU_ASSERT(rol('m',4) == (unsigned char) 214);
	CU_ASSERT(rol('m',0) == (unsigned char) 109);
}

// Function containing the test of patternValidate
void testPATTERNVALIDATE(void){
	Pattern * p = calloc(1,sizeof(Pattern));
	int status;
	char ** opts;
	char * ops;
	int * vals;
	int * lens;
	// Valid pattern
	char * patt = strdup("~:2;^3:45");
	status = patternValidate(patt,p);
	CU_ASSERT(status == 0);
	CU_ASSERT(p->numOpts == 2);
	CU_ASSERT(p->maxOpts == 2);
	opts = calloc(p->numOpts,sizeof(char *));
	ops = calloc(p->numOpts,sizeof(char));
	vals = calloc(p->numOpts,sizeof(int));
	lens = calloc(p->numOpts,sizeof(int));
	opts[0] = "~:2";
	ops[0] = '~';
	vals[0] = 0;
	lens[0] = 2;
	opts[1] = "^3:45";
	ops[1] = '^';
	vals[1] = 3;
	lens[1] = 45;
	for (int i = 0; i < p->numOpts;i++){
		CU_ASSERT(strncmp(p->opts[i],opts[i],strlen(opts[i])) == 0);
		CU_ASSERT(p->ops[i] == ops[i]);
		CU_ASSERT(p->vals[i] == vals[i]);
		CU_ASSERT(p->lens[i] == lens[i]);
	}
	free(p->opts);
	free(opts);
	free(p->ops);
	free(ops);
	free(p->vals);
	free(vals);
	free(p->lens);
	free(lens);
	free(patt);
	// Multiple options
	patt = strdup("~:2;^3:45;ror5:36;rol9:3");
	status = patternValidate(patt,p);
	CU_ASSERT(status == 0);
	CU_ASSERT(p->numOpts == 4);
	CU_ASSERT(p->maxOpts == 4);
	opts = calloc(p->numOpts,sizeof(char *));
	ops = calloc(p->numOpts,sizeof(char));
	vals = calloc(p->numOpts,sizeof(int));
	lens = calloc(p->numOpts,sizeof(int));
	opts[0] = "~:2";
	ops[0] = '~';
	vals[0] = 0;
	lens[0] = 2;
	opts[1] = "^3:45";
	ops[1] = '^';
	vals[1] = 3;
	lens[1] = 45;
	opts[2] = "ror5:36";
	ops[2] = 'r';
	vals[2] = 5;
	lens[2] = 36;
	opts[3] = "rol9:3";
	ops[3] = 'l';
	vals[3] = 1;
	lens[3] = 3;
	for (int i = 0; i < p->numOpts;i++){
		CU_ASSERT(strncmp(p->opts[i],opts[i],strlen(opts[i])) == 0);
		CU_ASSERT(p->ops[i] == ops[i]);
		CU_ASSERT(p->vals[i] == vals[i]);
		CU_ASSERT(p->lens[i] == lens[i]);
	}
	free(p->opts);
	free(opts);
	free(p->ops);
	free(ops);
	free(p->vals);
	free(vals);
	free(p->lens);
	free(lens);
	free(patt);
	// redirect stdout so we dont get the error messages
	freopen("/dev/null","w",stdout);
	// Too few options
	patt = strdup("~:2");
	status = patternValidate(patt,p); // Invalid Pattern
	CU_ASSERT(status == 1);
	free(p->opts);
	// Bad Operators/Values
	free(patt);
	patt = strdup("~3:2;ror1:3");
	CU_ASSERT(patternValidate(patt,p) == 1);
	free(p->opts);
	free(p->ops);
	free(p->vals);
	free(p->lens);
	free(patt);
	patt = strdup("ror:2;~:3");
	CU_ASSERT(patternValidate(patt,p) == 1);
	free(p->opts);
	free(p->ops);
	free(p->vals);
	free(p->lens);
	free(patt);
	patt = strdup("rol:2;~:3");
	CU_ASSERT(patternValidate(patt,p) == 1);
	free(p->opts);
	free(p->ops);
	free(p->vals);
	free(p->lens);
	free(patt);
	patt = strdup("^:2;~:3");
	CU_ASSERT(patternValidate(patt,p) == 1);
	free(p->opts);
	free(p->ops);
	free(p->vals);
	free(p->lens);
	free(patt);
	patt = strdup("x:2;~:3");
	CU_ASSERT(patternValidate(patt,p) == 1);
	free(p->opts);
	free(p->ops);
	free(p->vals);
	free(p->lens);
	free(patt);
	patt = strdup("~:;ror3:4");
	// Bad Length
	CU_ASSERT(patternValidate(patt,p) == 1);
	free(p->opts);
	free(p->ops);
	free(p->vals);
	free(p->lens);
	free(patt);
	// Bad Format
	patt = strdup("~3:ror3:4");
	CU_ASSERT(patternValidate(patt,p) == 1);
	free(p->opts);
	free(patt);
	patt = strdup("~:3ror3:4");
	CU_ASSERT(patternValidate(patt,p) == 1);
	free(patt);
	free(p->opts);
	// fix stdout
	freopen("/dev/tty","w",stdout);
	free(p);
}

// Function containing the tests for the encode function
void testENCODE(void){
	char * messages = strdup("messages");
	// Pattern covers the full string and all operations
	unsigned char * encoded = encode((unsigned char *) messages,full);
	CU_ASSERT(memcmp(encoded,messagesFull,8) == 0);
	free(encoded);
	// Pattern covers more than the string
	char * mess = strdup("mess");
	encoded = encode((unsigned char *) mess,full);
	CU_ASSERT(memcmp(encoded,messagesFull,4) == 0);
	free(encoded);
	free(mess);
	// Pattern repeats
	encoded = encode((unsigned char *) messages,repeat);
	CU_ASSERT(memcmp(encoded,messagesRepeat,8) == 0);
	free(messages);
	free(encoded);
}

// Function containing the tests for the decode function
void testDECODE(void){
	// Pattern covers the full string
	unsigned char * decoded = decode(messagesFull,full);
	CU_ASSERT(memcmp(decoded,"messages",8) == 0);
	free(decoded);
	// Pattern covers more than the string
	unsigned char * mess = calloc(5,sizeof(unsigned char));
	mess[0] = 0x92;
	mess[1] = 0x9a;
	mess[2] = 0x71;
	mess[3] = 0x71;
	mess[4] = '\0';
	decoded = decode(mess,full);
	CU_ASSERT(memcmp(decoded,"mess",4) == 0);
	free(decoded);
	// Pattern repeats
	decoded = decode(messagesRepeat,repeat);
	CU_ASSERT(memcmp(decoded,"messages",8) == 0);
	free(decoded);
	free(mess);
}

// Function containing the tests for the encode64 function
void testENCODE64(void){
	// Just the right length
	unsigned char * encoded = encode64((unsigned char *) pas);
	CU_ASSERT(memcmp(encoded,encPas,4) == 0);
	free(encoded);
	// One more
	encoded = encode64((unsigned char *) pass);
	CU_ASSERT(memcmp(encoded,encPass,8) == 0);
	free(encoded);
	// Two more
	encoded = encode64((unsigned char *) passe);
	CU_ASSERT(memcmp(encoded,encPasse,8) == 0);
	free(encoded);
}

// Function containing the tests for the decode64 function
void testDECODE64(void){
	// just the right length
	CU_ASSERT(memcmp(decode64(encPas),(unsigned char *) pas,4) == 0);
	// one more
	CU_ASSERT(memcmp(decode64(encPass),(unsigned char *) pass,8) == 0);
	// two more
	CU_ASSERT(memcmp(decode64(encPasse),(unsigned char *) passe,8) == 0);
}

///	UDP HANDLER TESTS	///
// Function containing the tests for the makeSocket function
void testMAKESOCKET(void){
	struct timeval t;
	memset(&t,0,sizeof(t));
	t.tv_usec = TIMEOUT;
	for (int i = 0; i < TOTAL_SOCKETS;i ++){
		int s = makeSocket(i,&t);
		CU_ASSERT(s != -1);
		close(s);
	}
}

// Function containing the tests for the scrapeAddr function
void testSCRAPEADDR(void){
	char * addr = calloc(16,sizeof(char));
	CU_ASSERT(scrapeAddr(addr,lo) == 0);
	CU_ASSERT(strncmp(addr,"127.0.0.1",strlen(addr)) == 0);
	CU_ASSERT(scrapeAddr(addr,mid) == 0);
	CU_ASSERT(strncmp(addr,"119.119.119.119",strlen(addr)) == 0);
	// Broadcast address
	CU_ASSERT(scrapeAddr(addr,zero) == 1);
	CU_ASSERT(scrapeAddr(addr,fs) == 1);
	free(addr);
}

// Function containing the tests for the scrapePort function
void testSCRAPEPORT(void){
	// Good ports
	CU_ASSERT(scrapePort(lo) == 1);
	CU_ASSERT(scrapePort(fs) == 65535);
	CU_ASSERT(scrapePort(mid) == 30583);
	// Bad ports
	CU_ASSERT(scrapePort(zero) == -1);
}

// Function containing the tests for the scrapePattern function
void testSCRAPEPATTERN(void){
	CU_ASSERT(scrapePattern(parsed,lo) == 7);
	CU_ASSERT(parsed->numOpts == 2);
	CU_ASSERT('~' == parsed->ops[0]);
	CU_ASSERT('~' == parsed->ops[1]);
	CU_ASSERT(parsed->vals[0] == 0);
	CU_ASSERT(parsed->vals[1] == 0);
	CU_ASSERT(parsed->lens[0] == 2);
	CU_ASSERT(parsed->lens[1] == 2);
	free(parsed->opts);
	free(parsed->ops);
	free(parsed->vals);
	free(parsed->lens);
	CU_ASSERT(scrapePattern(parsed,fs) == 9);
	CU_ASSERT(parsed->numOpts == 2);
	CU_ASSERT('^' == parsed->ops[0]);
	CU_ASSERT('^' == parsed->ops[1]);
	CU_ASSERT(parsed->vals[0] == 3);
	CU_ASSERT(parsed->vals[1] == 3);
	CU_ASSERT(parsed->lens[0] == 1);
	CU_ASSERT(parsed->lens[1] == 1);
	free(parsed->opts);
	free(parsed->ops);
	free(parsed->vals);
	free(parsed->lens);
	CU_ASSERT(scrapePattern(parsed,mid) == 13);
	CU_ASSERT(parsed->numOpts == 2);
	CU_ASSERT('r' == parsed->ops[0]);
	CU_ASSERT('r' == parsed->ops[1]);
	CU_ASSERT(parsed->vals[0] == 1);
	CU_ASSERT(parsed->vals[1] == 1);
	CU_ASSERT(parsed->lens[0] == 7);
	CU_ASSERT(parsed->lens[1] == 7);
	free(parsed->opts);
	free(parsed->ops);
	free(parsed->vals);
	free(parsed->lens);
	// Bad Pattern
	freopen("/dev/null","w",stdout); // redirect stdout
	CU_ASSERT(scrapePattern(parsed,zero) == -1);
	freopen("/dev/tty","w",stdout);
	free(parsed->ops);
	free(parsed->opts);
	free(parsed->lens);
	free(parsed->vals);
}

// Function containing the tests for the scrapeMessage function
void testSCRAPEMESSAGE(void){
	char * patt = strdup("~:2;~:2");
	patternValidate(patt,parsed);
	free(patt);
	unsigned char * message = calloc(5,sizeof(unsigned char));
	memset(message,0,5);
	unsigned char * init = calloc(13,sizeof(unsigned char));
	unsigned char * local = calloc(28,sizeof(unsigned char));
	CU_ASSERT(scrapeMessage(message,memcpy(local,lo,28),16,28,parsed,init) == 4);
	unsigned char initOne[] = {0x01,0x00,0x04,0x8f,0x9e,0x8c,0x8c,0x70,0x61,0x73,0x73,0x77,0x64};
	CU_ASSERT(memcmp(init,initOne,5) == 0);
	free(message);
	free(init);
	free(local);
	message = calloc(6,sizeof(unsigned char *));
//	memset(message,0,6);
	init = calloc(14,sizeof(unsigned char *));
	local = calloc(31,sizeof(unsigned char *));
	CU_ASSERT(scrapeMessage(message,memcpy(local,fs,31),18,31,parsed,init) == 5);
	unsigned char initTwo[] = {0x01,0x00,0x05,0x92,0x9e,0x8c,0x8c,0x9a,0x70,0x61,0x73,0x73,0x77,0x64};
	CU_ASSERT(memcmp(init,initTwo,6) == 0);
	free(message);
	free(local);
	free(init);
	free(parsed->opts);
	free(parsed->ops);
	free(parsed->vals);
	free(parsed->lens);
}

// Function containing the tests for the unpackResponse function
void testUNPACKRESPONSE(void){
	char * patt = strdup("~:2;~:2");
	patternValidate(patt,parsed);
	free(patt);
	unsigned char * message = (unsigned char *) "pass";
	unsigned char * decoded = calloc(4,sizeof(unsigned char));
	unsigned char good[] = {0x02,0xb5,0x73,0x8f,0x9e,0x8c,0x8c};
	CU_ASSERT(unpackResponse(good,parsed,message,decoded,4) == 46451);
	// Bad port
	unsigned char badPort[] = {0x02,0x00,0x00,0x8f,0x9e,0x8c,0x8c};
	CU_ASSERT(unpackResponse(badPort,parsed,message,decoded,4) == -1);
	// Message doesnt match
	unsigned char mass[] = {0x02,0xb5,0x73,0x92,0x9e,0x8c,0x8c};
	CU_ASSERT(unpackResponse(mass,parsed,message,decoded,4) == -1);
	// Bad packet type
	unsigned char type[] = {0xff,0xb5,0x73,0x8f,0x9e,0x8c,0x8c};
	CU_ASSERT(unpackResponse(type,parsed,message,decoded,4) == -1);
	free(decoded);
}

// Function containing the tests for the makeTCPSocket function
void testMAKETCPSOCKET(void){
	int s = makeTCPSocket(16000);
	CU_ASSERT(s != -1);
	close(s);
	// Bad ports
	s = makeTCPSocket(-1);
	CU_ASSERT(s == -1);
	s = makeTCPSocket(65536);
	CU_ASSERT(s == -1);
}

// Function containing the tests for the craftValidation function
void testCRAFTVALIDATION(void){
	unsigned char * validation = calloc(7,sizeof(unsigned char));
	unsigned char * decoded = (unsigned char *) "pass";
	unsigned char pass[] = {0x03,0x42,0x68,0x70,0x61,0x73,0x73};
	CU_ASSERT(craftValidation(validation,decoded,17000,4) == 0);
	CU_ASSERT(memcmp(validation,pass,7) == 0);
	unsigned char mass[] = {0x03,0xff,0xff,0x6d,0x61,0x73,0x73};
	decoded = (unsigned char *) "mass";
	CU_ASSERT(craftValidation(validation,decoded,65535,4) == 0);
	CU_ASSERT(memcmp(validation,mass,7) == 0);
	// Bad port
	CU_ASSERT(craftValidation(validation,decoded,65536,4) == -1);
	CU_ASSERT(craftValidation(validation,decoded,0,4) == -1);
	free(validation);
}

int main(int argc, char ** argv){
	CU_pSuite encSuite = NULL;
	CU_pSuite udpSuite = NULL;
	CU_pSuite tcpSuite = NULL;
	if (CUE_SUCCESS != CU_initialize_registry()){
		return CU_get_error();
	}
	encSuite = CU_add_suite("Encoder",init_enc,clean_enc);
	udpSuite = CU_add_suite("UDPHandler",init_udp,clean_udp);
	tcpSuite = CU_add_suite("TCPHandler",init_tcp,clean_tcp);
	if (NULL == encSuite || NULL == udpSuite || NULL == tcpSuite){
		CU_cleanup_registry();
		return CU_get_error();
	}
	// Add encoder suite tests
	if ((NULL == CU_add_test(encSuite,"ror()",testROR)) ||
		(NULL == CU_add_test(encSuite,"rol()",testROL)) ||
		(NULL == CU_add_test(encSuite,"patternValidate()",testPATTERNVALIDATE)) ||
		(NULL == CU_add_test(encSuite,"encode()",testENCODE)) ||
		(NULL == CU_add_test(encSuite,"decode()",testDECODE)) ||
		(NULL == CU_add_test(encSuite,"encode64()",testENCODE64))){
		CU_cleanup_registry();
		return CU_get_error();
	}
	// Add udp suite tests
	if ((NULL == CU_add_test(udpSuite,"makeSocket()",testMAKESOCKET)) ||
		(NULL == CU_add_test(udpSuite,"scrapeAddr()",testSCRAPEADDR)) ||
		(NULL == CU_add_test(udpSuite,"scrapePort()",testSCRAPEPORT)) ||
		(NULL == CU_add_test(udpSuite,"scrapePattern()",testSCRAPEPATTERN)) ||
		(NULL == CU_add_test(udpSuite,"scrapeMessage()",testSCRAPEMESSAGE)) ||
		(NULL == CU_add_test(udpSuite,"unpackResponse()",testUNPACKRESPONSE))){
		CU_cleanup_registry();
		return CU_get_error();
	}
	// Add tcp suite tests
	if ((NULL == CU_add_test(tcpSuite,"makeTCPSocket()",testMAKETCPSOCKET)) ||
		(NULL == CU_add_test(tcpSuite,"craftValidation()",testCRAFTVALIDATION))){
		CU_cleanup_registry();
		return CU_get_error();
	}
	CU_basic_set_mode(CU_BRM_VERBOSE);
	CU_basic_run_tests();
	CU_cleanup_registry();
	return CU_get_error();
}
