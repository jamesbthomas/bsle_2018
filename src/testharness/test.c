// C Source File for Testing the FTS Encoder Module

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include "CUnit/Basic.h"
#include "../headers/encoder.h"
#include "../headers/udpHandler.h"

#define TIMEOUT 2000
#define TOTAL_SOCKETS 1001

Pattern * full;
unsigned char * messagesFull;
Pattern * repeat;
unsigned char * messagesRepeat;

Pattern * parsed;
unsigned char * lo;
unsigned char * fs;
unsigned char * mid;
unsigned char * zero;

int init_enc(void){
	full = calloc(1,sizeof(Pattern));
	int status = patternValidate(strdup("~:2;^2:2;ror2:2;rol2:2"),full);
	if (status != 0){
		printf("FATAL - Failed to validate encoding pattern 'full'\n");
		exit(1);
	}
	messagesFull = calloc(8,sizeof(unsigned char));
	messagesFull[0] = 0x92;
	messagesFull[1] = 0x9a;
	messagesFull[2] = 0x71;
	messagesFull[3] = 0x71;
	messagesFull[4] = 0x58;
	messagesFull[5] = 0xd9;
	messagesFull[6] = 0x95;
	messagesFull[7] = 0xcd;

	repeat = calloc(1,sizeof(Pattern));
	status = patternValidate(strdup("~:2;^2:2"),repeat);
	if (status != 0){
		printf("FATAL - Failed to validate encoding pattern 'repeat'\n");
		exit(1);
	}
	messagesRepeat = calloc(8,sizeof(unsigned char));
	messagesRepeat[0] = 0x92;
	messagesRepeat[1] = 0x9a;
	messagesRepeat[2] = 0x71;
	messagesRepeat[3] = 0x71;
	messagesRepeat[4] = 0x9e;
	messagesRepeat[5] = 0x98;
	messagesRepeat[6] = 0x67;
	messagesRepeat[7] = 0x71;

	return 0;
}

int init_udp(void){
	parsed = calloc(1,sizeof(Pattern));
	if (patternValidate(strdup("~:2;~:2"),parsed) != 0){
		printf("FATAL - Failed to validate encoding pattern 'parsed'\n");
		exit(1);
	}
	lo = calloc(20,sizeof(unsigned char));
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
	lo[16] = 0x70;
	lo[17] = 0x61;
	lo[18] = 0x73;
	lo[19] = 0x73;
	fs = calloc(20,sizeof(unsigned char));
	memcpy(fs,lo,20);
	fs[1] = 0xff;
	fs[2] = 0xff;
	fs[3] = 0xff;
	fs[4] = 0xff;
	fs[5] = 0xff;
	fs[6] = 0xff;
	mid = calloc(20,sizeof(unsigned char));
	memcpy(mid,lo,20);
	mid[1] = 0x77;
	mid[2] = 0x77;
	mid[3] = 0x77;
	mid[4] = 0x77;
	mid[5] = 0x77;
	mid[6] = 0x77;
	zero = calloc(20,sizeof(unsigned char));
	memcpy(zero,lo,20);
	zero[1] = 0x00;
	zero[2] = 0x00;
	zero[3] = 0x00;
	zero[4] = 0x00;
	zero[5] = 0x00;
	zero[6] = 0x00;
	return 0;
}

int clean_enc(void){
	if (full != NULL){
		free(full);
	}
	if (messagesFull != NULL){
		free(messagesFull);
	}
	if (repeat != NULL){
		free(repeat);
	}
	if (messagesRepeat != NULL){
		free(messagesRepeat);
	}
	return 0;
}

