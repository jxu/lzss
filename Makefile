CC = gcc
CFLAGS = -O3 -g -Wall -Wextra

release: executable

# because not put in a separate dir, may require make -B to force
debug: CFLAGS += -DDEBUG
debug: executable

executable: compress decompress tests

lzss.o: lzss.c lzss.h 
	$(CC) $(CFLAGS) -c $< -o $@

compress: lzss.o compress.c 
	$(CC) $(CFLAGS) $^ -o $@

decompress: lzss.o decompress.c 
	$(CC) $(CFLAGS) $^ -o $@

tests: lzss.o tests.c 
	$(CC) $(CFLAGS) $^ -o $@