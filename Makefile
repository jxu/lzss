CC = gcc
CFLAGS = -O2 -Wall -Wextra -std=c99

all: compress decompress

lzss.o: lzss.c lzss.h 
	$(CC) $(CFLAGS) -c $< -o $@

compress: lzss.o compress.c 
	$(CC) $(CFLAGS) $^ -o $@

decompress: lzss.o decompress.c 
	$(CC) $(CFLAGS) $^ -o $@
