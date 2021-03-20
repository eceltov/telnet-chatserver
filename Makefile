CC=g++
CFLAGS=-Wall
MKDIR_P = mkdir -p
SDIR=src
ODIR=obj
BDIR=bin

_OBJ = \
	socket.o \
	chatserver.o

OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

_BIN = \
	ChatServer

BIN = $(patsubst %,$(BDIR)/%,$(_BIN))

all: $(ODIR) $(BDIR) $(OBJ) $(BIN)  

.PHONY: all clean

$(ODIR):
	$(MKDIR_P) $(ODIR)

$(BDIR):
	$(MKDIR_P) $(BDIR)

$(ODIR)/%.o: $(SDIR)/%.cpp
	$(CC) -c -o $@ $< $(CFLAGS)

$(BDIR)/ChatServer: $(OBJ) $(SDIR)/main.cpp
	$(CC) -o $@ $(SDIR)/main.cpp $(OBJ) $(CFLAGS)

clean: 
	rm obj/* | true
	rm bin/* | true