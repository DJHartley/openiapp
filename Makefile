CC=gcc
IDIR=include
SDIR=src
CFLAGS= -g -Wall -I$(IDIR)
LIBS = -limobiledevice -lplist -lusbmuxd -lssl -lcrypto
TARGET = openiapp
OUTPUT = release/openiapp

all: $(TARGET)

$(TARGET): $(SDIR)/$(TARGET).c
		$(CC) $(CFLAGS) -o $(OUTPUT) $(SDIR)/$(TARGET).c $(LIBS)

clean:
	rm $(TARGET)
