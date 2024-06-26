// C encoder header
#ifndef ENCODER_HEADER
#define ENCODER_HEADER
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include "../headers/encoder.h"

// Data structure for a parsed encoding pattern
typedef struct pattern{
	int numOpts;
	int maxOpts;
	char ** opts;
	char * ops;
	int * vals;
	int * lens;
} Pattern;

// Dictionary used for base 64 encoding/decoding
char dict[] = {'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O',
			'P','Q','R','S','T','U','V','W','X','Y','Z','a','b','c','d',
			'e','f','g','h','i','j','k','l','m','n','o','p','q','r','s',
			't','u','v','w','x','y','z','0','1','2','3','4','5','6','7',
			'8','9','+','/'};

// Function to validate an encoding pattern scraped from the C2's request packet and parse it for easier use later
// Takes the scraped pattern as a string and the Pattern object that will store the parsed pattern
// Returns 0 if successful, 1 if failed
int patternValidate(char * pattern,Pattern * parsed){
	parsed -> numOpts = 0;
	parsed -> maxOpts = 2;
	parsed -> opts = calloc(parsed->maxOpts,sizeof(char *));
	// Build out the options
	char * token = strtok(pattern,";");
	while (token != NULL){
		// Check to see if we need to allocate more space for the options
		if (parsed->numOpts >= parsed->maxOpts){
			parsed->maxOpts = parsed->maxOpts * 2;
			parsed->opts = (char **) realloc(parsed->opts,parsed->maxOpts*sizeof(char *));
			if (parsed->opts == NULL){
				// realloc failure
				return 1;
			}
		}
		// Assign the option and increase the number of options
		parsed->opts[parsed->numOpts] = token;
		parsed->numOpts += 1;
		token = strtok(NULL,";");
	}
	free(token);
	// Make sure there are at least two options in the pattern
	if (parsed->numOpts < 2){
		return 1;
	}
	// Allocate space for the rest of the struct and some stuff to process the opts
	int i;
	char * opVal;
	parsed->ops = (char *) calloc(parsed->numOpts,sizeof(char *));
	parsed->vals = (int *) calloc(parsed->numOpts+1,sizeof(int));
	parsed->lens = (int *) calloc(parsed->numOpts+1,sizeof(int));
	for (i = 0;i < parsed->numOpts;i++){
		// Create a duplicate of the option that we can tokenize
		char * duppedOpts = strdup(parsed->opts[i]);
		opVal = strtok(duppedOpts,":");
		char * len = strtok(NULL,":");
		// Check the format of the option
		if (len == NULL){
			free(duppedOpts);
			return 1;
		}
		// Validate the length from the pattern and capture it
		int num = atoi(len);
		if (num == 0 && strncmp(len,"0",1) != 0){
			free(duppedOpts);
			return 1;
		}
		parsed->lens[i] = num;
		// Another format check
		if (strtok(NULL,":") != NULL){
			free(duppedOpts);
			return 1;
		}
		// handle the ^ operator
		if (strncmp(opVal,"^",1) == 0){
			parsed->ops[i] = opVal[0];
			// format check
			if (strlen(opVal) < 2){
				free(duppedOpts);
				return 1;
			}
			// Convert the value of this option to something useable
			char * val = calloc(strlen(opVal),sizeof(char));
			for (int c = 1;c < strlen(opVal);c++){
				if (isdigit(opVal[c]) == 0){
					free(duppedOpts);
					return 1;
				}
				val[c-1] = opVal[c];
			}
			// Capture the value
			val[strlen(opVal)-1] = '\0';
			parsed->vals[i] = atoi(val)%8;
			free(val);
		}
		// handle the ror operator
		else if (strncmp(opVal,"ror",3) == 0){
			parsed->ops[i] = opVal[2];
			// format check
			if (strlen(opVal) < 4){
				free(duppedOpts);
				return 1;
			}
			// convert the value to something useable
			char * val = calloc(strlen(opVal)-2,sizeof(char));
			for (int c = 3;c < strlen(opVal);c++){
				if (isdigit(opVal[c]) == 0){
					free(duppedOpts);
					return 1;
				}
				val[c-3] = opVal[c];
			}
			// capture the value
			val[strlen(opVal)-3] = '\0';
			parsed->vals[i] = atoi(val)%8;
			free(val);
		}
		// handle the rol operator
		else if (strncmp(opVal,"rol",3) == 0){
			parsed->ops[i] = opVal[2];
			// format check
			if (strlen(opVal) < 4){
				free(duppedOpts);
				return 1;
			}
			// convert the value to something useable
			char * val = calloc(strlen(opVal)-2,sizeof(char));
			for (int c = 3;c < strlen(opVal);c++){
				if (isdigit(opVal[c]) == 0){
					free(duppedOpts);
					return 1;
				}
				val[c-3] = opVal[c];
			}
			// capture the value
			val[strlen(opVal)-3] = '\0';
			parsed->vals[i] = atoi(val)%8;
			free(val);
		}
		// handle the ~ operator
		else if (strncmp(opVal,"~",1) == 0){
			// format check
			if (strlen(opVal) != 1){
				free(duppedOpts);
				return 1;
			}
			// capture the value
			parsed->ops[i] = opVal[0];
			parsed->vals[i] = 0;
		}
		// handle invalid operators
		else{
			free(duppedOpts);
			return 1;
		}
		free(duppedOpts);
	}
	return 0;
}

