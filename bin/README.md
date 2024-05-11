# /bin

This directory stores the binary files for this project. Each file's entry in this document contains a brief description of its purpose and usage.

## /bin/IPAddWhiteList

Not sure what this is for yet, but from the name it sounds like this is a whitelist of approved ip addresses, presumably for approved FSS servers and used by the FTS.
### Usage - unknown
### Purpose - unknown
 //TODO - figure out what this is for

## /bin/FSS.py

This python file contains the main code for the remote File Storage System module. This module receives encrypted files over TCP from the FTS and stores them remotely.
### Usage - TBD
### Purpose - provide storage for the file transfer service
### Requirements
- Support file transfer from C2 to FSS via FTS //TODO
- Python3
- Support TCP and UDP communications //TODO
- Support custom communication protocol //TODO
- Support encode options //TODO
- Decrypt files before writing to disk //TODO
- Support connection timeout //TODO
- Support command line arguments and prompted input //TODO
- Encode data sent to the FSS //TODO
- Decode data sent to the C2 //TODO
- Descriptive output
- Modular
- Tested
- Input validation //TODO

## /bin/FTS

This binary file is the executable for the File Transfer Service module. This module receives unencrypted configuration messages and files from the C2 program, encrypt the file using the provided configurations, and send the encrypted file to the FSS.
### Usage - TBD
### Purpose - receive unencrypted files from C2, encrypt, and send to FSS
### Requirements
- Multithreaded in C //TODO
- Support TCP and UDP //TODO
- Log all session information, including: C2 IP address and port, FSS IP and port, encode pattern, name of file received, size of file received, size of file transferred //TODO
- Support custom communication protocol //TODO
- Support ability to receive multiple files at one time from different C2s using TCP //TODO
- Encode using the pattern from the C2s request packet //TODO
- Support transferring files from the FTS to multiple FSS at the same time //TODO
- Support file transfer from C2 to FSS via FTS
- Attempt to reconnect to the FSS in the event of a connection timeout //TODO
- Descriptive output
- Modular
- Tested
- Input validation //TODO

## /bin/C2.py

This python files contains the main code for the Command and Control module. This module sends unencrypted configuration information, mainly an encryption pattern and validation message, and an unecrypted file to the FTS for encryption and storage.
### Usage - TBD
### Purpose - user interface with the FTS
### Requirements
- Python3
- Support TCP and UDP communications //TODO
- Support custom communication protocol //TODO
- Support file transfer to FSS via FTS //TODO
- Support prompted input and command line options //TODO
- Support connection timeout //TODO
- Descriptive output
- Modular
- Tested
- Input Validation //TODO
