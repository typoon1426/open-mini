# openmini Makefile

CC=gcc
CFLAGS=-I$(INCLUDE)
SRC=src
INCLUDE=include

all: $(SRC)/main.o $(SRC)/memory.o $(SRC)/fileIO.o $(SRC)/radio.o $(SRC)/error.o $(SRC)/serialDevice.o $(SRC)/cmdLineParser.o
	@echo "Linking all object files"
	$(CC) -o openMini $(SRC)/cmdLineParser.o  $(SRC)/memory.o $(SRC)/fileIO.o $(SRC)/radio.o $(SRC)/error.o $(SRC)/serialDevice.o $(SRC)/main.o

compile: $(SRC)/main.c $(SRC)/memory.c $(SRC)/fileIO.c $(SRC)/radio.c $(SRC)/error.c $(SRC)/serialDevice.c $(SRC)/cmdLineParser.c
	@echo "Compile all sources files"	
	$(CC) $(CFLAGS) -c $(SRC)/main.c $(SRC)/memory.c $(SRC)/fileIO.c $(SRC)/radio.c $(SRC)/error.c $(SRC)/serialDevice.c $(SRC)/cmdLineParser.c

clean:
	@echo "Remove all object and shared object files"
	rm -rf *.o $(SRC)/*.o openMini
