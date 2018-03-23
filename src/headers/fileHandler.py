# Python3 Source File for the fileHandler class used to interact with files on the C2

class FileHandler:

	# Validates that a provided file exists
	## Input - Filepath
	## Output - True if exists, False otherwise
	def fileValidate(self,path):
		try:
			f = open(path,"rb")
			f.close()
			return True
		except FileNotFoundError:
			print("File not found")
			return False

	# Reads from the provided file path and returns the line containing the provided string or none if it does not exist
	## Used by the C2 to check the IPAddWhiteList file
	def contains(self,path,string):
		with open(path) as file:
			for line in file:
				if line.find(str(string)) > -1:
					return line
		return None


	# Adds the provided string to the provided file as a new line
	## Used by the C2 to manage the IPAddWhiteList file
	## Returns the number of bytes written to the file
	def add(self,path,string):
		f = open(path,"w+")
		f.write(string+"\n")
		return len(string+"\n")
