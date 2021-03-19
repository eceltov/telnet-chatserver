CC=g++
CFLAGS=-Wall
SDIR=src
ODIR=obj
BDIR=bin

_OBJ = \
	socket.o \
	chatserver.o

OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

_BIN = \
	main

BIN = $(patsubst %,$(BDIR)/%,$(_BIN))

all: $(OBJ) $(BIN) 

.PHONY: all clean

$(ODIR)/%.o: $(SDIR)/%.cpp
	$(CC) -c -o $@ $< $(CFLAGS)

$(BDIR)/main: $(OBJ) $(SDIR)/main.cpp
	$(CC) -o $@ $(SDIR)/main.cpp $(OBJ) $(CFLAGS)

clean: 
	rm obj/* | true
	rm bin/* | true