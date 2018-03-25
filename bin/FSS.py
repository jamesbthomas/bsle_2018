# Python3 Source File for File Storage Service Main Function
## Each FSS instance supports exactly one encoding pattern specified by the user at runtime
# TODO TEST!!!!
import getopt, sys, socket, os
file_loc = os.path.dirname(os.path.realpath(__file__))
headers_dir = "/".join(file_loc.split("/")[:-1])+"/src/headers"
sys.path.append(headers_dir)
try:
	from packetCrafter import *
	from encoder import *
	from fileHandler import *
	from tcpHandler import *
except ImportError as err:
	print(err)
	sys.exit(2)

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

	sock = makeUDP(port,False);
	if (sock == None):
		sys.exit(2)

	try:
		while True:
			try:
				message = None
				while message == None:
					message, (ftsAddr,ftsPort)  = sock.recvfrom(1450)
					if (message[0] != 0x31):
						message == None
						continue
					if verbose:
						print("Received message from {0}:{1} - {2}".format(ftsAddr,ftsPort,message))
			except KeyboardInterrupt: # Round-about way to keyboard interrupt the sniff
				# CTRL+C will stop sniff but not throw the Interrupt into the main thread
				# If you CTRL+C, then it will IndexError when trying to index into the packets that were sniffed
				print("\nBye!")
				sys.exit(0)

			decoded = encoder.decode(message[1:]) # self.plainBytes.decode() utf-8 cant decode 0x8f in position 0
			if (decoded == None):
				print("Error: Could not decode message")
				sys.exit(2)
			recoded = encoder.encode(decoded)
			if verbose:
				print(message[1:],"->",decoded,"->",recoded)

			if message[1:] != recoded:
				print("Error: Decode/Recode Failure")
				sys.exit(2)

			# Prep the TCP Listener
			tcpSock = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
			tcpSock.bind(('0.0.0.0',0))
			tcpPort = tcpSock.getsockname()[1]
			tcpSock.listen(1)

			# Send the 0x02 packet
			sock.sendto(b'\x02'+tcpPort.to_bytes(2,byteorder='big')+recoded,(ftsAddr,ftsPort))
			if verbose:
				print("Local Port - "+str(tcpPort))

			if verbose:
				print("Waiting for file...")

			tcp = TCPHandler(ftsAddr,-1,verbose)
			s,src = tcpSock.accept()
			s.settimeout(0.5)

			if (src[0] != ftsAddr):
				print("WRONG SOURCE")
				sys.exit(0)

			# Receive the file
			rcvd = tcp.recvFile(s)
			if rcvd < 1:
				print("Receive Failure")
				sys.exit(2)

			if verbose:
				print("File Received - Wrote "+str(rcvd)+" bytes")

	except KeyboardInterrupt:
		print("\nBye!")
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
	# Check permissions
	if os.geteuid() != 0:
		print("Error: Must be run as super user")
		usage()
		sys.exit(0)
	# grab options and pass to main
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
		print("Error: Not enough arguments")
		usage()
		sys.exit(1)

	if len(args) > 0:
		print("Error: Unexpected Arguments")
		usage()
		sys.exit(1)

	main(opts)
	sys.exit(0)
