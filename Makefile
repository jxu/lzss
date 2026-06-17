CC = gcc
CFLAGS = -O2 -Wall -Wextra -g

all: compress decompress tests

lzss.o: lzss.c lzss.h 
	$(CC) $(CFLAGS) -c $< -o $@

compress: lzss.o compress.c 
	$(CC) $(CFLAGS) $^ -o $@

decompress: lzss.o decompress.c 
	$(CC) $(CFLAGS) $^ -o $@

tests: lzss.o tests.c 
	$(CC) $(CFLAGS) $^ -o $@