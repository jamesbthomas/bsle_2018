// C Source File for Testing the FTS Encoder Module

#include <stdio.h>
#include <stdlib.h>
#include "CUnit/Basic.h"
#include "../headers/encoder.h"
//#include "../sources/encoder.c"

Pattern * full;
unsigned char * messagesFull;
Pattern * repeat;
unsigned char * messagesRepeat;

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

int main(int argc, char ** argv){
	CU_pSuite encSuite = NULL;
	if (CUE_SUCCESS != CU_initialize_registry()){
		return CU_get_error();
	}
	encSuite = CU_add_suite("Encoder",init_enc,clean_enc);
	if (NULL == encSuite){
		CU_cleanup_registry();
		return CU_get_error();
	}
	if ((NULL == CU_add_test(encSuite,"ror()",testROR)) ||
		(NULL == CU_add_test(encSuite,"rol()",testROL)) ||
		(NULL == CU_add_test(encSuite,"patternValidate()",testPatternValidate)) ||
		(NULL == CU_add_test(encSuite,"encode()",testENCODE)) ||
		(NULL == CU_add_test(encSuite,"decode()",testDECODE))){
		CU_cleanup_registry();
		return CU_get_error();
	}
	CU_basic_set_mode(CU_BRM_VERBOSE);
	CU_basic_run_tests();
	CU_cleanup_registry();
	return CU_get_error();
}