// AUTHOR NAME: dasblinkenlight (stackoverflow username)
// WEB ADDRESS: https://ideone.come/pq4Ejs - linked from https://stackoverflow.com/questions/21289606/rotation-of-unsigned-char-by-n-bits
// DATE ACCESSED: 28MAR2018
// Altered slightly to match what I needed
unsigned char ror(const unsigned char value, int shift){
	return (value >> shift%8 & 0xFF) | (value << (8 - shift%8) & 0xFF);
}

// AUTHOR NAME: dasblinkenlight (stackoverflow username)
// WEB ADDRESS: https://ideone.come/pq4Ejs - linked from https://stackoverflow.com/questions/21289606/rotation-of-unsigned-char-by-n-bits
// DATE ACCESSED: 28MAR2018
// Altered slightly to match what I need
unsigned char rol(const unsigned char value, int shift){
	return (value << shift%8 & 0xFF) | (value >> (8 - shift%8) & 0xFF);
}

// Function to encode a string using a provided parsed encoding pattern
// Returns encoded string if successful, NULL otherwise
unsigned char * encode(unsigned char * data,Pattern * parsed){
	// allocate space for stuff that this function needs
	int currOpt = 0;
	char * cdata = (char *) data;
	int size = strlen(cdata);
	int done = 0;
	unsigned char * encoded = calloc(size+1,sizeof(unsigned char));
	// while the number of characters weve encoded is less than the total number of characters we need to encode
	while (done < size){
		// grab the operator value and length of the current encoding option
		char op = parsed->ops[currOpt];
		int val = parsed->vals[currOpt];
		int len = parsed->lens[currOpt];
		// encode as many characters as this encoding option is supposed to handle
		for (int count = 0;count < len;count++){
			if (done > size){
				return encoded;
			}
			if (op == '^'){
				encoded[done] = val ^ data[done];
			}
			else if (op == 'r'){
				unsigned char shifted = ror(data[done],val);
				encoded[done] = shifted;
			}
			else if (op == 'l'){
				unsigned char shifted = rol(data[done],val);
				encoded[done] = shifted;
			}
			else if (op == '~'){
				encoded[done] = ~data[done];
			}
			else{
				// invalid operator
				free(encoded);
				return NULL;
			}
			done += 1;
		}
		// increment currOpt to move to the next encoding option
		currOpt += 1;
		// enable encoding to wrap back around to the beginning of the pattern if there is still more to encode
		if (currOpt > parsed->numOpts){
			currOpt = 0;
		}
	}
	// null terminate the encoded string
	encoded[size] = '\0';
	return encoded;
}

// Function to decode a string using a provided parsed encoding pattern
// Returns decoded string if successful, NULL otherwise
unsigned char * decode(unsigned char * data,Pattern * parsed){
	// initial allocation
	int currOpt = 0;
	char * cdata = (char *) data;
	int size = strlen(cdata);
	int done = 0;
	unsigned char * decoded = calloc(size+1,sizeof(unsigned char));
	// while the number of decoded characters is less than the total number of characters
	while (done < size){
		// grab the operator value and length of the current encoding option
		char opt = parsed->ops[currOpt];
		int val = parsed->vals[currOpt];
		int len = parsed->lens[currOpt];
		// use the operator and value as many times as len specified
		for (int count = 0;count < len;count++){
			if (done > size){
				return decoded;
			}
			if (opt == '^'){
				decoded[done] = val ^ data[done];
			}
			else if (opt == 'r'){
				unsigned char shifted = rol(data[done],val);
				decoded[done] = shifted;
			}
			else if (opt == 'l'){
				unsigned char shifted = ror(data[done],val);
				decoded[done] = shifted;
			}
			else if (opt == '~'){
				decoded[done] = ~data[done];
			}
			else{
				printf("Error: Invalid Parsed Pattern\n");
				return NULL;
			}
			done += 1;
		}
		// move to the next encoding option
		currOpt += 1;
		// enable the function to wrap back to the beginning of the encoding pattern
		if (currOpt > parsed->numOpts){
			currOpt = 0;
		}
	}
	// null terminate the decoded string and return
	decoded[size] = '\0';
	return decoded;
}

