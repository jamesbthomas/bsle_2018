# Source file for C2 Program written in Python3
# TODO TEST WITH TCPHANDLER!!!
import getopt, sys, os, re, logging
# Keep scapy from throwing annoying warnings
logging.getLogger("scapy.runtime").setLevel(logging.ERROR)
from scapy.all import *
file_loc = os.path.dirname(os.path.realpath(__file__)) # Find the directory this file is stored in
headers_dir = "/".join(file_loc.split("/")[:-1])+"/src/headers"	# Location of the header file
sys.path.append(headers_dir)
try:
	from packetCrafter import *
	from fileHandler import *
	from tcpHandler import *
except ImportError as err:
	print(err)
	sys.exit(2)

# Main function
def main(opts,args):
	print("Starting FTS C2 program...")
	# Set scapy to work with the loopback address
	conf.L3socket = L3RawSocket

	# TODO FTS IP address
	ftsAddr = "127.0.0.1"
	# TODO FTS IP Address

	# TODO local port to send traffic from
	localPort = 65534
	# TODO local port to send traffic from

	file = None
	path = None
	pattern = None
	phrase = None
	socket = None

	# Prep the file handler
	handler = FileHandler()

	# Check to see if options were provided on the command line
	if (len(opts) < 1):
		# Run the prompt sequence
		global interactive
		interactive  = True
		while True:
			try:
				file = input("Path to file: ").strip()
			except KeyboardInterrupt:
				print("\nBye!")
				sys.exit(0)
			if handler.fileValidate(file):
				break
		while True:
			try:
				pattern = input("Encoding Pattern: ").strip()
			except KeyboardInterrupt:
				print("\nBye!")
				sys.exit(0)
			if patternValidate(pattern):
				break
		try:
			phrase = input("Passphrase: ").strip()
		except KeyboardInterrupt:
			print("\nBye!")
			sys.exit(0)

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

		# If user didnt provide file through a switch, check args
		if not file:
			try:
				file = args[0].strip()
				if not handler.fileValidate(file):
					sys.exit(1)
			except IndexError as err:
				print("Error parsing command line options: "+str(err))
				sys.exit(1)

	parts = socket.split(":")
	addr = parts[0]
	fssPort = parts[1]
	crafter = PacketCrafter()
	sock = makeUDP(localPort,True)
	if (sock == None):
		sys.exit(2)

	try:
		while True:
			response = requestTransfer(addr,fssPort,pattern,phrase,ftsAddr,sock,verbose)
			if response != None:
				break
			elif not interactive:
				print("Failed to initiate connection...\nExiting...\nBye!")
				sys.exit(3)
			else:
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
		print("Bye!")
		sys.exit(0)

	# Extract TCP port and verify validation message
	tcpPort = crafter.unpackValidation(response,phrase)
	print(sock)
	sys.exit(0)
	if not tcpPort:
		sys.exit(3)

	if not handler.contains(file_loc+"/IPAddWhiteList",addr):
		if handler.add(file_loc+"/IPAddWhiteList",addr+","+fssPort+","+pattern) != len(addr+","+fssPort+","+pattern+"\n"):
			print("Error: Internal - Failed to add to IPAddWhiteList")
			sys.exit(2)

	# Data Transfer
	tcp = TCPHandler()
		## TCP Three-way Handshake
	if not handshake(verbose):
		print("Error: TCP Handshake Failed")
		sys.exit(4)
		## Data transfer
	fsize = os.stat(file).st_size
	if sendFile(file,fsize,verbose) != fsize:
		print("WARNING: Total bytes sent does not match file size")
		## TCP Close
	try:
		while not close(verbose):
			print("Retrying close...")
	except KeyboardInterrupt:
		print("WARNING: Exiting without closing TCP Connection")

	print("Bye!")
	return 0

# Function to facilitate interactively choosing a FSS
def choose(pattern):
	output=[]
	try:
		with open(file_loc+"/IPAddWhiteList") as f:
			num = 1
			for line in f:
				parts=line.split(",")
				if len(parts) != 3:
					print("Internal Error: IPAddWhiteList Misformatted")
					sys.exit(3)
				if parts[2].rstrip() == pattern:
					output.append("   "+str(num)+")\t"+parts[0]+":"+parts[1]+"\t"+parts[2]+"\n")
					num += 1
		print("")
		for i in range(len(output)):
			print(output[i])

	except FileNotFoundError:
		f=open(file_loc+"/IPAddWhiteList","w")
		f.close()

	if len(output) == 0:
		print("No valid FSS found for that encoding pattern, please manually enter a socket")
		return enterSocket()
	else:
		try:
			choice = input("Choose a socket from one listed above my entering the number to the left, or enter 0 to submit a new socket: ").strip()
			if choice == "0":
				return enterSocket()
			else:
				entry = output[int(choice)-1].split("\t")[1] # extract the IP address and port
				return entry
		except KeyboardInterrupt:
			print("Bye!")
			sys.exit(0)

# Helper for choose
def enterSocket():
	while True:
		try:
			socket = input("Destination Socket: ").strip()
		except KeyboardInterrupt:
			print("\nBye!")
			sys.exit(0)

		if socketValidate(socket):
			break
	return socket



# Usage function to print switches and input
def usage():
	print("Usage: sudo python3 C2.py [-hv] -e '<encoding pattern>' -p <passphrase> -d <ip>:<port> [-f] <file>")
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
	print("\t -d <ip>:<port> /t same as --destination, specifies the IP Address and port of the FSS server to send to")
	print("\t -f <filepath>\t optional, same as --file, specifies which file you want to transfer")
	print("\t -v \t\t optional, same as --verbose")
	print("\t -h\t\t same as --help, displays this menu")
	return 0

# Function to validate that a provided socket is valid
## Return Value 0 - Socket is not valid
## Return Value 1 - Socket is valid
def socketValidate(socket):
	parts = socket.split(":")
	if (len(parts) != 2):
		print("Error: Invalid Socket Format")
		return False
	# Check IP Address
	if not addrValidate(parts[0]):
		return False
	# Check Port Number
	if (int(parts[1]) > 65535 or int(parts[1]) < 1):
		print("Error: Invalid Port Number")
		return False
	return True

if __name__ == "__main__":
	# Check permissions
	if os.geteuid() != 0:
		print("Error: Must be run as superuser")
		usage()
		sys.exit(0)
	# Global Variables to mark the session as interactive and/or verbose
	global interactive
	interactive = False
	global verbose
	verbose = False

	# Check for command line args and pass to main
	try:
		opts,args = getopt.getopt(sys.argv[1:],"vhe:p:f:d:",["help","encode=","passphrase=","file=","destination==","verbose"])
	except getopt.GetoptError as err:
		print(err)
		usage()
		sys.exit(1)

	try:
		opt.index(('-h',''))
		help()
		sys.exit(0)
	except ValueError:
		try:
			opts.index(('--help',''))
			help()
			sys.exit(0)
		except ValueError:
			pass
	except NameError: # Catches when C2 is run with no arguments
		pass

	try:
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
