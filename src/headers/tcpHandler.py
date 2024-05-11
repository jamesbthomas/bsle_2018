# Python3 Source File for the TCPHandler Class designed to handle sending and receiving TCP packets and associated functions to assist with handling packets
import re, random, os, time, socket
from encoder import *
from udpHandler import *

class TCPHandler():

	def __init__(self,dst,dport,pattern,verbose):
		# Takes a destination address, port, and encoding pattern to use for all traffic
		# Also takes a verbose argument to mark the session as verbose
		self.verbose = verbose
		if not addrValidate(dst):
			raise ValueError('Invalid IP Address')
		self.dst = dst
		if (dport < 1 or dport > 65535) and dport != -1:	# catch -1 so that this doesnt raise an exception when called by the FSS
			raise ValueError('Invalid port number')
		self.dport = dport
		self.sport = None
		self.seq = random.randint(0,2147483647)
		self.lastSeq = 0
		self.nextSeq = 0
		self.socket = None
		try:
			self.enc = Encoder(pattern)
		except ValueError as err:
			raise ValueError(err)

	def handshake(self):
		# Conducts the TCP three-way handshake with the destination and port provided at init
		## Returns True if success, None otherwise
		## Used by the C2 to initiate data transfer to the FTS
		self.socket = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
		self.socket.settimeout(5)
		self.socket.setsockopt(socket.IPPROTO_TCP,socket.TCP_NODELAY,1)
		self.socket.connect((self.dst,self.dport))
		return True;

	def sendFile(self,path,size):
		# Checks to see if connection is established and sends the amount of data indicated by size from the file pointed to by path to the destination
		## Returns total number of bytes transmitted, -1 if failed
		if self.socket == None:
			print("Error: Connection not established")
			return -1
		sent = 0
		f = open(path,"rb")
		while sent < size:
			sent += self.socket.send(f.read())
		if self.verbose:
			print("Send complete -",sent,"bytes transmitted")
		f.close()
		return sent

	def recvFile(self,sock,fname):
		# Prepares to receive the file and saves it to the designated path
		## Returns total number of bytes written to the file, -1 if failed
		bytesRcvd = 0
		f = open(fname,"w")
		total = 0
		pkt = sock.recv(1450)
		try:
			while(pkt != b''):
				# receive as long as packet contains stuff
				total += len(pkt)
				f.write(self.enc.decode(pkt))
				pkt = sock.recv(1450)
		except socket.timeout:
			pass
		except Exception as err:
			print(err)
			return -1
		f.close()
		return total

# Function to validate that a provided IP address is valid
## Returns true if valid, false if not
## Used by socketValidate in main
def addrValidate(addr):
	# Make sure its not a broadcast address
	if addr == "0.0.0.0" or addr == "255.255.255.255":
		print("Error: Invalid IP Address - will not send to all/broadcast")
		return False
	try:
		octets = addr.split(".")
		# Verify there are four octets
		if len(octets) != 4:
			print("Error: Invalid Format")
			return False
		# Make sure the octets are valid
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

if __name__ == "__main__":
	h = TCPHandler()