// Function to encode a provided string using base64
// Returns encoded string if successful, NULL otherwise
// Used by each transfer session to create its log entry
unsigned char * encode64(unsigned char * string){
	// find the length of the base64 encoded string based on the length of the input string
	int len = strlen((char *) string);
	int elen;
	if (len % 3 == 0){
		elen = ((int) len/3)*4;
	}
	else {
		elen = (((int) len/3)+1)*4;
	}
	// allocate space for it
	unsigned char * encoded = calloc(elen+2,sizeof(unsigned char));
	// encode
	int enc = 0;
	for (int plain = 0;plain < len;plain+=3){
		// If we need to pad twice
		if (plain+1 >= len){
			int together = string[plain] << 4;
			encoded[enc] = dict[together >> 6];
			encoded[enc+1] = dict[together & 0x3f];
			encoded[enc+2] = 0x3d;
			encoded[enc+3] = 0x3d;
		}
		// If we need to pad once
		else if (plain+2 >= len){
			int together = string[plain] << 10 | string[plain+1] << 2;
			encoded[enc] = dict[together >> 12];
			encoded[enc+1] = dict[together >> 6 & 0x3f];
			encoded[enc+2] = dict[together & 0x3f];
			encoded[enc+3] = 0x3d;
		}
		// No padding
		else{
			int together = string[plain] << 16 | string[plain+1] << 8 | string[plain+2];
			encoded[enc] = dict[together >> 18];
			encoded[enc+1] = dict[together >> 12 & 0x3f];
			encoded[enc+2] = dict[together >> 6 & 0x3f];
			encoded[enc+3] = dict[together & 0x3f];
		}
		enc += 4;
	}
	// add a newline, null terminate, and return
	encoded[elen] = '\n';
	encoded[elen+1] = '\0';
	return encoded;
}

// Function to decode a provided string using base64
// Returns decoded string if successful, NULL otherwise
// Used by the command prompt thread to query the log
unsigned char * decode64(unsigned char * string){
	// calculate the length of the decoded string
	int len = strlen((char *) string);
	int dlen = len;
	// if it was padded twice
	if (string[len-1] == 61){
		dlen -= 2;
	}
	// if it was padded once
	else if (string[len] == 61){
		dlen -= 1;
	}
	// allocate
	unsigned char * decoded = calloc(dlen,sizeof(unsigned char));
	int plain = 0;
	// decode
	for (int enc = 0;enc < len;enc += 4){
		if (dlen == len-1 && enc+4 == len){
			int together = (((strchr(dict,string[enc])-dict) & 0x3f) << 18) | (((strchr(dict,string[enc+1])-dict) & 0x3f) << 12) | ((strchr(dict,string[enc+2])-dict) & 0x3f);
			decoded[plain] = together >> 16;
			decoded[plain+1] = together >> 8 & 0xff;
		}
		else if (dlen == len-2 && enc+4 == len){
			int together = (((strchr(dict,string[enc])-dict) & 0x3f) << 18) | (((strchr(dict,string[enc+1])-dict) & 0x3f) << 12);
			decoded[plain] = together >> 16;
		}
		else {
			int together = (((strchr(dict,string[enc])-dict) & 0x3f) << 18) | (((strchr(dict,string[enc+1])-dict) & 0x3f) << 12) | (((strchr(dict,string[enc+2])-dict) & 0x3f) << 6) | ((strchr(dict,string[enc+3])-dict) & 0x3f);
			decoded[plain] = together >> 16;
			decoded[plain+1] = together >> 8 & 0xff;
			decoded[plain+2] = together & 0xff;
		}
		plain += 3;
	}
	return decoded;
}

#endif
/* Main function for interacting directly with this module
int main(int argc,char ** argv){
	unsigned char * pas = "pass";
	unsigned char * encPas = calloc(8,sizeof(unsigned char));
	encPas[0] = 0x63;
	encPas[1] = 0x47;
	encPas[2] = 0x46;
	encPas[3] = 0x7a;
	encPas[4] = 0x63;
	encPas[5] = 0x77;
	encPas[6] = 0x3d;
	encPas[7] = 0x3d;
	printf("%s\t%s\t%d\n",encPas,decode64(encPas),strlen((char *) decode64(encPas)));
	return 0;
}*/
