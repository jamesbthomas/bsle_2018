# Project Challenges

This document details the challenges I encountered, and any design issues and/or security flaws associated with this project.

## Challenges
	My first challenge associated with this project came in the construction of my repo. Most of my experience using git comes from personal projects where it was always easier for me to do my development in a single branch, and adjusting to the requirement to work entirely in branches was a bit of an adjustment. In order to overcome this challenge, I started small with my first "initial" branch, to use a test case to ensure I was doing everything correctly and I understood the workflow before moving into feature-oriented branches for development.

## Design Issues
### 1) FTS IP Address
In a realistic situation, the IP address of the FTS server would need to be determined dynamically. However, the prompt for this project did not specify the network layout or whcih IP address would be reserved for the FTS server. In order to complete the C2 program, I decided to assume the IP address of the FTS server would be statically assigned to 10.0.0.3 for two reasons. First, having a statically assigned IP address in the C2 program is more secure because it ensures that the program won't be vulnerable to a poisoning attack by attempting to discover the FTS server before sending the type 0x00 packet. Secondly, it more closely conforms to the project specifications that did not indicate any communications between the C2 and FTS prior to the 0x00 packet. The actual address of 10.0.0.3 was chosen based on the configuration of my VirtualBox environment - my VM runs on 10.0.0.2.
### 2) Encoding Pattern Length
In the design prompt, it indicated that a valid encoding pattern contains at least two valid encoding options, however, the specification for the communication protocol indicated that the encoding pattern field in type 0x00 packets was only four bytes long, meaning that we would need to convert the encoded string into something that will fit in four bytes. Because of this limited size, I decided to limit the number of encoding options in a valid encoding pattern to two. Doing so enabled me to design a converter that would take a string, for example "^2:40;rol1:120", to a 4-byte long number representing all of the options. The actual method of encoding is described in the design document, however, I was forced to limit the size of certain fields within the encode pattern to 1 byte in order to make them easier to configure using python's built-in "struct" class that enables byte-size configurations.

## Security Flaws
### 1) C2 -> FTS is vulnerable to man-in-the-middle
Because everything between the C2 and FTS is sent unencrypted, including the encryption pattern and validation message, any variation on the classic man-in-the-middle attack poses a massive security flaw.
