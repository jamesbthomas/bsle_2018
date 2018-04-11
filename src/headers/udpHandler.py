# Python3 Source File for the UDPHandler class used to craft packets for the custom communication protocol and supporting functions
# Assume all input validated before being passed to PacketCrafter
from scapy.all import *
from struct import *
import socket
from encoder import *

class UDPHandler:

	# UDP Packet Section
	def craftRequest(self,addr,port,pattern,phrase,name):
		# Craft Packet Type 0x00
		# Field 1 - Address of the FSS
		addrCheck = self.convertAddr(addr)
		if addrCheck == None:
			return None
		addrBytes = addrCheck.to_bytes(4,byteorder='big')
		# Field 2 - Port on the FSS
		if int(port) < 1 or int(port)>65535:
			return None
		portBytes = int(port).to_bytes(2,byteorder='big')
		# Field 3 - Length of Encoding Pattern
		if not patternValidate(pattern):
			return None
		pattLenBytes = len(pattern).to_bytes(2,byteorder='big')
		# Field 4 - Encoding Pattern
		patternBytes = bytes(pattern,'us-ascii')
		# EXTRA Field 5 - Length of validation message
		phraseLenBytes = len(phrase).to_bytes(2,byteorder='big')
		# Field 6 - Validation Message
		phraseBytes = bytes(phrase,'us-ascii')
		# EXTRA Field 7 - Destionation File Name
		nameBytes = bytes(name,'us-ascii')
		pkt = b'\x00'+addrBytes+portBytes+pattLenBytes+patternBytes+phraseLenBytes+phraseBytes+nameBytes
		return pkt

	def unpackInit(self,pkt):
		# Unpack Packet Type 0x01
		# Verifies packet type and returns the encoded initialization message, or None if error
		# TODO find a way to automate testing this, see unpackResponse
		if (int(pkt.load[0]) != 49): # dec 49 = ascii 1
			print("Error: Invalid Packet Type")
			return None
		return pkt.load[1:],pkt[IP].src,pkt[UDP].sport
		# TODO see if this is necessary from the FSS side

	def convertAddr(self,addr):
		# Convert an IP address from a string to a number that can be put into the packet
		decimal = 0
		numOct = 0
		# Split the address into octets and reverse it in place (minimizes memory)
		octets = addr.split(".")
		if (len(octets) != 4):
			print("ERROR: Unexpected IP Address format")
			return None
		octets.reverse()
		for octet in octets:
			num = int(octet)
			for i in (7,6,5,4,3,2,1,0):
				val = 2**i
				if val <= num:
					num -= val
					decimal += 2**(i+(numOct*8))
			numOct += 1
		return decimal

	def craftResponse(self,port,message):
		# Craft Packet type 0x02/0x03
		# TODO modify to match new algorithm
		return None

	def unpackValidation(self,pkt,message):
		# Unpack Packet Type 0x03
		# Takes the packet and initialization message and returns the TCP Port for the transfer or None if the validation message does not match
		# TODO find a way to convert unsent packet into raw bytes so we can unit test this
		if int(pkt[0]) != 0x03:
			print("Error: Invalid Packet Type")
			return None
		tcpPort = int.from_bytes(pkt[1:3],byteorder='big')
		if tcpPort < 1 or tcpPort > 65535:
			print("Error: Could not validate response - invalid TCP port number")
			return None
		validation = pkt[3:].decode()
		if validation != message:
			print("Error: Could not validate response - incorrect validation message")
			return None
		return tcpPort

# Function to create a UDP socket on the provided port
## Return Value - the socket object on success, None otherwise
def makeUDP(port,timeout):
	try:
		sock = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
		sock.bind(('0.0.0.0',port))
		if timeout:
			sock.settimeout(0.1)
		return sock
	except socket.error as err:
		print("Error: Failed to create UDP socket - "+err)
		return None

# Function to send the request packet to the FTS and receive the validation packet
## Returns the validation packet (type 0x03) if the connection was successful, None otherwise
def requestTransfer(addr,port,pattern,phrase,ftsAddr,localport,verbose):
	try:
		sock = makeUDP(localport,True)
		udp = UDPHandler()
		pkt = udp.craftRequest(addr,port,pattern,phrase)
		for ftsPort in range(16000,17001):
			if ftsPort%100 == 0 and not verbose and ftsPort != 16000:
				print("Trying to connect . . . current port = ",ftsPort)
			elif verbose:
				print("Trying port ",ftsPort," . . . ")
			sock.sendto(pkt,(ftsAddr,ftsPort))
			try:
				response,(respAddr,respPort) = sock.recvfrom(1450)
			except socket.timeout:
				continue
			if ((respAddr != ftsAddr) or (respPort != ftsPort)):
				continue
			if response:
				print("Validation Received!")
				return response
			if verbose:
				print("Timed out...")
		return None
	except Exception as err:
		print(err+"\nBye!")
		sys.exit(2)

# Used for dev testing
if __name__ == "__main__":
	udp = UDPHandler()
	print(udp.craftRequest("127.0.0.1","1111","~:40;rol1:120","firstTest"))
