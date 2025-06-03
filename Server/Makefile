CC = gcc
CFLAGS = -Wall -pthread

all: server

server: server.c ../common/network.c
	$(CC) $(CFLAGS) server.c ../common/network.c -o server

clean:
	rm -f server

