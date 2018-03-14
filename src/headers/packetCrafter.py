# Python3 Source File for the PacketCrafter class used to craft packets for the custom communication protocol
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
	r = pc.craftRequest(127,100,"xyz","gh")
	r.show()
	hexdump(r)
