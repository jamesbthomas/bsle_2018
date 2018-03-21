# Python3 Source File for the PacketCrafter class used to craft packets for the custom communication protocol and supporting functions
# Assume all input validated before being passed to PacketCrafter
from scapy.all import *
from struct import *

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

	def craftInit(self,message):
		# Craft Packet Type 0x01
		pkt = Init()
		pkt.encMessage = message
		return pkt

	def craftResponse(self,port,message):
		# Craft Packet Type 0x02 and 0x03
		pkt = Response()
		pkt.tcpPort = port
		pkt.validation = message
		return pkt

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
		# Takes the encrypted validation message and crafts the response packet
		pkt = Response()
		if port < 1 or port > 65535:
			print("Error: Invalid Port Number")
			return None
		pkt.tcpPort = port
		pkt.validation = message
		return pkt

class Request(Packet):
	# Packet Type 0x00 - C2 -> FTS
	name = "FTRequest"
	fields_desc = [ByteField("packetType",0),
			IntField("fssAddress",0),
			ShortField("fssPort",0),
			ShortField("patternLength",0),
			StrLenField("pattern",""),
			StrLenField("message","") ]

class Response(Packet):
	# Packet Type 0x02 - FSS -> FTS
	name = "FTResponse"
	fields_desc = [ByteField("packetType",2),
			ShortField("tcpPort",0),
			StrLenField("validation","") ]

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
