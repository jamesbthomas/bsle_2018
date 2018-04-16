# Source file for C2 Program written in Python3
import getopt, sys, os, re, logging, time
file_loc = os.path.dirname(os.path.realpath(__file__)) # Find the directory this file is stored in
headers_dir = "/".join(file_loc.split("/")[:-1])+"/src/headers"	# Location of the header file
sys.path.append(headers_dir)
try:
	from udpHandler import *
	from fileHandler import *
	from tcpHandler import *
except ImportError as err:
	print(err)
	sys.exit(2)

# Main function
def main(opts,args):
	print("Starting FTS C2 program...")

	# TODO FTS IP address
	ftsAddr = "127.0.0.1"
	# TODO FTS IP Address

	file = None
	path = None
	pattern = None
	phrase = None
	socket = None

	# Prep the file handler
	handler = FileHandler()

	# Check to see if options were provided on the command line
	if (len(opts) < 1):
		# Run the prompt sequence if they were not
		global interactive
		interactive  = True
		global verbose
		verbose = True
		# Grab the path to the file to send
		while True:
			try:
				file = input("Path to file: ").strip()
			except KeyboardInterrupt:
				print("\nBye!")
				sys.exit(0)
			if handler.fileValidate(file):
				break
		# Grab the encoding pattern
		while True:
			try:
				pattern = input("Encoding Pattern: ").strip()
			except KeyboardInterrupt:
				print("\nBye!")
				sys.exit(0)
			if patternValidate(pattern):
				break
		# grab the pass phrase
		try:
			phrase = input("Passphrase: ").strip()
		except KeyboardInterrupt:
			print("\nBye!")
			sys.exit(0)

		# grab the destination file name
		try:
			destName = input("Name on the FSS: ").strip().split('/')[-1]
		except KeyboardInterrupt:
			print("\nBye!")
			sys.exit(0)

		# choose a destination fss
		socket = choose(pattern)

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
				file = val.strip()
				if not handler.fileValidate(file):
					sys.exit(1)
			elif switch == "-d" or switch == "--destination":
				socket = val.strip()
				if not socketValidate(socket):
					sys.exit(1)
			elif switch == "-s" or switch == "--store-as":
				destName = val.strip().split('/')[-1]

	# Now that we have all our user input, break it into the parts we need and create the modules we need for the UDP initialization sequence
	parts = socket.split(":")
	addr = parts[0]
	fssPort = parts[1]
	udp = UDPHandler()

	# Conduct the Initialization sequence
	try:
		while True:
			# Request the transfer, within udpHandler this function sends 0x00, receives 0x03, and handles 0x04
			response = requestTransfer(addr,fssPort,pattern,phrase,ftsAddr,destName,verbose)
			if response != None:
				break
			elif not interactive:
				# Close if run from the command line
				print("Failed to initiate connection...\nExiting...\nBye!")
				sys.exit(3)
			else:
				# Prompt to try again if run interactively
				print("Failed to initiate connection...")
				while True:
					cont = input("Would you like to continue? [y/n]: ").strip()
					if cont == 'y':
						print("Trying again...")
						break
					elif cont == 'n':
						print("Exiting...\nBye!")
						sys.exit(3)
					else:
						print("Please enter either \'y\' or \'n\'")
						continue
	except KeyboardInterrupt:
		# Allow the user to force closure and gracefully exit
		print("Bye!")
		sys.exit(0)

	# Extract TCP port and verify validation message
	tcpPort = udp.unpackValidation(response,phrase)
	if not tcpPort:
		sys.exit(3)

	# Add a now valid FSS to the whitelist
	if not handler.contains(file_loc+"/IPAddWhiteList",addr):
		if handler.add(file_loc+"/IPAddWhiteList",addr+","+fssPort+","+pattern) != len(addr+","+fssPort+","+pattern+"\n"):
			print("ERROR: Internal - Failed to add to IPAddWhiteList")
			sys.exit(2)

	# Data Transfer
	tcp = TCPHandler(ftsAddr,tcpPort,pattern,verbose)
		## TCP Three-way Handshake
	if not tcp.handshake():
		print("ERROR: TCP Handshake Failed")
		sys.exit(4)

	## Data transfer
	fsize = os.stat(file).st_size
	if tcp.sendFile(file,fsize) != fsize:
		print("WARNING: Total bytes sent does not match file size")
	## TCP Close
	tcp.socket.close()

	print("Bye!")
	return 0

