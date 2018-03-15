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
							print(bytes([self.plainBytes[index] >> int(self.ops[i][1:])]))
							# TODO write right bit shift that takes bits off the end and puts them into the MSB
						elif self.ops[i][0] == "l":
							# Conduct bitwise Left Shift
							# TODO write left bit shift that takes bits off the end and puts them into the MSB
						elif self.ops[i][0] == "^":
							# Conduct bitwise XOR
							# TODO
						index += 1
						numOps += 1
			except IndexError:
				break
		print(self.enc)
		return None

	def shiftRight(self,binary):
		# TODO
		return None

	def shiftLeft(self,binary):
		# TODO
		return None

# Used for dev testing
if __name__ == "__main__":
	enc = Encoder("~:40;rol1:120")
	enc.encode("message")
