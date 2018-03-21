# Python3 header file for the Encoder class used by the FSS, replicated in encoder.c/encoder.h for FTS
# Will encode/decode one message at a time and save both the plaintext and the encoded messages
import re

class Encoder:

	def __init__(self,pattern):
		# Takes an encoding pattern as input on initialization to use for encoding/decoding
		# Confirm pattern format
		self.ops = []
		self.lens = []
		opts = pattern.split(";")
		if (len(opts) != 2):
			print("Error: Invalid Pattern Format")
			raise ValueError("Invalid Pattern Format")
		for opt in opts:
			opLen = opt.split(":")
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
							print("Error: Internal Error - Unknown Operator") # TODO raise exception
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
								print("Error: Internal Error - Binary Translation Error") # TODO raise exception
								return None
							rollVal = int(self.ops[i][1:])%8
							self.plainBytes += bytes([int("".join(byte[rollVal:]+byte[:rollVal]),2)])
						elif self.ops[i][0] == "l":
							# Encrypt with rotate left, decrypt with rotate right
							byte = bin(self.enc[index])[2:]
							if (len(byte) < 8):
								byte = '0'+byte
							elif (len(byte) > 8):
								print("Error: Internal Error - Binary Translation Error") # TODO raise exception
								return None
							rollVal = (int(self.ops[i][1:])%8) * -1
							self.plainBytes += bytes([int("".join(byte[rollVal:]+byte[:rollVal]),2)])
						elif self.ops[i][0] == "^":
							# Encrypt with XOR, decrypt with XOR
							byte = int('0'+bin(self.enc[index])[2:],2)
							xorVal = int(self.ops[i][1:])%8
							self.plainBytes += bytes([byte ^ xorVal])
						else:
							print("Error: Internal Error - Unknown Operator") # TODO raise exception
							return None
						index += 1
						numOps += 1
			except IndexError:
				break
		self.plain = self.plainBytes.decode()
		return self.plain

# Used for dev testing
if __name__ == "__main__":
	enc = Encoder("ror1:40;rol1:120")
	encoded = enc.encode("m")
	print("encoded - ",encoded)
	decoded = enc.decode(encoded)
	print("decoded - ",decoded)