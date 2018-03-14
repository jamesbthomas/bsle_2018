# Python3 Source File for the PacketCrafter class used to craft packets for the custom communication protocol and supporting functions
from scapy.all import *

class PacketCrafter:
	'Packet Crafter for custom communication protocol'

	# UDP Packet Section
	def craftRequest(self,addr,port,pattern,phrase):
		# Craft Packet Type 0x00
		pkt = Request()
		pkt.fssAddress = addr # TODO convert string to decimal
		pkt.fssPort = port
		pkt.patternLength = len(phrase)
		pkt.pattern = pattern # TODO convert encoding pattern to 4 byte number
		pkt.message = phrase
		return pkt

	def craftInit(self,message):
		# Craft Packet Type 0x01
		pkt = Init()
		pkt.encMessage = message

	def craftResponse(self,port,message):
		# Craft Packet Type 0x02 and 0x03
		pkt = Response()
		pkt.tcpPort = port
		pkt.validation = message

	def convertAddr(self,addr):
		# Convert an IP address from a string to a number that can be put into the packet
		decimal = 0
		numOct = 0
		# Split the address into octets and reverse it in place (minimizes memory)
		octets = addr.split(".")
		octets.reverse()
		for octet in octets:
			num = int(octet)
			for i in (7,6,5,4,3,2,1,0):
				val = 2**i
				if val <= num:
					num -= val
					decimal += 2**(i+(numOct*8))
				print(num,val,2**(i+(numOct*8)))
			numOct += 1
		return decimal

class Request(Packet):
	name = "FTRequest"
	fields_desc = [ByteField("packetType",0),
			IntField("fssAddress",0),
			ShortField("fssPort",0),
			ShortField("patternLength",0),
			StrFixedLenField("pattern","",length_from=lambda x:4),
			StrLenField("message","") ]

class Init(Packet):
	name = "FTInit"
	fields_desc = [ByteField("packetType",1),
			StrLenField("encMessage","") ]

class Response(Packet):
	name = "FTResponse"
	fields_desc = [ByteField("packetType",2),
			ShortField("tcpPort",0),
			StrLenField("validation","") ]

# Used for dev testing
if __name__ == "__main__":
	pc = PacketCrafter()
	print(pc.convertAddr("127.0.0.1"))
