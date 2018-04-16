# Python3 header file for the Encoder class used by the FSS, replicated in encoder.c/encoder.h for FTS
# Will encode/decode one message at a time and save both the plaintext and the encoded messages
import re

class Encoder:

	def __init__(self,pattern):
		# Takes an encoding pattern as input on initialization to use for encoding/decoding
		# Confirm pattern format
		self.ops = []
		self.lens = []
		self.opts = pattern.split(";")
		# Make sure there are at least two encoding options
		if (len(self.opts) != 2):
			print("Error: Invalid Pattern Format")
			raise ValueError("Invalid Pattern Format")
		# Iterate across each option
		for opt in self.opts:
			opLen = opt.split(":")
			# make sure its in the right general format
			if (len(opLen) != 2):
				print("Error: Invalid Option Format")
				raise ValueError("Invalid Pattern Format")
			# Condense the operator to just the operation and value (if any) and store in self.ops
			# ie, rol6 becomes l6, ror5 becomes r5,
			if (opLen[0][0] == "r"):
				self.ops.append(opLen[0][2:])
			elif (opLen[0][0] == "~" and len(opLen[0]) != 1):
				print("Error: Invalid Operation Format - ~ should not have anything following it")
				raise ValueError("Invalid Pattern Format")
			else:
				self.ops.append(opLen[0])
			# Capture length self.lens
			self.lens.append(opLen[1])
		self.pattern = pattern

	def encode(self,message):
		# Takes a plaintext message and encodes it using the pattern specified at initialization
		self.plain = message
		# Transpose to a byte string
		self.plainBytes = message.encode()
		self.enc = b''
		index = 0
		while(index < len(self.plainBytes)):
			try:
				for i in (0,1):
					numOps = 0
					while (numOps < int(self.lens[i])):
						if self.ops[i] == "~":
							# Conduct bitwise NOT, aka XOR with all 1s
							self.enc += bytes([255 ^ self.plainBytes[index]])
						elif self.ops[i][0] == "r":
							# Conduct bitwise Right Shift
							byte = '0'+bin(self.plainBytes[index])[2:] # trim the 0b off the front and add an eighth bit to round out the byte
							rollVal = (int(self.ops[i][1:])%8) * -1
							self.enc += bytes([int("".join(byte[rollVal:]+byte[:rollVal]),2)])
						elif self.ops[i][0] == "l":
							# Conduct bitwise Left Shift
							byte = '0'+bin(self.plainBytes[index])[2:]
							rollVal = int(self.ops[i][1:])%8
							self.enc += bytes([int("".join(byte[rollVal:]+byte[:rollVal]),2)])
						elif self.ops[i][0] == "^":
							# Conduct bitwise XOR
							byte = int('0'+bin(self.plainBytes[index])[2:],2)
							xorVal = int(self.ops[i][1:])%8
							self.enc += bytes([byte ^ xorVal])
						else:
							print("Error: Internal Error - Unknown Operator")
							return None
						index += 1
						numOps += 1
			except IndexError:
				break
		return self.enc

	def decode(self,encrypted):
		# Takes a byte string and decrypts it into plaintext
		self.enc = encrypted
		self.plainBytes = b''
		index = 0
		while (index < len(self.enc)):
			try:
				for i in (0,1):
					numOps = 0
					while (numOps < int(self.lens[i])):
						if self.ops[i] == "~":
							# Encrypt with NOT, decrypt with NOT
							self.plainBytes += bytes([255 ^ self.enc[index]])
						elif self.ops[i][0] == "r":
							# Encrypt with rotate right, decrypt with rotate left
							byte = bin(self.enc[index])[2:]
							if (len(byte) < 8):
								byte = '0'+byte
							elif (len(byte) > 8):
								print("Error: Internal Error - Binary Translation Error")
								return None
							rollVal = int(self.ops[i][1:])%8
							self.plainBytes += bytes([int("".join(byte[rollVal:]+byte[:rollVal]),2)])
						elif self.ops[i][0] == "l":
							# Encrypt with rotate left, decrypt with rotate right
							byte = bin(self.enc[index])[2:]
							if (len(byte) < 8):
								byte = '0'+byte
							elif (len(byte) > 8):
								print("Error: Internal Error - Binary Translation Error")
								return None
							rollVal = (int(self.ops[i][1:])%8) * -1
							self.plainBytes += bytes([int("".join(byte[rollVal:]+byte[:rollVal]),2)])
						elif self.ops[i][0] == "^":
							# Encrypt with XOR, decrypt with XOR
							byte = int('0'+bin(self.enc[index])[2:],2)
							xorVal = int(self.ops[i][1:])%8
							self.plainBytes += bytes([byte ^ xorVal])
						else:
							print("Error: Internal Error - Unknown Operator")
							return None
						index += 1
						numOps += 1
			except IndexError:
				break
		try:
			self.plain = self.plainBytes.decode()
			return self.plain
		except UnicodeDecodeError as err:
			# Catch bit flip errors just in case it comes out unreadable
			print(err)
			return None

# Function to validate that a provided pattern is valid
## Returns False = pattern is not valid
## Returns True = pattern is valid
def patternValidate(pattern):
	opts = pattern.split(";")
	# make sure there are at least two options
	if (len(opts) < 2):
		print("ERROR: Insufficient Encoding Options")
		return False
	# Validate each option using a regex
	for opVal in opts:
		if not re.match('(~|\^\d+|ror\d+|rol\d+):\d+',opVal):
			print("ERROR: Invalid Encoding Option")
			return False
	return True

# Used for dev testing and by the test.sh script
if __name__ == "__main__":
	enc = Encoder("~:2;~:2")
	with open("/etc/passwd","r") as file:
		print(enc.encode(file.read()))
