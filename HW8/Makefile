.PHONY: all

all: main

main: utility
	gcc -c main.c -Wall -o main.o
	gcc main.o utility.o -o main

utility:
	gcc -c utility.c -Wall -o utility.o

