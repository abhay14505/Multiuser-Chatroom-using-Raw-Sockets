CC = gcc
CFLAGS = `pkg-config --cflags gtk+-3.0` -Wall -pthread
LIBS = `pkg-config --libs gtk+-3.0`

all: client

client: client.c gui.c typing.c ../common/network.c
	$(CC) $(CFLAGS) client.c gui.c typing.c ../common/network.c -o client $(LIBS)

clean:
	rm -f client

