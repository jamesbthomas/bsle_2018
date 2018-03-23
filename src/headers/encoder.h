// Header file for FTS Encoder
#ifndef ENCODER_HEADER
#define ENCODER_HEADER

typedef struct pattern{
	int numOpts;
	int maxOpts;
	char ** opts;
	char * ops;
	int * vals;
	int * lens;
} Pattern;

int patternValidate(char * pattern,Pattern * parsed);

unsigned char ror(const unsigned char value, int shift);
unsigned char rol(const unsigned char value, int shift);

unsigned char * encode(char * data,Pattern * parsed);
unsigned char * decode(unsigned char * data, Pattern * parsed);

#endif
