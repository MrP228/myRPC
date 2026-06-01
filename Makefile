CC = gcc
CFLAGS = -Wall -Wextra -O2

.PHONY: all clean deb client server

all: client server

client:
	$(CC) $(CFLAGS) -o myRPC-client myRPC-client.c

server:
	$(CC) $(CFLAGS) -o myRPC-server myRPC-server.c

clean:
	rm -f myRPC-client myRPC-server
	rm -rf deb_build/

deb: all
	chmod +x build_deb.sh
	./build_deb.sh
