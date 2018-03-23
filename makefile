RMSRC=rm -f src/sources/*.o
RMHDR=rm -f src/headers/*.o
RMTST=rm -f src/testharness/*.o
TSTDIR=src/testharness/
SRCDIR=src/sources/
HDRDIR=src/headers/

cleanall:
# Removes all object files and binaries
	rm bin/FTS
	make clean

clean:
# Removes all object files
	$(RMSRC)
	$(RMHDR)
	$(RMTST)

debug:
# Builds with debugging symbols
	make install

build:
# Builds without debug information and removes object files
	make install
	$(RM)

buildAll:
# Builds with debug information and leaves object files
	make install

install:
# Installs applicable external libraries and ensures python3 is available
	sudo apt install python3
	sudo apt install python3-pip
	sudo pip3 install scapy-python3
	sudo apt-get install libcunit1 libcunit1-doc libcunit1-dev

test:
	gcc -o $(TSTDIR)test $(TSTDIR)test.c $(SRCDIR)encoder.c -Wall -lcunit
	## PYTHON TESTS ##
	-python3 $(TSTDIR)test.py
	##    C   TESTS ##
	-$(TSTDIR)test

.PHONY: clean
