// C Header file for the PacketCrafter FTS Module
#ifndef CRAFTER_HEADER
#define CRAFTER_HEADER

// Struct representing the Initialization packet type 0x01 FTS->FSS
typedef struct Init{
	unsigned char packetType; 	// 1byte, 1
	unsigned char * message;	// varies, encoded initialization message
} init;

// Struct representing the Validate packet type 0x03 FTS->C2
typedef struct Validate{
	unsigned char packetType;	// 1byte , 3
	unsigned char tcpPort[2];	// 2bytes, port to establish TCP transfer connection on
	unsigned char * message;	// varies, unencoded validation message from the FSS
} validate;

#endif
