// C encoder header
#ifndef ENCODER_HEADER
#define ENCODER_HEADER
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// TODO typedef a struct to hold the parsed pattern

// Function to validate an encoding pattern scraped from the C2's request packet
// Returns 1 if equal, 0 if not
int patternValidate(char * pattern){
	int maxOpts = 2;
	char ** opts = calloc(maxOpts,sizeof(char *));
	char * token = strtok(pattern,";");
	int numOpts = 0;
	while (token != NULL){
		if (numOpts > maxOpts){
			maxOpts = maxOpts * 2;
			opts = (char **) realloc(opts,maxOpts);
			if (opts == NULL){
				printf("Error: Realloc Failure\n");
				exit(2);
			}
		}
		opts[numOpts] = calloc(strlen(token),sizeof(char));
		opts[numOpts] = token;
		numOpts += 1;
		token = strtok(NULL,";");
	}
	for (int i = 0;i < numOpts;i++){
		printf("%d\t",i);
		printf("%s\n",opts[i]);
	}
	return 1;
}

#endif
// Main function for interacting directly with this module
int main(int argc,char ** argv){
	char *data = strdup("ror1:2;^27:2;~:180;rol50:34");
	patternValidate(data);
	return 0;
}
