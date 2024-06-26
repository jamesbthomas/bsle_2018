# Unit tests for C2 and FSS programs using the unittest module
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
	from udpHandler import *
	from tcpHandler import *
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
		self.udp = UDPHandler()
		self.enc = Encoder("^2:40;rol1:120")
		self.handler = FileHandler()

	def tearDown(self):
		# Reset stdout
		self.stdout.close()
		sys.stdout = sys.__stdout__

##	ENCODER TESTS	##

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

class PatternValidateTestCase(DefaultTestCase):

	def runTest(self):
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

## 	FILEHANDLER TESTS	##

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

class ContainsTestCase(DefaultTestCase):

	def runTest(self):
		self.assertEqual(self.handler.contains("/etc/hosts","127.0.0.1"),"127.0.0.1	localhost\n")
		self.assertEqual(self.handler.contains("/etc/hosts","::1"),"::1     ip6-localhost ip6-loopback\n")
		self.assertEqual(self.handler.contains(headers_dir+"/udpHandler.py","this is not there"),None)
		self.assertEqual(self.handler.contains(headers_dir+"/udpHandler.py","# Python3 Source File"),"# Python3 Source File for the UDPHandler class used to craft packets for the custom communication protocol and supporting functions\n")
		self.assertEqual(self.handler.contains("/etc/hosts",1),"127.0.0.1	localhost\n")

class AddTestCase(DefaultTestCase):

	def runTest(self):
		self.assertEqual(self.handler.add("testfile.txt","test string"),len("test string")+1)
		f = open("testfile.txt","r")
		self.assertEqual(f.read(len("test string")),"test string")
		f.close()
		os.remove("testfile.txt")

##	UDPHANDLER TESTS	##

class CraftRequestTestCase(DefaultTestCase):

	def runTest(self):
		firstTest = self.udp.craftRequest("127.0.0.1","1111","~:40;rol1:120","firstTest","passwd")
		firstPkt = b'\x00'+self.udp.convertAddr("127.0.0.1").to_bytes(4,byteorder='big')+int("1111").to_bytes(2,byteorder='big')+len("~:40;rol1:120").to_bytes(2,byteorder='big')+bytes("~:40;rol1:120",'us-ascii')+len("firstTest").to_bytes(2,byteorder='big')+bytes("firstTest",'us-ascii')+bytes("passwd",'us-ascii')
		self.assertEqual(firstTest,firstPkt)
		secondTest = self.udp.craftRequest("127001","1111","~:40;rol1:120","secondTest","passwd")
		self.assertEqual(secondTest,None)
		thirdTest = self.udp.craftRequest("127.0.0.1","0","~:40;rol1:120","thirdTest","passwd")
		self.assertEqual(thirdTest,None)
		fourthTest = self.udp.craftRequest("127.0.0.1","1111","~2:40;rol1:120","fourthTest","passwd")
		self.assertEqual(fourthTest,None)
		fifthTest = self.udp.craftRequest("127.0.0.1","1111","~:40rol1:120","fifthTest","passwd")
		self.assertEqual(fifthTest,None)
		sixthTest = self.udp.craftRequest("127.0.0.1","1111","~:40;rol1120","sixthTest","passwd")
		self.assertEqual(sixthTest,None)

class UnpackInitTestCase(DefaultTestCase):

	def runTest(self):
		# right size
		goodpkt = b'\x01\x00\x04\x8f\x9e\x8c\x8c\x74\x65\x73\x74'
		mess,name = self.udp.unpackInit(goodpkt)
		self.assertEqual(mess,b'\x8f\x9e\x8c\x8c')
		self.assertEqual(name,b'test')
		# too short
		shortpkt = b'\x01\x00\x04'
		mess,name = self.udp.unpackInit(shortpkt)
		self.assertEqual(mess,None)
		self.assertEqual(name,None)
		# wrong packet type
		typepkt = b'\x00\x00\x04\x8f\x9e\x8c\x8c\x74\x65\x73\x74'
		mess,name = self.udp.unpackInit(typepkt)
		self.assertEqual(mess,None)
		self.assertEqual(name,None)

