.PHONY: all

all: server client

server.cpp:
	g++ server.cpp -o server

client.c:
	gcc client.c -o client