int clean_udp(void){
	if (parsed != NULL){
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
void testPatternValidate(void){
	Pattern * parsed = calloc(1,sizeof(Pattern));
	int status;
	char ** opts;
	char * ops;
	int * vals;
	int * lens;
	// Valid pattern
	status = patternValidate(strdup("~:2;^3:45"),parsed);
	CU_ASSERT(status == 0);
	CU_ASSERT(parsed->numOpts == 2);
	CU_ASSERT(parsed->maxOpts == 2);
	opts = calloc(parsed->numOpts,sizeof(char *));
	ops = calloc(parsed->numOpts,sizeof(char));
	vals = calloc(parsed->numOpts,sizeof(int));
	lens = calloc(parsed->numOpts,sizeof(int));
	opts[0] = calloc(3,sizeof(char));
	opts[0] = "~:2";
	ops[0] = '~';
	vals[0] = 0;
	lens[0] = 2;
	opts[1] = calloc(5,sizeof(char));
	opts[1] = "^3:45";
	ops[1] = '^';
	vals[1] = 3;
	lens[1] = 45;
	for (int i = 0; i < parsed->numOpts;i++){
		CU_ASSERT(strncmp(parsed->opts[i],opts[i],strlen(opts[i])) == 0);
		CU_ASSERT(parsed->ops[i] == ops[i]);
		CU_ASSERT(parsed->vals[i] == vals[i]);
		CU_ASSERT(parsed->lens[i] == lens[i]);
	}
	// Multiple options
	status = patternValidate(strdup("~:2;^3:45;ror5:36;rol9:3"),parsed);
	CU_ASSERT(status == 0);
	CU_ASSERT(parsed->numOpts == 4);
	CU_ASSERT(parsed->maxOpts == 4);
	opts = calloc(parsed->numOpts,sizeof(char *));
	ops = calloc(parsed->numOpts,sizeof(char));
	vals = calloc(parsed->numOpts,sizeof(int));
	lens = calloc(parsed->numOpts,sizeof(int));
	opts[0] = calloc(3,sizeof(char));
	opts[0] = "~:2";
	ops[0] = '~';
	vals[0] = 0;
	lens[0] = 2;
	opts[1] = calloc(5,sizeof(char));
	opts[1] = "^3:45";
	ops[1] = '^';
	vals[1] = 3;
	lens[1] = 45;
	opts[2] = calloc(7,sizeof(char));
	opts[2] = "ror5:36";
	ops[2] = 'r';
	vals[2] = 5;
	lens[2] = 36;
	opts[3] = calloc(6,sizeof(char));
	opts[3] = "rol9:3";
	ops[3] = 'l';
	vals[3] = 1;
	lens[3] = 3;
	for (int i = 0; i < parsed->numOpts;i++){
		CU_ASSERT(strncmp(parsed->opts[i],opts[i],strlen(opts[i])) == 0);
		CU_ASSERT(parsed->ops[i] == ops[i]);
		CU_ASSERT(parsed->vals[i] == vals[i]);
		CU_ASSERT(parsed->lens[i] == lens[i]);
	}
	// redirect stdout so we dont get the error messages
	freopen("/dev/null","w",stdout);
	// Too few options
	status = patternValidate(strdup("~:2"),parsed); // Invalid Pattern
	CU_ASSERT(status == 1);
	// Bad Operators/Values
	CU_ASSERT(patternValidate(strdup("~3:2;ror1:3"),parsed) == 1);
	CU_ASSERT(patternValidate(strdup("ror:2;~:3"),parsed) == 1);
	CU_ASSERT(patternValidate(strdup("rol:2;~:3"),parsed) == 1);
	CU_ASSERT(patternValidate(strdup("^:2;~:3"),parsed) == 1);
	CU_ASSERT(patternValidate(strdup("x:2;~:3"),parsed) == 1);
	// Bad Length
	CU_ASSERT(patternValidate(strdup("~:;ror3:4"),parsed) == 1);
	// Bad Format
	CU_ASSERT(patternValidate(strdup("~3;ror3:4"),parsed) == 1);
	CU_ASSERT(patternValidate(strdup("~:3ror3:4"),parsed) == 1);
	// fix stdout
	freopen("/dev/tty","w",stdout);
	free(parsed);
}

// Function containing the tests for the encode function
void testENCODE(void){
	// Pattern covers the full string and all operations
	unsigned char * encoded = encode((unsigned char *) strdup("messages"),full);
	CU_ASSERT(memcmp(encoded,messagesFull,8) == 0);
	// Pattern covers more than the string
	encoded = encode((unsigned char *) strdup("mess"),full);
	CU_ASSERT(memcmp(encoded,messagesFull,4) == 0);
	// Pattern repeats
	encoded = encode((unsigned char *) strdup("messages"),repeat);
	CU_ASSERT(memcmp(encoded,messagesRepeat,8) == 0);
}

// Function containing the tests for the decode function
void testDECODE(void){
	// Pattern covers the full string
	CU_ASSERT(memcmp(decode(messagesFull,full),"messages",8) == 0);
	// Pattern covers more than the string
	unsigned char * mess = calloc(4,sizeof(unsigned char));
	mess[0] = 0x92;
	mess[1] = 0x9a;
	mess[2] = 0x71;
	mess[3] = 0x71;
	CU_ASSERT(memcmp(decode(mess,full),"mess",4) == 0);
	// Pattern repeats
	CU_ASSERT(memcmp(decode(messagesRepeat,repeat),"messages",8) == 0);
	free(mess);
}


///	UDP HANDLER TESTS	///
// Function containing the tests for the makeSocket function
void testMAKESOCKET(void){
	struct timeval t;
	t.tv_usec = TIMEOUT;
	for (int i = 0; i < TOTAL_SOCKETS;i ++){
		CU_ASSERT(makeSocket(i,&t) == (i+3));
	}
	for (int i = 0; i < TOTAL_SOCKETS; i++){
		close(i+3);
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
	unsigned char zeroes[] = {0x00, 0x00, 0x00, 0x00, 0x00};
	CU_ASSERT(scrapeAddr(addr,zeroes) == 1);
	unsigned char fs[] = {0x00,0xff,0xff,0xff,0xff};
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
	
}

// Function containing the tests for the scrapeMessage function
void testSCRAPEMESSAGE(void){
	CU_ASSERT(0 == 1);
}

// Function containing the tests for the unpackResponse function
void testUNPACKRESPONSE(void){
	CU_ASSERT(0 == 1);
}

int main(int argc, char ** argv){
	CU_pSuite encSuite = NULL;
	CU_pSuite udpSuite = NULL;
	if (CUE_SUCCESS != CU_initialize_registry()){
		return CU_get_error();
	}
	encSuite = CU_add_suite("Encoder",init_enc,clean_enc);
	udpSuite = CU_add_suite("UDPHandler",init_udp,clean_udp);
	if (NULL == encSuite || NULL == udpSuite){
		CU_cleanup_registry();
		return CU_get_error();
	}
	// Add encoder suite tests
	if ((NULL == CU_add_test(encSuite,"ror()",testROR)) ||
		(NULL == CU_add_test(encSuite,"rol()",testROL)) ||
		(NULL == CU_add_test(encSuite,"patternValidate()",testPatternValidate)) ||
		(NULL == CU_add_test(encSuite,"encode()",testENCODE)) ||
		(NULL == CU_add_test(encSuite,"decode()",testDECODE))){
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
	CU_basic_set_mode(CU_BRM_VERBOSE);
	CU_basic_run_tests();
	CU_cleanup_registry();
	return CU_get_error();
}
