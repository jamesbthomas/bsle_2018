# Unit tests for C2 program using the unittest module
import sys, unittest, os
from struct import *
file_loc = os.path.dirname(os.path.realpath(__file__))
headers_dir = "/".join(file_loc.split("/")[:-2])+"/src/headers";
sources_dir = "/".join(file_loc.split("/")[:-2])+"/src/sources";
bin_dir = "/".join(file_loc.split("/")[:-2])+"/bin";
sys.path.append(headers_dir)
sys.path.append(sources_dir)
sys.path.append(bin_dir)
# Add the path to C2 to python's path so we can import it here, path addition is volatile
try:
	from C2 import socketValidate
	from packetCrafter import *
	from encoder import *
	from fileHandler import *
except ImportError as err:
	print(err)
	sys.exit(2)

class DefaultTestCase(unittest.TestCase):

	def setUp(self):
		# Suppress print calls from the functions
		## Sets stdout to the null device
		self.stdout = open(os.devnull,'w')
		sys.stdout = self.stdout
		self.pc = PacketCrafter()
		self.enc = Encoder("^2:40;rol1:120")
		self.handler = FileHandler()

	def tearDown(self):
		# Reset stdout
		self.stdout.close()
		sys.stdout = sys.__stdout__

class PatternValidateTestCase(DefaultTestCase):

	def runTest(self):
		print("Running patternValidate tests")
		# Valid Patterns
		## Provided in the prompt
		self.assertTrue(patternValidate("^2:40;rol1:120"))
		self.assertTrue(patternValidate("~:20;ror3:40"))
		self.assertTrue(patternValidate("ror8:80;^16:160"))
		## Should match patterns with more than two options
		self.assertTrue(patternValidate("^2:40;rol1:120;~:20"))
		self.assertTrue(patternValidate("ror3:40;ror8:80;^16:160"))
		# Invalid Patterns
		## Should fail on single, valid options
		self.assertFalse(patternValidate("^2:40"))
		self.assertFalse(patternValidate("rol1:120"))
		self.assertFalse(patternValidate("rol10:300"))
		## Should fail if first char in the option is not ^,~, or r
		self.assertFalse(patternValidate("a4:120;ror8:30"))
		self.assertFalse(patternValidate("rol6:5;14:340"))
		self.assertFalse(patternValidate("$7:5;rol6:30;~:94"))
		## Should fail if first char is r and is not followed by 'or' or 'ol'
		self.assertFalse(patternValidate("r1:5"))
		self.assertFalse(patternValidate("ro5:60"))
		self.assertFalse(patternValidate("rox5:100"))
		## Should fail if ~ not followed by a :
		self.assertFalse(patternValidate("~3:40"))
		self.assertFalse(patternValidate("rol9:3;~a:29"))
		self.assertFalse(patternValidate("~%:3;rol8:20"))
		## Should fail if has a valid operation but no number
		self.assertFalse(patternValidate("~:20"))
		self.assertFalse(patternValidate("^:30"))
		self.assertFalse(patternValidate("rol:80"))
		self.assertFalse(patternValidate("ror:40"))
		## Should fail if valid operation not followed by number of bytes
		self.assertFalse(patternValidate("^2:40;rol1"))
		self.assertFalse(patternValidate("~:20;ror3:a"))
		self.assertFalse(patternValidate("ror8:%;rol6:40"))
		self.assertFalse(patternValidate("rol8:~"))
		self.assertFalse(patternValidate("^5:80;rol36:^39"))

class FileValidateTestCase(DefaultTestCase):

	def runTest(self):
		# Known valid files
		## Relative Path
		self.assertEqual(self.handler.fileValidate(bin_dir+"/C2.py"),True)
		self.assertEqual(self.handler.fileValidate(headers_dir+"/../sources/README.md"),True)
		## Absolute Path (Assumes *nix system)
		self.assertEqual(self.handler.fileValidate("/usr/lib/os-release"),True)
		self.assertEqual(self.handler.fileValidate("/etc/passwd"),True)
		# Known invalid files
		## Relative Path
		self.assertEqual(self.handler.fileValidate("../../badfile"),False)
		self.assertEqual(self.handler.fileValidate("../anotherbad"),False)
		self.assertEqual(self.handler.fileValidate("badhere"),False)
		## Absoluate Path
		self.assertEqual(self.handler.fileValidate("/etc/badfile"),False)
		self.assertEqual(self.handler.fileValidate("/home/baddir"),False)
		# Malformed Path
		self.assertEqual(self.handler.fileValidate(".../."),False)
		self.assertEqual(self.handler.fileValidate("$/.."),False)

class socketValidateTestCase(DefaultTestCase):

	def runTest(self):
		# Valid Socket
		self.assertTrue(socketValidate("127.0.0.1:1"))
		self.assertTrue(socketValidate("8.8.8.8:4000"))
		self.assertTrue(socketValidate("10.73.195.4:65535"))
		# Invalid IP
		self.assertFalse(socketValidate("256.256.256.256:1"))
		self.assertFalse(socketValidate("a.b.c.d:1"))
		self.assertFalse(socketValidate("0.0.0.-1:1"))
		self.assertFalse(socketValidate("0.0.0.0:1"))
		self.assertFalse(socketValidate("255.255.255.255:1"))
		# Invalid Port
		self.assertFalse(socketValidate("127.0.0.1:0"))
		self.assertFalse(socketValidate("127.0.0.1:65536"))
		# Invalid Format
		self.assertFalse(socketValidate("127001:1"))
		self.assertFalse(socketValidate("127.0.0.14000"))

