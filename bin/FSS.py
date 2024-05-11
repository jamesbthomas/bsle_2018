# Python3 Source File for File Storage Service Main Function
## Each FSS instance supports exactly one encoding pattern specified by the user at runtime
import getopt, sys, socket, os
file_loc = os.path.dirname(os.path.realpath(__file__))
headers_dir = "/".join(file_loc.split("/")[:-1])+"/src/headers"
sys.path.append(headers_dir)
try:
	from udpHandler import *
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
	udp = UDPHandler()
	handler = FileHandler()
	verbose = False

	if len(opts) < 1:
		# request the encoding pattern
		while True:
			try:
				pattern = input("Encoding Pattern: ").strip()
				if patternValidate(pattern):
					break
			except KeyboardInterrupt:
				print("\nBye!")
				sys.exit(0)
		# request the local port
		while True:
			try:
				port = int(input("Local Port: ").strip())
				if port > 0 and port < 65536:
					break
				else:
					print("Error: Port must be between 1 and 65535")
			except KeyboardInterrupt:
				print("\nBye!")
			except TypeError:
				print("Error: Port must be a number")
		# request the destination directory
		while True:
			try:
				destination = input("Destination Directory [.]: ").strip()
				if destination == '':
					destination = "./"
					break
				if os.path.isdir(destination):
					if destination[-1] != '/':
						destination += '/'
					break
				else:
					print("Error: Destination must be a directory")
			except KeyboardInterrupt:
				print("\nBye!")
				sys.exit(0)

	else:
		for switch,val in opts:
			# Iterate through the switches and assign values
			if switch == "-e" or switch == "--encode":
				if patternValidate(val):
					pattern = val
			elif switch == "-d" or switch == "--destination":
				destination = val
				# Make sure provided destination is a directory and make sure its / terminated
				if not os.path.isdir(destination):
					print("Error: Destination must be a directory")
					sys.exit(1)
				elif destination[-1] != '/':
					destination += '/'
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

		# If destination directory wasnt provided, set the default
		if destination == None:
			destination = "./"

	# Catch any random errors
	if pattern == None or port == None:
		print("Error: Failed to assign values")
		sys.exit(2)

	# Create the encoder handler and UDP listener
	encoder = Encoder(pattern)
	sock = makeUDP(port,False);
	if (sock == None):
		# Catch socket crate errors
		sys.exit(2)

	try:
		# Loop to keep the server up and running
		while True:
			# Loop until we get a reqeust to save a new file
			while True:
				try:
					# Loop until we get a 0x01 packet
					while True:
						# Receive and unpack packets
						pkt, (ftsAddr,ftsPort)  = sock.recvfrom(1450)
						message,filename = udp.unpackInit(pkt)
						if message != None and filename != None:
							# Make sure unpackInit worked, ipsofacto that we received a 0x01 packet
							if verbose:
								print("Received message from {0}:{1} - {2} - {3}".format(ftsAddr,ftsPort,message,filename))
							break
				except KeyboardInterrupt:
					# Allow forceful, graceful exit
					sock.close();
					print("\nBye!")
					sys.exit(0)

				# Verify that the filename is available for this transfer
				if handler.fileValidate(destination+filename.decode("us-ascii")):
					# Send packet type 0x04
					if verbose:
						print("File name exists - sending type 0x04")
					sock.sendto(b'\x04',(ftsAddr,ftsPort))
					continue
				else:
					break

			# Start processing the message
			decoded = encoder.decode(message)
			if (decoded == None):
				# Make sure decode succeeded
				print("Error: Could not decode message")
				sys.exit(2)
			recoded = encoder.encode(decoded)
			if recoded == None:
				# catches an error with the encoding pattern that may not have been hit yet
				print("Error: Could not encode message")
				sys.exit(2)
			if verbose:
				print(message,"->",decoded,"->",recoded)

			if message != recoded:
				# Catch any errors in the decode/recode process
				print("Error: Decode/Recode Failure")
				sys.exit(2)

			# Prep the TCP Listener
			tcpSock = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
			tcpSock.bind(('0.0.0.0',0))
			tcpSock.settimeout(5)
			tcpPort = tcpSock.getsockname()[1]
			tcpSock.listen(1)

			# Send the 0x02 packet
			sock.sendto(b'\x02'+tcpPort.to_bytes(2,byteorder='big')+recoded,(ftsAddr,ftsPort))
			if verbose:
				print("Local Port - "+str(tcpPort))

			if verbose:
				print("Waiting for file...")

			# Build the handler for the TCP transfer and start accepting connections
			tcp = TCPHandler(ftsAddr,-1,pattern,verbose)
			while True:
				s,src = tcpSock.accept()
				s.settimeout(2)
				if (src[0] == ftsAddr):
					# Deny connections that arent from the FTS
					break

			# Receive the file
			rcvd = tcp.recvFile(s,destination+filename.decode("us-ascii"))
			if rcvd < 1:
				print("Receive Failure")
				tcpSock.close()
				s.close()
				sys.exit(2)

			if verbose:
				print("File Received - Wrote "+str(rcvd)+" bytes")
			tcpSock.close()
			s.close()

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
	print("\t -d <path> \t optional, same as --destination, specify the destination directory for the FSS to store files, defaults to the working directory at runtime")
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
		# Capture the inputs
		opts,args = getopt.getopt(sys.argv[1:],"e:p:hd:v",["help","encode=","port=","destination=","verbose"])
		# Check for the -h switch, works with the ValueError catch below
		opts.index(('-h',''))
		help()
		sys.exit(0)
	except getopt.GetoptError as err:
		# Catch switch errors
		print(err)
		usage()
		sys.exit(1)
	except ValueError:
		try:
			# Check for --help switch
			opts.index(('--help',''))
			help()
			sys.exit(0)
		except ValueError:
			pass

	if len(args) > 0:
		# Make sure there arent any extraneous inputs
		print("Error: Unexpected Arguments")
		usage()
		sys.exit(1)

	main(opts)
	sys.exit(0)

# Exit Codes
## 0 - Success
## 1 - Input Error
## 2 - Internal Error
## 3 - Could not complete initialization sequence (UDP)
## 4 - TCP Error
