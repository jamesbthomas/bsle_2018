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
		pkt.pattern = self.convertPattern(pattern)
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
				print(num,val,2**(i+(numOct*8)))
			numOct += 1
		return decimal

	def convertPattern(self,pattern):
		# Convert encoding pattern from a string into a 4byte value
		# Split into options, each option gets 2 bytes
		opts = pattern.split(";")
		print(opts)
		if len(opts) != 2:
			print("Error: Invalid Encoding Pattern")
			return None
		packed = b''
		# Split into operation/length, each gets 1 byte
		for opt in opts:
			opLen = opt.split(":")
			if len(opLen) != 2:
				print("Error: Invalid Option Format")
				return None
			if opLen[0][0] == "~":	# b 01
				if (len(opLen[0]) != 1):
					print("Error: Invalid Operation")
					return None
				packed += pack("BB",64,int(opLen[1]))
			elif opLen[0][0:3] == "ror":	# b 10
				packed += pack("BB",(2*64)+int(opLen[0][3:]),int(opLen[1]))
			elif opLen[0][0:3] == "rol":	# b 11
				packed += pack("BB",(3*64)+int(opLen[0][3:]),int(opLen[1]))
			elif opLen[0][0] == "^":	# b 00
				packed += pack("BB",int(opLen[0][1:]),int(opLen[1]))
		return packed

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
	fields_desc = [ByteField("packetType",4),
			ShortField("tcpPort",0),
			StrLenField("validation","") ]

# Used for dev testing
if __name__ == "__main__":
	pc = PacketCrafter()
	out = pc.convertPattern("~:40rol1:120")
	print(unpack("BBBB",out))