# Function to facilitate interactively choosing a FSS
def choose(pattern):
	output=[]
	try:
		with open(file_loc+"/IPAddWhiteList") as f:
			# Read the IPAddWhiteList file for all FSSs that match the encoding pattern
			num = 1
			for line in f:
				parts=line.split(",")
				if len(parts) != 3:
					print("WARNING: IPAddWhiteList Entry Misformatted - "+line)
					continue
				# Check to see if this entry matches the encoding pattern for this session
				if parts[2].rstrip() == pattern:
					# Format the IPAddWhiteList entry to make it easier to read
					output.append("   "+str(num)+")\t"+parts[0]+":"+parts[1]+"\n")
					num += 1
		print("")
		for i in range(len(output)):
			# Print the formatted output
			print(output[i])

	except FileNotFoundError:
		# Catch a missing whitelist file and create it
		f=open(file_loc+"/IPAddWhiteList","w")
		f.close()

	# Handle the case where no servers exist for that pattern
	if len(output) == 0:
		print("No valid FSS found for that encoding pattern, please manually enter a socket")
		return enterSocket()
	# Start the prompt sequence for choosing a socket from the whitelist
	else:
		try:
			while True:
				try:
					choice = int(input("Choose a socket from one listed above my entering the number to the left, or enter 0 to submit a new socket: ").strip())
					if choice >= 0 and choice <= len(output):
						# Make sure the number actually corresponds to an entry
						break
				except ValueError:
					# Catch a blank line or a non numeric choice
					print("Please enter a number indicating which server you want to send your file to")
					continue
			if choice == "0":
				# Allow manual entry
				return enterSocket()
			else:
				# format and return the socket
				entry = output[int(choice)-1].split("\t")[1] # extract the IP address and port
				return entry
		except KeyboardInterrupt:
			# Allow forceful, graceful closure
			print("Bye!")
			sys.exit(0)

# Helper for choose
def enterSocket():
	while True:
		try:
			# Accept user input
			socket = input("Destination Socket: ").strip()
		except KeyboardInterrupt:
			# Allow forceful, graceful closure
			print("\nBye!")
			sys.exit(0)
		if socketValidate(socket):
			# make sure its a valid socket
			break
	return socket



# Usage function to print switches and input
def usage():
	print("Usage: sudo python3 C2.py [-hv] -e '<encoding pattern>' -p <passphrase> -d <ip>:<port> -f <file> -s <file>")
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
	print("\t -d <ip>:<port> \t same as --destination, specifies the IP Address and port of the FSS server to send to")
	print("\t -s <file name>\t same as --store-as, specifies the name of the file on the FSS")
	print("\t -f <file path>\t same as --file, specifies which file you want to transfer")
	print("\t -v \t\t optional, same as --verbose")
	print("\t -h\t\t same as --help, displays this menu")
	return 0

# Function to validate that a provided socket is valid
## Return Value 0 - Socket is not valid
## Return Value 1 - Socket is valid
def socketValidate(socket):
	parts = socket.split(":")
	if (len(parts) != 2):
		print("ERROR: Invalid Socket Format")
		return False
	# Check IP Address
	if not addrValidate(parts[0]):
		return False
	# Check Port Number
	if (int(parts[1]) > 65535 or int(parts[1]) < 1):
		print("ERROR: Invalid Port Number")
		return False
	return True

if __name__ == "__main__":
	# Check permissions
	if os.geteuid() != 0:
		print("ERROR: Must be run as superuser")
		usage()
		sys.exit(0)
	# Global Variables to mark the session as interactive and/or verbose
	global interactive
	interactive = False
	global verbose
	verbose = False

	# Check for command line args and pass to main
	try:
		opts,args = getopt.getopt(sys.argv[1:],"vhe:p:f:d:s:",["help","encode=","passphrase=","file=","destination==","store-as=","verbose"])
	except getopt.GetoptError as err:
		# Catch switch errors
		print(err)
		usage()
		sys.exit(1)

	try:
		# Check for -h switch
		opts.index(('-h',''))
		help()
		sys.exit(0)
	except ValueError:
		try:
			# check for --help switch
			opts.index(('--help',''))
			help()
			sys.exit(0)
		except ValueError:
			pass
	except NameError: # Catches when C2 is run with no arguments
		pass

	try:
		# Check for verbose switch
		if opts.index(('-v','')):
			verbose=True
	except ValueError:
		pass

	main(opts,args)
	sys.exit(0)

# Exit Codes
## 0 - success
## 1 - input error
## 2 - internal error
## 3 - could not complete initialization sequence (UDP)
## 4 - TCP Error
