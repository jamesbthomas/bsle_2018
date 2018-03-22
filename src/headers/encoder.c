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

// Function to validate an encoding pattern scraped from the C2's request packet
// Returns a struct containing the parsed pattern or none if invalid
Pattern * patternValidate(char * pattern){
	Pattern * parsed = calloc(1,sizeof(Pattern));
	parsed -> numOpts = 0;
	parsed -> maxOpts = 2;
	parsed -> opts = calloc(parsed->maxOpts,sizeof(char *));
	parsed -> ops = calloc(parsed->maxOpts,sizeof(char *));
	parsed -> vals = calloc(parsed->maxOpts,sizeof(int));
	parsed -> lens = calloc(parsed->maxOpts,sizeof(int));
	char * token = strtok(pattern,";");
	while (token != NULL){
		if (parsed->numOpts > parsed->maxOpts){
			parsed->maxOpts = parsed->maxOpts * 2;
			parsed->opts = (char **) realloc(parsed->opts,parsed->maxOpts);
			if (parsed->opts == NULL){
				printf("Error: Realloc Failure\n");
				exit(2);
			}
		}
		parsed->opts[parsed->numOpts] = calloc(strlen(token),sizeof(char));
		parsed->opts[parsed->numOpts] = token;
		parsed->numOpts += 1;
		token = strtok(NULL,";");
	}
	if (parsed->numOpts < 2){
		printf("Error: Invalid Pattern\n");
		exit(2);
	}
	int i;
	char * opVal;
	for (i = 0;i < parsed->numOpts;i++){
		opVal = strtok(parsed->opts[i],":");
		char * len = strtok(NULL,":");
		int num = atoi(len);
		if (num == 0 && strncmp(len,"0",1) != 0){
			printf("Error: Invalid Length\n");
			exit(2);
		}
		parsed->lens[i] = num;
		if (strtok(NULL,":") != NULL){
			printf("Error: Invalid Option\n");
			exit(2);
		}
		if (strncmp(opVal,"^",1) == 0){
			parsed->ops[i] = opVal[0];
			char * val = calloc(strlen(opVal)-1,sizeof(char));
			for (int c = 1;c < strlen(opVal);c++){
				if (isdigit(opVal[c]) == 0){
					printf("Error: Invalid Value\n");
					exit(2);
				}
				val[c-1] = opVal[c];
			}
			parsed->vals[i] = atoi(val);
			free(val);
		}
		else if (strncmp(opVal,"ror",3) == 0){
			parsed->ops[i] = opVal[2];
			char * val = calloc(strlen(opVal)-3,sizeof(char));
			for (int c = 3;c < strlen(opVal);c++){
				if (isdigit(opVal[c]) == 0){
					printf("Error: Invalid Value\n");
					exit(2);
				}
				val[c-3] = opVal[c];
			}
			parsed->vals[i] = atoi(val);
			free(val);
		}
		else if (strncmp(opVal,"rol",3) == 0){
			parsed->ops[i] = opVal[2];
			char * val = calloc(strlen(opVal)-3,sizeof(char));
			for (int c = 3;c < strlen(opVal);c++){
				if (isdigit(opVal[c]) == 0){
					printf("Errpr: Invalid Value\n");
					exit(2);
				}
				val[c-3] = opVal[c];
			}
			parsed->vals[i] = atoi(val);
			free(val);
		}
		else if (strncmp(opVal,"~",1) == 0){
			parsed->ops[i] = opVal[0];
			parsed->vals[i] = -1;
		}
		else{
			printf("Error: Invalid Operation\n");
			exit(2);
		}
	}
	for (int i = 0;i < parsed->numOpts;i++){
		printf("%d\t",i);
		printf("%s\t%c\t%d\t%d\n",parsed->opts[i],parsed->ops[i],parsed->vals[i],parsed->lens[i]);
	}
	return parsed;
}

#endif
// Main function for interacting directly with this module
int main(int argc,char ** argv){
	char *data = strdup("ror1:2;^27:2;~:180;rol50:34");
	patternValidate(data);
	return 0;
}
