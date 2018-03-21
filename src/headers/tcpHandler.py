# Python3 Source File for the TCPHandler Class designed to handle sending and receiving TCP packets and associated functions to assist with handling packets
import re, random, os
from packetCrafter import *
from scapy.all import *
# TODO TEST!!!!!
class TCPHandler():

	def __init__(self,dst,dport,verbose):
		# Takes a destination address and port for all sent traffic
		# Also takes a verbose argument to mark the session as verbose
		self.verbose = verbose
		self.dst = dst
		self.dport = dport
		self.seq = randint(0,2147483647)
		self.lastSeq = 0
		self.nextSeq = 0
		self.state = "init"

	def handshake(self,verbose):
		# Conducts the TCP three-way handshake with the destination and port provided at init
		## Returns True if handshake is successful, False otherwise
		syn = IP(dst=self.dst) / TCP(dport=self.dport,flags="S",seq=self.seq)
		synAck = None
		fails = 0 # keep track of how many times we've failed so this doesn't loop forever
		self.state = "SYN_SENT"
		while fails < 3:
			synAck = sr1(syn,timeout=0.1)
			if verbose:
				print("Sent SYN \#"+fails)
			if not synAck:
				if synAck[TCP].flags != 18:
					synAck = None
				fails += 1
				if verbose:
					print("Timeout")
			else:
				self.state = "ESTABLISHED"
				break
		if fails >= 3:
			print("Error: Could not establish TCP connection")
			self.state = "init"
			return False
		if verbose:
			print("SYNACK Received!")
		# Craft ACK and send
		self.lastSeq = synAck.seq
		ack = IP(dst=self.dst) / TCP(dport=self.dport,flags="A",ack=synAck.seq+1,seq=synAck.ack)
		send(ack)
		if verbose:
			print("ACK Sent - Connection Established")
		return True

	def close(self,verbose):
		# Close the connection
		## Returns True if close successful, False otherwise
		fin = IP(dst=self.dst)) / TCP(dport=self.dport,flags="F")
		fails = 0
		finAck = None
		self.state = "FIN_WAIT_1"
		while fails < 3:
			finAck = sr1(fin1,timeout=0.1)
			if verbose:
				print("Sent FIN \#"+fails)
			if not finAck:
				if finAck[TCP].flags != 16:
					finAck = None
				fails += 1
				if verbose:
					print("Timeout")
			else:
				self.state = "FIN_WAIT_2"
				break
		if fails >= 3:
			print("Error: Could not close connection")
			self.state = "ESTABLISHED"
			return False
		if verbose:
			print("ACK Received from FTS")
		# Wait for FIN from FTS
		finServer = None
		fails = 0
		if verbose:
			print("Waiting for FIN from FTS...")
		while fails < 3:
			finServer = sniff(count=1,filter="tcp[tcpflags] & tcp-fin != 0 and src host = "+self.dst)
			if verbose:
				fails += 1
			else:
				self.state = "TIME_WAIT"
				break
		if fails >= 3:
			print("Error: Could not close connection")
			self.state = "ESTABLISHED"
			return False
		if verbose:
			print("FIN Received from FTS")
		# Send final ACK
		ack = IP(dst=self.dst) / TCP(dport=self.dport,flags="A")
		send(ack)
		if verbose:
			print("ACK Sent to server - Handler reset to initial conditions")
		self.seq = randint(0,2147483647)
		self.lastSeq = 0
		self.nextSeq = 0
		self.state = "init"

	def sendFile(self,path,size,verbose):
		# Checks to see if connection is established and sends the amount of data indicated by size from the file pointed to by path to the destination
		## Returns total number of bytes transmitted, -1 if failed
		if self.state != "ESTABLISHED":
			print("Error: Connection not established")
			return -1
		try:
			f = open(path,"rb")
		except FileNotFoundError:
			print("Error: File not found")
			return -1
		sent = 0
		while sent < size:
			# Read 1400 bytes at a time, lower than max segment size to avoid issues but not so low that it will be slow
			data = f.read(size-sent)
			pkt = IP(dst=self.dst) / TCP(dport=self.dport,seq=self.lastSeq) / data
			self.nextSeq = self.lastSeq + len(data)
			ack = None
			while ack == None:
				if verbose:
					print("Sent",sent+len(data),"of",size,"bytes")
				ack = sr1(pkt,timeout=0.1)
				if ack[TCP].flags != 16 or ack[TCP].ack != self.nextSeq:
					ack = None
				else:
					if verbose:
						print("Received ACK for bytes",sent,"-",len(data))
					sent += len(data)
					self.lastSeq = self.nextSeq
		if verbose:
			print("Send complete - "+sent+" bytes transmitted")
		return sent

# Function to send the request packet to the FTS
## Returns the response packet (type 0x03) if the connection was successful, None otherwise
def requestTransfer(addr,port,pattern,phrase,ftsAddr):
	crafter = PacketCrafter()
	requestPacket = crafter.craftRequest(addr,port,pattern,phrase)
	for ftsPort in range(16000,17001):
		if ftsPort%100 == 0 and not self.verbose and ftsPort != 16000:
			print("Trying to connect... 16000 - ",ftsPort,"unavailable")
		if self.verbose:
			print("Trying port ",ftsPort,". . . ")
		request = IP(dst=ftsAddr) / UDP(dport=ftsPort) / requestPacket
		response = sr1(request,timeout=0.1,verbose=False)
		if response:
			print("Connection established!")
			return response
		if self.verbose:
			print("Timed out...")

	return None

# Function to validate that a provided IP address is valid
## Returns true if valid, false if not
## Used by socketValidate in main
def addrValidate(addr):
	if addr == "0.0.0.0" or addr == "255.255.255.255":
		print("Error: Invalid IP Address - will not send to all/broadcast")
		return False
	try:
		octets = addr.split(".")
		if len(octets) != 4:
			raise AttributeError("Invalid Format")
		for oct in octets:
			if int(oct) > 255 or int(oct) < 0:
				print("Error: Invalid IP Address")
				return False
	except AttributeError:
		print("Error: Invalid IP Address Format")
		return False
	except ValueError:
		print("Error: Invalid IP Address Format")
		return False
	return True