class ConvertAddrTestCase(DefaultTestCase):

	def runTest(self):
		self.assertEqual(self.udp.convertAddr("127.0.0.1"),2130706433)
		self.assertEqual(self.udp.convertAddr("0.0.0.0"),0)
		self.assertEqual(self.udp.convertAddr("255.255.255.255"),4294967295)

class CraftResponseTestCase(DefaultTestCase):

	def runTest(self):
		# good input
		first = self.udp.craftResponse(1337,"pass")
		firstpkt = b'\x02\x05\x39pass'
		self.assertEqual(first,firstpkt)
		# low port
		second = self.udp.craftResponse(0,"low port")
		self.assertEqual(second,None)
		# high port
		third = self.udp.craftResponse(65536,"high port")
		self.assertEqual(third,None)

class UnpackValidationTestCase(DefaultTestCase):

	def runTest(self):
		# good packet
		self.assertEqual(self.udp.unpackValidation(b'\x03\x05\x39\x67\x6f\x6f\x64\x20\x70\x61\x63\x6b\x65\x74',"good packet"),1337)
		# bad packet type
		self.assertEqual(self.udp.unpackValidation(b'\x00\x05\x39\x67\x6f\x6f\x64\x20\x70\x61\x63\x6b\x65\x74',"bad type"),None)
		# bad port
		self.assertEqual(self.udp.unpackValidation(b'\x03\x00\x00\x67\x6f\x6f\x64\x20\x70\x61\x63\x6b\x65\x74',"bad port"),None)
		# bad message
		self.assertEqual(self.udp.unpackValidation(b'\x03\x05\x39\x68\x6f\x6f\x64\x20\x70\x61\x63\x6b\x65\x74',"good packet"),None)
		# empty packet
		self.assertEqual(self.udp.unpackValidation(b'',"empty packet"),None)
		self.assertEqual(self.udp.unpackValidation(None,"empty packet"),None)

class MakeUDPTestCase(DefaultTestCase):

	def runTest(self):
		sock = makeUDP(1337,True)
		self.assertEqual(sock.getsockname()[1],1337)
		sock.close()
		sock = makeUDP(1338,False)
		self.assertEqual(sock.getsockname()[1],1338)
		sock.close()
		sock = makeUDP(0,False)
		self.assertNotEqual(sock,None)
		sock.close()
		self.assertEqual(makeUDP(65536,False),None)

## 	TCPHANDLER TESTS	##

class TCPInitTestCase(DefaultTestCase):

	def runTest(self):
		# good handler
		t = TCPHandler("127.0.0.1",1337,"~:2;~:2",True)
		self.assertEqual(t.dst,"127.0.0.1")
		self.assertEqual(t.dport,1337)
		self.assertEqual(t.enc.ops,["~","~"])
		self.assertEqual(t.enc.lens,["2","2"])
		self.assertEqual(t.enc.opts,["~:2","~:2"])
		self.assertEqual(t.enc.pattern,"~:2;~:2")
		self.assertEqual(t.verbose,True)
		# bad address
		self.assertRaises(ValueError,TCPHandler,"bad address",1337,"~:2;~:2",True)
		# bad port
		self.assertRaises(ValueError,TCPHandler,"127.0.0.1",0,"~:2;~:2",True)
		self.assertRaises(ValueError,TCPHandler,"127.0.0.1",65536,"~:2;~:2",True)
		# bad pattern
		self.assertRaises(ValueError,TCPHandler,"127.0.0.1",1337,"bad pattern",True)

class AddrValidateTestCase(DefaultTestCase):

	def runTest(self):
		# good address
		self.assertTrue(addrValidate("127.0.0.1"))
		# broadcast address
		self.assertFalse(addrValidate("0.0.0.0"))
		self.assertFalse(addrValidate("255.255.255.255"))
		# except cases
		self.assertFalse(addrValidate("bad address"))
		self.assertFalse(addrValidate(""))

## 	C2 GENERIC TESTS	##

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

if __name__ == "__main__":
	unittest.main()
