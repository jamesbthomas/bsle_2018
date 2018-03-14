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
  //TODO - expand description, build requirements table to use as a reference during development

## /bin/FTS

This binary file is the executable for the File Transfer Service module. This module receives unencrypted configuration messages and files from the C2 program, encrypt the file using the provided configurations, and send the encrypted file to the FSS.
### Usage - TBD
### Purpose - receive unencrypted files from C2, encrypt, and send to FSS
  //TODO - expand description, build requirements table to use as a reference during development

## /bin/C2.py

This python files contains the main code for the Command and Control module. This module sends unencrypted configuration information, mainly an encryption pattern and validation message, and an unecrypted file to the FTS for encryption and storage.
### Usage - TBD
### Purpose - user interface with the FTS
  //TODO - expand description, build requirements table to use as a reference during development
