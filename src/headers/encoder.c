// C encoder header
#ifndef ENCODER_HEADER
#define ENCODER_HEADER
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

typedef struct Patterns {
	int numOpts;
	int maxOpts;
	char ** opts;
	char * ops;
	int * vals;
	int * lens;
} Pattern;

// Function to validate an encoding pattern scraped from the C2's request packet and parse it for easier use later
// Takes the scraped pattern as a string and the Pattern object that will store the parsed pattern
// Returns 0 if successful, 1 if failed
int patternValidate(char * pattern,Pattern * parsed){
	parsed -> numOpts = 0;
	parsed -> maxOpts = 2;
	parsed -> opts = calloc(parsed->maxOpts,sizeof(char *));
	char * token = strtok(pattern,";");
	while (token != NULL){
		if (parsed->numOpts > parsed->maxOpts){
			parsed->maxOpts = parsed->maxOpts * 2;
			parsed->opts = (char **) realloc(parsed->opts,parsed->maxOpts);
			if (parsed->opts == NULL){ // | (parsed->ops == NULL) | (parsed->vals == NULL) | (parsed->lens == NULL)){
				printf("Error: Realloc Failure\n");
				return 1;
			}
		}
		parsed->opts[parsed->numOpts] = calloc(strlen(token),sizeof(char));
		parsed->opts[parsed->numOpts] = token;
		parsed->numOpts += 1;
		token = strtok(NULL,";");
	}
	if (parsed->numOpts < 2){
		printf("Error: Invalid Pattern\n");
		return 1;
	}
	int i;
	char * opVal;
	parsed->ops = (char *) calloc(parsed->numOpts,sizeof(char *));
	parsed->vals = (int *) calloc(parsed->numOpts,sizeof(int));
	parsed->lens = (int *) calloc(parsed->numOpts,sizeof(int));
	for (i = 0;i < parsed->numOpts;i++){
		opVal = strtok(strdup(parsed->opts[i]),":");
		char * len = strtok(NULL,":");
		int num = atoi(len);
		if (num == 0 && strncmp(len,"0",1) != 0){
			printf("Error: Invalid Length\n");
			return 1;
		}
		parsed->lens[i] = num;
		if (strtok(NULL,":") != NULL){
			printf("Error: Invalid Option\n");
			return 1;
		}
		if (strncmp(opVal,"^",1) == 0){
			parsed->ops[i] = opVal[0];
			char * val = calloc(strlen(opVal)-1,sizeof(char));
			for (int c = 1;c < strlen(opVal);c++){
				if (isdigit(opVal[c]) == 0){
					printf("Error: Invalid Value\n");
					return 1;
				}
				val[c-1] = opVal[c];
			}
			parsed->vals[i] = atoi(val);
		}
		else if (strncmp(opVal,"ror",3) == 0){
			parsed->ops[i] = opVal[2];
			char * val = calloc(strlen(opVal)-3,sizeof(char));
			for (int c = 3;c < strlen(opVal);c++){
				if (isdigit(opVal[c]) == 0){
					printf("Error: Invalid Value\n");
					return 1;
				}
				val[c-3] = opVal[c];
			}
			parsed->vals[i] = atoi(val);
		}
		else if (strncmp(opVal,"rol",3) == 0){
			parsed->ops[i] = opVal[2];
			char * val = calloc(strlen(opVal)-3,sizeof(char));
			for (int c = 3;c < strlen(opVal);c++){
				if (isdigit(opVal[c]) == 0){
					printf("Error: Invalid Value\n");
					return 1;
				}
				val[c-3] = opVal[c];
			}
			parsed->vals[i] = atoi(val);
		}
		else if (strncmp(opVal,"~",1) == 0){
			parsed->ops[i] = opVal[0];
			parsed->vals[i] = 0;
		}
		else{
			printf("Error: Invalid Operation\n");
			exit(2);
		}
	}
	return 0;
}

// AUTHOR NAME: dasblinkenlight (stackoverflow username)
// WEB ADDRESS: https://ideone.come/pq4Ejs - linked from https://stackoverflow.com/questions/21289606/rotation-of-unsigned-char-by-n-bits
// DATE ACCESSED: 28MAR2018
// Altered slightly to match what I needed
unsigned char ror(const unsigned char value, int shift){
	return (value >> shift & 0xFF) | (value << (8 - shift) & 0xFF);
}

// AUTHOR NAME: dasblinkenlight (stackoverflow username)
// WEB ADDRESS: https://ideone.come/pq4Ejs - linked from https://stackoverflow.com/questions/21289606/rotation-of-unsigned-char-by-n-bits
// DATE ACCESSED: 28MAR2018
// Altered slightly to match what I need
unsigned char rol(const unsigned char value, int shift){
	return (value << shift & 0xFF) | (value >> (8 - shift) & 0xFF);
}

// Function to encode a string using a provided parsed encoding pattern
// Returns encoded string if successfull, NULL otherwise
unsigned char * encode(char * data,Pattern * parsed){
	int currOpt = 0;
	int size = strlen(data)-1;
	printf("%d\n",size);
	int done = 0;
	unsigned char * encoded = calloc(size,sizeof(unsigned char));
	while (currOpt < parsed->numOpts){
		char opt = parsed->ops[currOpt];
		int val = parsed->vals[currOpt];
		int len = parsed->lens[currOpt];
		for (int count = 0;count < len;count++){
			printf("%d\t%c\t%c\n",done,data[done],opt);
			if (done > size){
				return encoded;
			}
			if (opt == '^'){
				encoded[done] = val ^ data[done];
			}
			else if (opt == 'r'){
				encoded[done] = ror(data[done],val);
			}
			else if (opt == 'l'){
				encoded[done] = rol(data[done],val);
			}
			else if (opt == '~'){
				encoded[done] = ~data[done];
			}
			done += 1;
		}
		currOpt += 1;
	}
	return NULL;
}

#endif
// Main function for interacting directly with this module
int main(int argc,char ** argv){
	char *data = strdup("ror1:2;^27:2;~:180;rol50:34");
	Pattern * parsed = calloc(1,sizeof(Pattern));
	patternValidate(data,parsed);
	for (int i = 0;i < parsed->numOpts;i++){
		printf("%d\t",i);
		printf("%s\t%c\t%d\t%d\n",parsed->opts[i],parsed->ops[i],parsed->vals[i],parsed->lens[i]);
	}
	printf("\n");
	printf("%s\n",encode("message",parsed));
	return 0;
}
