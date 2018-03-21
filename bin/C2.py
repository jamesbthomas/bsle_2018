# Source file for C2 Program written in Python3
# TODO check permissions, make sure this is being run as superuser
import getopt, sys, os, re
from scapy.all import *
sys.path.append("../src/headers")
try:
	from packetCrafter import *
except ImportError:
	print("Error: must be run from projectroot/bin")
	sys.exit(1)

# Main function
def main(opts,args):
	print("Starting FTS C2 program...")

	# TODO FTS IP address
	ftsAddr = "10.0.0.2"
	# TODO FTS IP Address

	file = None
	path = None
	pattern = None
	phrase = None
	socket = None

	# Check to see if options were provided on the command line
	if (len(opts) < 1):
		# Run the prompt sequence
		global interactive
		interactive  = True
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
			if patternValidate(pattern):
				break
		try:
			phrase = input("Passphrase: ").strip()
		except KeyboardInterrupt:
			print("\nBye!")
			sys.exit(0)
		while True:
			try:
				socket = input("Destination Socket: ").strip()
			except KeyboardInterrupt:
				print("\nBye!")
				sys.exit(0)
			if socketValidate(socket):
				break

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
			elif switch == "-d" or switch == "--destination":
				socket = val.strip()
				if not socketValidate(socket):
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

	parts = socket.split(":")
	addr = parts[0]
	port = parts[1]

	try:
		while True:
			response = requestTransfer(addr,port,pattern,phrase,ftsAddr)
			if response != None:
				break
			elif not interactive:
				print("Failed to initiate connection...\nExiting...\nBye!")
				sys.exit(2)
			else:
				print("Failed to initiate connection...")
				while True:
					cont = input("Would you like to continue? [y/n]: ").strip()
					if cont == 'y':
						print("Trying again...")
						break
					elif cont == 'n':
						print("Exiting...\nBye!")
						sys.exit(2)
					else:
						print("Please enter either \'y\' or \'n\'")
						continue
	except KeyboardInterrupt:
		print("Bye!")
		sys.exit(0)

	# TODO extract TCP Port (9-10) and validation message (11+)

	# TODO confirm validation message == initialization message
		## If it does, add the FSS to the list of valid FSS (addr,port,pattern)

	# TODO Data Transfer
		## TCP Three-way Handshake
		## Data transfer
		## TCP Close

	print("Bye!")
	return 0

def requestTransfer(addr,port,pattern,phrase,ftsAddr):
	crafter = PacketCrafter()
	requestPacket = crafter.craftRequest(addr,port,pattern,phrase)
	for ftsPort in range(16000,17001):
		if ftsPort%100 == 0 and not verbose and ftsPort != 16000:
			print("Trying to connect... 16000 -",ftsPort," unavailable")
		if verbose:
			print("Trying port ",ftsPort,". . .")
		request = IP(dst=ftsAddr) / UDP(dport=ftsPort) / requestPacket
		response = sr1(request,timeout=0.1,verbose=False)
		if response:
			print("Connection established!")
			return response
		if verbose:
			print("Timed out...")
	return None

# Usage function to print switches and input
def usage():
	print("Usage: python3 C2.py [-hv] -e '<encoding pattern>' -p <passphrase> -d <ip>:<port> [-f] <file>")
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

# Function to validate that a provided file exists
## Takes the filepath as input and returns a file object if the file exists, returns None otherwise
def fileValidate(path):
	try:
		f = open(path,"rb")
		return f
	except FileNotFoundError as err:
		print("File not found")
		return None

# Function to validate that a provided socket is valid
## Return Value 0 - Socket is not valid
## Return Value 1 - Socket is valid
def socketValidate(socket):
	parts = socket.split(":")
	if (len(parts) != 2):
		print("Error: Invalid Socket Format")
		return 0
	# Check IP Address
	try:
		for oct in re.match('([0-9]{1,3})\.([0-9]{1,3})\.([0-9]{1,3})\.([0-9]{1,3})',parts[0]).groups():
			assert(int(oct) <= 255)
			assert(int(oct) >= 0)
		if parts[0] == "0.0.0.0" or parts[0] == "255.255.255.255":
			# Check for any and broadcast, no reason to use these
			raise Exception
	except Exception:
		print("Error: Invalid IP Address")
		return 0
	# Check Port Number
	if (int(parts[1]) > 65535 or int(parts[1]) < 1):
		print("Error: Invalid Port Number")
		return 0
	return 1

if __name__ == "__main__":
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
		opts.index(('-h',''))
		help()
		sys.exit(0)
	except ValueError:
		try:
			opts.index(("--help",''))
			help()
			sys.exit(0)
		except ValueError:
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
## 2 - could not complete initialization sequence (UDP)
