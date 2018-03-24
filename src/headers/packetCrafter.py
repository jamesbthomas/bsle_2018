# Python3 Source File for the PacketCrafter class used to craft packets for the custom communication protocol and supporting functions
# Assume all input validated before being passed to PacketCrafter
from scapy.all import *
from struct import *
import socket

class PacketCrafter:
	'Packet Crafter for custom communication protocol'

	# UDP Packet Section
	def craftRequest(self,addr,port,pattern,phrase):
		# Craft Packet Type 0x00
		pkt = Request()
		pkt.packetType = 0
		pkt.fssAddress = self.convertAddr(addr)
		if int(port) > 65535 or int(port) < 1:
			print("Error: Invalid port number")
			pkt.fssPort = None
		else:
			pkt.fssPort = int(port)
		pkt.patternLength = len(pattern)
		if not patternValidate(pattern):
			return None
		pkt.pattern = pattern
		pkt.message = phrase
		if pkt.fssAddress == None or pkt.fssPort == None or pkt.pattern == None:
			return None
		else:
			return pkt

	def unpackInit(self,pkt):
		# Unpack Packet Type 0x01
		# Verifies packet type and returns the encoded initialization message, or None if error
		# TODO find a way to automate testing this, see unpackResponse
		if (int(pkt.load[0]) != 49): # dec 49 = ascii 1
			print("Error: Invalid Packet Type")
			return None
		return pkt.load[1:],pkt[IP].src,pkt[UDP].sport

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
		# Takes the encrypted validation message and port to initiate the TCP transfer on
		pkt = Response()
		if port < 1 or port > 65535:
			print("Error: Invalid Port Number")
			return None
		pkt.tcpPort = port
		pkt.validation = message
		return pkt

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

class Request(Packet):
	# Packet Type 0x00 - C2 -> FTS
	name = "FTRequest"
	fields_desc = [ByteField("packetType",0),
			IntField("fssAddress",0),
			ShortField("fssPort",0),
			ShortField("patternLength",0),
			StrLenField("pattern",""),
			StrLenField("message","") ]

	def bytes(self):
		# Test function used to extract the byte string representing this layer
		return True

class Response(Packet):
	# Packet Type 0x02 - FSS -> FTS
	name = "FTResponse"
	fields_desc = [ByteField("packetType",2),
			ShortField("tcpPort",0),
			StrLenField("validation","") ]

	def payload(self):
		# Test function used to extract the byte string representing this layer
		return self.packetType

# Function to create a UDP socket on the provided port
## Return Value - the socket object on success, None otherwise
def makeUDP(port,timeout):
	try:
		sock = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
		sock.bind(('0.0.0.0',port))
		if timeout:
			sock.settimeout(0.003)
		return sock
	except socket.error as err:
		print("Error: Failed to create UDP socket - "+err)
		return None

# Function to validate that a provided pattern is valid
## Return Value 0 - pattern is not valid
## Return Value 1 - pattern is valid
def patternValidate(pattern):
	opts = pattern.split(";")
	if (len(opts) < 2):
		print("Error: Insufficient Encoding Options")
		return 0
	for set in opts:
		if not re.match('(~|\^\d+|ror\d+|rol\d+):\d+',set):
			print("Error: Invalid Encoding Pattern")
			return 0
	return 1

# Used for dev testing
if __name__ == "__main__":
	pc = PacketCrafter()
	print(pc.craftRequest("127.0.0.1",1111,"~:2~:2","pass").pattern)
