# Python3 Source File for the fileHandler class used to interact with files on the C2 and FSS

class FileHandler:

	# Validates that a provided file exists
	## Input - Filepath
	## Output - file object if it exists, None otherwise
	def fileValidate(self,path):
		try:
			f = open(path,"rb")
			return f
		except FileNotFoundError:
			print("File not found")
			return None

	# Reads from the provided file path and returns the line containing the provided string or none if it does not exist
	## Used by the C2 to check the IPAddWhiteList file
	def contains(self,path,string):
		with open(path) as file:
			for line in file:
				if line.find(str(string)) > -1:
					return line
		return None

