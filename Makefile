CC=gcc
CFLAGS=-c -g

all: abt gbn sr
	@- echo Make successful.

abt: abt.o
	$(CC) abt.o -o abt

gbn: gbn.o
	$(CC) gbn.o -o gbn
	
sr: sr.o
	$(CC) sr.o -o sr
	
abt.o: abt.c
	$(CC) $(CFLAGS) abt.c

gbn.o: gbn.c
	$(CC) $(CFLAGS) gbn.c
	
sr.o: sr.c
	$(CC) $(CFLAGS) sr.c

clean:
	rm -rf *o abt gbn sr
	@- echo Data Cleansing Done.Ready to Compile
