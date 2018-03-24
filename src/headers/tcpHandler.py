# Python3 Source File for the TCPHandler Class designed to handle sending and receiving TCP packets and associated functions to assist with handling packets
import re, random, os, time, socket
from scapy.all import *
from encoder import *
from packetCrafter import *
# TODO TEST!!!!!
class TCPHandler():

	def __init__(self,dst,dport,verbose):
		# Takes a destination address and port for all sent traffic
		# Also takes a verbose argument to mark the session as verbose
		self.verbose = verbose
		self.dst = dst
		self.dport = dport
		self.sport = None
		self.seq = random.randint(0,2147483647)
		self.lastSeq = 0
		self.nextSeq = 0
		self.socket = None

	def handshake(self,filename):
		# Conducts the TCP three-way handshake with the destination and port provided at init
		## Returns True if success, None otherwise
		## Used by the C2 to initiate data transfer to the FTS
		self.socket = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
		self.socket.settimeout(5)
		self.socket.connect((self.dst,self.dport))
		return True;

	def shake(self,port):
		# Accepts TCP three-way handshake
		## Returns True if handshake successful, False otherwise
		## USed by the FSS to tell the FTS to initiat encoded transfer
		fails = 0
		self.state = "LISTEN"
		syn = None
		while (fails < 3):
			syn = sniff(filter="tcp dst port "+str(port)+" and ip src host "+self.dst,count=1)[0]
			if not syn:
				if syn[TCP].flags != 2:
					syn = None
				fails += 1
			else:
				self.state = "SYN_RCVD"
				self.dport = syn[TCP].sport

		if fails >= 3:
			print("Error: Bad SYN")
			return False
		fails = 0
		synAck = send(IP(dst=self.dst) / TCP(dport=self.dport,flags="SA",ack=syn[TCP].seq+1,seq=self.seq))
		while (fails < 3):
			ack = sniff(filter="tcp dst port "+str(port)+" and ip src host "+self.dst,count=1)[0]
			if not ack:
				if ack[TCP].flags != 16:
					ack = None
				fails += 1
			else:
				self.state = "ESTABLISHED"
				break
		if fails >= 3:
			print("Error: Bad ACK")
			return False
		self.sport = port
		return True,pkt.load

	def sendFile(self,path,size):
		# Checks to see if connection is established and sends the amount of data indicated by size from the file pointed to by path to the destination
		## Returns total number of bytes transmitted, -1 if failed
		if self.socket == None:
			print("Error: Connection not established")
			return -1
		sent = 0
		f = open(path,"rb")
		name = self.socket.send(bytes(path.split("/")[-1],'us-ascii'))
		while sent < size:
			sent += self.socket.send(f.read())
		if self.verbose:
			print("Send complete - "+sent+" bytes transmitted")
		f.close()
		return sent

	def recvFile(self,path):
		# Prepares to receive the file and saves it to the designated path
		## Returns total number of bytes written to the file, -1 if failed
		if self.sport == None:
			print("Error: Cannot receive files without shaking first")
			return -1
		bytesRcvd = 0
		t = time.localtime()
		f = open(path+"_"+str(t.tm_hour)+str(t.tm_min)+"_"+str(t.tm_mon)+str(t.tm_mday),"wb")
		# Sniff until we get a FIN packet
		try:
			while True:
				pkt = sniff(filter="tcp dst port "+str(PORT)+" and ip src host "+self.dst,count=1)[0]
				if pkt:
					if pkt[TCP].flags == 1:
						break
					else:
						bytesRecvd += len(pkt.load)
						f.write(pkt.load)
		except IndexError:
			print("Error: Failed to receive packets")
			f.close()
			return -1
		# Handle the Close
		send(IP(dst=self.dst) / TCP(dport=self.dport,sport=self.sport,flags="A"))
		self.state = "CLOSE_WAIT"
		fin = IP(dst=self.dst) / TCP(dport=self.dport,sport=self.sport,flags="F")
		ack = None
		while ack == None:
			self.state = "LAST_ACK"
			ack = sr1(fin,timeout=0.1,verbose=False)
			if ack != None:
				if ack[TCP].flags == 16:
					self.state == "CLOSED"
					f.close()
					if verbose:
						print("Transfer complete!")
					return bytesRcvd
				else:
					self.state = "CLOSE_WAIT"
					ack = None
		f.close()
		print("Error: Failed to close")
		return -1

# Function to send the request packet to the FTS
## Returns the response packet (type 0x03) if the connection was successful, None otherwise
def requestTransfer(addr,port,pattern,phrase,ftsAddr,localport,verbose):
	try:
		sock = makeUDP(localport,True)
		crafter = PacketCrafter()
		requestPacket = crafter.craftRequest(addr,port,pattern,phrase)
		for ftsPort in range(16000,16001): #range(16000,17001):
			if ftsPort%100 == 0 and not verbose and ftsPort != 16000:
				print("Trying to connect... 16000 - ",ftsPort,"unavailable")
			if verbose:
				print("Trying port ",ftsPort,". . . ")
#			request = IP(dst=ftsAddr) / UDP(sport=65534,dport=ftsPort) / requestPacket
			addrBytes = crafter.convertAddr(addr).to_bytes(4,byteorder='big')
			portBytes = int(port).to_bytes(2,byteorder='big')
			lenBytes = len(pattern).to_bytes(2,byteorder='big')
			patternBytes = bytes(pattern,'us-ascii')
			phraseBytes = bytes(phrase,'us-ascii')
			sock.sendto(b'\x00'+addrBytes+portBytes+lenBytes+patternBytes+phraseBytes,(ftsAddr,ftsPort))
			try:
				response,(respAddr,respPort) = sock.recvfrom(1450)
			except socket.timeout:
				time.sleep(1000)
			if ((respAddr != addr) or (respPort != ftsPort)):
				continue
			if response:
				print("Connection established!")
				return response, respPort
			if verbose:
				print("Timed out...")
		return None
	except Exception as err:
		print(err+"\nBye!")
		sys.exit(0);

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

if __name__ == "__main__":
	h = TCPHandler()