class ConvertAddrTestCase(DefaultTestCase):

	def runTest(self):
		self.assertEqual(self.pc.convertAddr("127.0.0.1"),2130706433)
		self.assertEqual(self.pc.convertAddr("0.0.0.0"),0)
		self.assertEqual(self.pc.convertAddr("255.255.255.255"),4294967295)

class EncoderTestCase(DefaultTestCase):

	def runTest(self):
		first = Encoder("~:20;ror3:40")
		self.assertEqual(first.pattern,"~:20;ror3:40")
		self.assertEqual(first.ops,["~","r3"])
		self.assertEqual(first.lens,["20","40"])
		self.assertRaises(ValueError,Encoder,"~2:20;ror3:40")
		self.assertRaises(ValueError,Encoder,"~:20;ror340")
		self.assertRaises(ValueError,Encoder,"~:20ror3:40")

class EncodeTestCase(DefaultTestCase):

	def runTest(self):
		encs = (Encoder("~:2;^1:2"),Encoder("ror1:2;rol1:2"))
		# Only first option runs
		self.assertEqual(encs[0].encode("m"),bytes([146]))
		self.assertEqual(encs[1].encode("m"),bytes([182]))
		# Both options run
		self.assertEqual(encs[0].encode("mess"),b'\x92\x9a\x72\x72')
		self.assertEqual(encs[1].encode("mess"),b'\xb6\xb2\xe6\xe6')
		# First runs twice, second runs once
		self.assertEqual(encs[0].encode("messa"),b'\x92\x9a\x72\x72\x9e')
		self.assertEqual(encs[1].encode("messa"),b'\xb6\xb2\xe6\xe6\xb0')
		# Both run twice or more
		self.assertEqual(encs[0].encode("messages"),b'\x92\x9a\x72\x72\x9e\x98\x64\x72')
		self.assertEqual(encs[1].encode("messages"),b'\xb6\xb2\xe6\xe6\xb0\xb3\xca\xe6')
		# TODO check exception cases

class DecodeTestCase(DefaultTestCase):

	def runTest(self):
		encs = (Encoder("~:2;^1:2"),Encoder("ror1:2;rol1:2"))
		# Only first option
		m0 = encs[0].encode("m")
		m1 = encs[1].encode("m")
		self.assertEqual(encs[0].decode(m0),"m")
		self.assertEqual(encs[1].decode(m1),"m")
		# Both options
		mess0 = encs[0].encode("mess")
		mess1 = encs[1].encode("mess")
		self.assertEqual(encs[0].decode(mess0),"mess")
		self.assertEqual(encs[1].decode(mess1),"mess")
		# First runs twice, second runs once
		messag0 = encs[0].encode("messag")
		messag1 = encs[1].encode("messag")
		self.assertEqual(encs[0].decode(messag0),"messag")
		self.assertEqual(encs[1].decode(messag1),"messag")
		# Reality test
		first = encs[0].encode("first message")
		second = encs[1].encode("second message")
		self.assertEqual(encs[0].decode(first),"first message")
		self.assertEqual(encs[1].decode(second),"second message")
		# TODO check exception cases

class CraftRequestTestCase(DefaultTestCase):

	def runTest(self):
		firstTest = self.pc.craftRequest("127.0.0.1","1111","~:40;rol1:120","firstTest")
		self.assertEqual(firstTest.name,"FTRequest")
		self.assertEqual(firstTest.packetType,0)
		self.assertEqual(firstTest.fssAddress,self.pc.convertAddr("127.0.0.1"))
		self.assertEqual(firstTest.fssPort,1111)
		self.assertEqual(firstTest.patternLength,len("~:40;rol1:120"))
		self.assertEqual(firstTest.pattern,b"~:40;rol1:120")
		self.assertEqual(firstTest.message,b"firstTest")
		secondTest = self.pc.craftRequest("127001","1111","~:40;rol1:120","secondTest")
		self.assertEqual(secondTest,None)
		thirdTest = self.pc.craftRequest("127.0.0.1","0","~:40;rol1:120","thirdTest")
		self.assertEqual(thirdTest,None)
		fourthTest = self.pc.craftRequest("127.0.0.1","1111","~2:40;rol1:120","fourthTest")
		self.assertEqual(fourthTest,None)
		fifthTest = self.pc.craftRequest("127.0.0.1","1111","~:40rol1:120","fifthTest")
		self.assertEqual(fifthTest,None)
		sixthTest = self.pc.craftRequest("127.0.0.1","1111","~:40;rol1120","sixthTest")
		self.assertEqual(sixthTest,None)

class CraftResponseTestCase(DefaultTestCase):

	def runTest(self):
		first = self.pc.craftResponse(1337,"validation message")
		self.assertEqual(first.tcpPort,1337)
		self.assertEqual(first.validation,b"validation message")
		second = self.pc.craftResponse(0,b"low port")
		self.assertEqual(second,None)
		third = self.pc.craftResponse(65536,b"high port")
		self.assertEqual(third,None)

class ContainsTestCase(DefaultTestCase):

	def runTest(self):
		self.assertEqual(self.handler.contains("/etc/hosts","127.0.0.1"),"127.0.0.1	localhost\n")
		self.assertEqual(self.handler.contains("/etc/hosts","::1"),"::1     ip6-localhost ip6-loopback\n")
		self.assertEqual(self.handler.contains(headers_dir+"/packetCrafter.py","this is not there"),None)
		self.assertEqual(self.handler.contains(headers_dir+"/packetCrafter.py","# Python3 Source File"),"# Python3 Source File for the PacketCrafter class used to craft packets for the custom communication protocol and supporting functions\n")
		self.assertEqual(self.handler.contains("/etc/hosts",1),"127.0.0.1	localhost\n")

if __name__ == "__main__":
	unittest.main()
