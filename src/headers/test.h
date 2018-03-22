// C Source file for Unit Tester
#ifndef UNIT_TESTER
#define UNIT_TESTER
#include <string.h>

// Function to test whether two strings are equal
// Returns 1 if equal, 0 if not
int assertStrEqual(char * first, char * second){
	if (strlen(first) != strlen(second)){
		return 0;
	}
	int len = strlen(first);
	if (strncmp(first,second,len)){
		return 0;
	}
	return 1;
}

// Function to test whether two ints are equal
// Returns 1 if equal, 0 if not
// Kinda gratuitous but I think it'll help when reading
int assertIntEqual(int first, int second){
	return first == second;
}

#endif
// Main for testing this module
//int main(int argc, char ** argv){
//	return 0;
//}
