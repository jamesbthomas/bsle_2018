# Project Challenges

This document details the challenges I encountered, and any design issues and/or security flaws associated with this project.

## Challenges
	My first challenge associated with this project came in the construction of my repo. Most of my experience using git comes from personal projects where it was always easier for me to do my development in a single branch, and adjusting to the requirement to work entirely in branches was a bit of an adjustment. In order to overcome this challenge, I started small with my first "initial" branch, to use a test case to ensure I was doing everything correctly and I understood the workflow before moving into feature-oriented branches for development.

## Design Issues

## Security Flaws
### 1) C2 -> FTS is vulnerable to man-in-the-middle
Because everything between the C2 and FTS is sent unencrypted, including the encryption pattern and validation message, any variation on the classic man-in-the-middle attack poses a massive security flaw.

