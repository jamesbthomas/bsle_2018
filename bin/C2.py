# Source file for C2 Program written in Python3
import getopt, sys, os, re

# Main function
def main(opts,args):
	print("Starting FTS C2 program...")

	file = None
	path = None
	pattern = None
	phrase = None

	# Check to see if options were provided on the command line
	if (len(opts) < 1):
		# Run the prompt sequence
		while True:
			try:
				path = input("Path to file: ").strip()
			except KeyboardInterrupt:
				print("\nBye!")
				sys.exit(0)
			file = fileValidate(path)
			if file != None:
				break

		while True:
			try:
				pattern = input("Encoding Pattern: ").strip()
			except KeyboardInterrupt:
				print("\nBye!")
				sys.exit(0)
			if not patternValidate(pattern):
				break
		try:
			phrase = input("Passphrase: ").strip()
		except KeyboardInterrupt:
			print("\nBye!")
			sys.exit(0)
	else:
		# Initialize variables based on contents in opts and args
		for switch,val in opts:
			if switch == "-e" or switch == "--encode":
				pattern = val.strip()
				if not patternValidate(pattern):
					sys.exit(1)
			elif switch == "-p" or switch == "--passphrase":
				phrase = val.strip()
			elif switch == "-f" or switch == "--file":
				path = val.strip()
				file = fileValidate(path)
				if not file:
					sys.exit(1)

		# If user didnt provide file through a switch, check args
		if not path:
			try:
				path = args[0].strip()
				file = fileValidate(path)
				if not file:
					sys.exit(1)
			except IndexError as err:
				print("Error parsing command line options: "+str(err))
				sys.exit(1)

	# TODO IPAddWhiteList
		## Managed from the command line maybe? But how do we ensure the FSS is valid?

	# TODO UDP Connection to FTS w/ initialization message, encryption pattern, destination port, destination address
		## Wait to receive unencrypted validation message
		## Timeout
		## IP Whitelist of valid FSS

	# TODO Data Transfer
		## TCP Three-way Handshake
		## Data transfer
		## TCP Close

	print("Bye!")
	return 0

# Usage function to print switches and input
def usage():
	print("Usage: python3 C2.py [-h] (-e |--encode=) '<encoding pattern>' (-p |--passphrase=) <passphrase> [-f |--file=] <file>")
	return 0

# Help function called by --help and -h
def help():
	print("Command and Control for FTS Help Menu")
	usage()
	print("This program takes input from the command line using the switches detailed below,\nor from a series of prompts when run in interactive mode.\nTo run in interactive mode, simply enter 'python3 C2.py'")
	print("\nSwitches - ")
	print("\t -e <pattern>\t same as --encode, specifies the encoding pattern to be used by the FTS server when sending the file to the FSS server")
	print("\t\t\t NOTE - the encoding pattern MUST be encapsulated in single quotes and contains two options separated by a semicolon")
	print("\t\t\t\t Each option contains two parts - an operation and a number of bytes to perform the operation on")
	print("\t\t\t\t There are four options available - Bitwise XOR (^), Bitwise NOT (~), Rotate Right (ror), and Rotate Left (rol)")
	print("\t -p <phrase>\t same as --passphrase, specifies the passphrase to be used to initialize the file transfer session")
	print("\t -f <filepath>\t optional, same as --file, specifies which file you want to transfer")
	print("\t -h\t\t same as --help, displays this menu")
	# TODO add a -v verbose option
	return 0

# Function to validate that a provided file exists
## Takes the filepath as input and returns a file object if the file exists, returns None otherwise
def fileValidate(path):
	try:
		f = open(path,"rb")
		return f
	except FileNotFoundError as err:
		print("File not found")
		return None

# Function to validate that a provided encoding pattern is legitimate
## Return Value 0 - encode pattern is not valid
## Return Value 1 - encode pattern is valid
def patternValidate(pattern):
	options = pattern.split(";")
	if (len(options) < 2):
		print("Error: Insufficient Encoding Options")
		return 0
	for set in options:
		if not re.match('(~|\^\d+|ror\d+|rol\d+):\d+',set):
			print("Error: Invalid Encoding Pattern")
			return 0
	return 1

if __name__ == "__main__":
	# Check for command line args and pass to main
	try:
		opts,args = getopt.getopt(sys.argv[1:],"he:p:f:",["help","encode=","passphrase=","file="])
	except getopt.GetoptError as err:
		print(err)
		usage()
		sys.exit(1)

	try:
		opts.index(('-h',''))
		help()
		sys.exit(0)
	except ValueError as err:
		pass

	main(opts,args)
	sys.exit(0)

# Exit Codes
## 0 - success
## 1 - input error
