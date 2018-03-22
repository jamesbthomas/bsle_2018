# Python3 Source File for File Storage Service Main Function
## Each FSS instance supports exactly one encoding pattern specified by the user at runtime
# TODO check permissions, should be run as superuser
# TODO change directory around to support imports
# TODO TEST!!!!
import getopt, sys, socket
sys.path.append("../src/headers")
try:
	from packetCrafter import *
	from encoder import *
	from fileHandler import *
	from tcpHandler import *
except ImportError:
	print("Error: Must be run from projectroot/bin")
	sys.exit(1)

def main(opts):
	# Main function
	# Takes options from getopt
	pattern = None
	port = None
	destination = None
	crafter = PacketCrafter()
	handler = FileHandler()
	verbose = False

	for switch,val in opts:
		if switch == "-e" or switch == "--encode":
			if patternValidate(val):
				pattern = val
		elif switch == "-d" or switch == "--destination":
			destination = val
		elif switch == "-p" or switch == "--port":
			if int(val) > 65535 or int(val) < 1:
				print("Error: Invalid Port Number")
				sys.exit(1)
			port = int(val)
		elif switch == "-v" or switch == "--verbose":
			verbose = True
		else:
			print("Error: Invalid Switch")
			usage()
			sys.exit(1)

	if pattern == None or port == None:
		print("Error: Failed to assign values")
		sys.exit(2)

	encoder = Encoder(pattern)

	try:
		message = None
		while message == None:
			init = sniff(filter="udp dst port "+str(port),count=1)[0]
			message,ftsAddr,ftsPort = crafter.unpackInit(init)
			if verbose:
				print("Received message from "+ftsAddr+":"+str(ftsPort)+" - "+message)
		while True:
			pass
	except IndexError: # Round-about way to keyboard interrupt the sniff
		# CTRL+C will stop sniff but not throw the Interrupt into the main thread
		# If you CTRL+C, then it will IndexError when trying to index into the packets that were sniffed
		print("\nBye!")
		sys.exit(0)

	decoded = encoder.decode(message)
	recoded = encoder.encode(decoded)
	if verbose:
		print(message+" -> "+decoded+" -> "+recoded)

	if message != decoded:
		print("Error: Decode/Recode Failure")
		sys.exit(2)

	# Find an open TCP Port in the ephemeral range (49152-65535), open the socket to hold the IP address until we're ready to send/sniff
	sock = None
	for port in range(49152,65536):
		sock = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
		result = sock.connect_ex(('localhost',port))
		if result == 0:
			break

	if verbose:
		print("Local Port - "+str(port))

	# Craft Packet Type 0x02
	response = crafter.craftResponse(port,recoded)
	tcp = TCPHandler(ftsAddr,-1,verbose)
	# Close socket, send packet, get ready for the shake
	sock.close()
	ready = False
	while not ready:
		ready,path = tcp.shake(port)
	# Receive the file
	if verbose:
		print("Waiting for file...")
	if tcp.recvFile(path) < 1:
		sys.exit(2)
	print("Bye!")
	sys.exit(0)

# Function to print usage information
def usage():
	print("Usage: sudo python3 FSS.py [-h] -e '<encoding pattern>' -p <port>")
	return 0

# Help function called by --help and -h
def help():
	print("File Storage Service for FTS Help Menu")
	usage()
	print("This program takes input from the command line to specify a number of parameters that define the operation of the FSS")
	print("\nSwitches - ")
	print("\t -e <pattern> \t same as --encode, specifies the encoding pattern that this FSS will support")
	print("\t -p <port> \t same as --port, specifies the port to listen on")
	print("\t -d <path> \t optional, same as --destination, specify the destination directory for the FSS to store files")
	print("\t -v\t\t same as --verbose, display verbose output")
	print("\t -h\t\t same as --help, display this menu")
	return 0

if __name__ == "__main__":
	try:
		opts,args = getopt.getopt(sys.argv[1:],"e:p:hd:v",["help","encode=","port=","destination=","verbose"])
		opts.index(('-h',''))
		help()
		sys.exit(0)
	except getopt.GetoptError as err:
		print(err)
		usage()
		sys.exit(1)
	except ValueError:
		try:
			opts.index(('--help',''))
			help()
			sys.exit(0)
		except ValueError:
			pass

	if len(opts) < 2:
		usage()
		sys.exit(1)

	if len(args) > 0:
		print("Error: Unexpected Arguments")
		usage()
		sys.exit(1)

	main(opts)
	sys.exit(0)
