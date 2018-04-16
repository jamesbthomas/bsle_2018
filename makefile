RMSRC=-rm -f src/sources/*.o
RMHDR=-rm -f src/headers/*.o
RMTST=-rm -f src/testharness/*.o
TSTDIR=src/testharness/
SRCDIR=src/sources/
HDRDIR=src/headers/
BINDIR=bin/
MODS=$(SRCDIR)encoder.o $(SRCDIR)udpHandler.o $(SRCDIR)tcpHandler.o
MAIN=$(SRCDIR)fts.o
WARN=-Werror -Wall

build: objects
# Builds without debug information and removes object files
	-gcc -o $(BINDIR)FTS $(MAIN) $(MODS) $(WARN) -lpthread -lm
# Clean 
	$(RMSRC)
	$(RMHDR)
	$(RMTST)

cleanAll: clean
# Removes all object files and binaries
	-rm -f bin/FTS

clean:
# Removes all object files
	$(RMSRC)
	$(RMHDR)
	$(RMTST)

objects:
	-gcc -c $(SRCDIR)encoder.c -o $(SRCDIR)encoder.o $(WARN)
	-gcc -c $(SRCDIR)udpHandler.c -o $(SRCDIR)udpHandler.o $(WARN)
	-gcc -c $(SRCDIR)tcpHandler.c -o $(SRCDIR)tcpHandler.o $(WARN)
	-gcc -c $(TSTDIR)test.c -o $(TSTDIR)test.o $(WARN)
	-gcc -c $(SRCDIR)fts.c -o $(SRCDIR)fts.o $(WARN)

debug:
# Builds with debugging symbols
	@echo "NOTICE - debugging optimized for GDB"
	-gcc -static -g -c $(SRCDIR)encoder.c -o $(SRCDIR)encoder.o $(WARN)
	-gcc -static -g -c $(SRCDIR)udpHandler.c -o $(SRCDIR)udpHandler.o $(WARN)
	-gcc -static -g -c $(SRCDIR)tcpHandler.c -o $(SRCDIR)tcpHandler.o $(WARN)
	-gcc -static -g -c $(TSTDIR)test.c -o $(TSTDIR)test.o $(WARN)
	-gcc -static -g -c $(SRCDIR)fts.c -o $(SRCDIR)fts.o $(WARN)

buildAll: debug
# Builds with debug information and leaves object files
	-gcc -o $(BINDIR)FTS $(MAIN) $(MODS) $(WARN) -lpthread -lm

install:
# Installs applicable external libraries and ensures python3 is available
	sudo apt install python3
	sudo apt install python3-pip
	sudo pip3 install scapy-python3
	sudo apt-get install libcunit1 libcunit1-doc libcunit1-dev

test: debug
	gcc -o $(TSTDIR)test $(MODS) $(TSTDIR)test.o $(WARN) -lcunit
	@echo "#####    PYTHON TESTS    #####"
	-python3 $(TSTDIR)test.py
	@echo "#####       C   TESTS    #####"
	-valgrind $(TSTDIR)test
	$(RMSRC)
	$(RMHDR)
	$(RMTST)

.PHONY: clean
