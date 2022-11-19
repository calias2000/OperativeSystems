# Makefile, versao 1
# Sistemas Operativos, DEI/IST/ULisboa 2019-20

CC      = gcc
LD      = gcc
CFLAGS  =-Wall -pthread -std=gnu99 -I../
MUTEX   = -DMUTEX
RWLOCK  = -DRWLOCK
LDFLAGS = -lm

# A phony target is one that is not really the name of a file
# https://www.gnu.org/software/make/manual/html_node/Phony-Targets.html
.PHONY: all clean run

all: tecnicofs-nosync tecnicofs-mutex tecnicofs-rwlock

tecnicofs-nosync: lib/bst.o lib/hash.o fs.o main.o
	$(LD) $(CFLAGS) $(LDFLAGS) -o tecnicofs-nosync lib/bst.o lib/hash.o fs.o main.o

tecnicofs-mutex: lib/bst.o lib/hash.o fsmutex.o mainmutex.o
	$(LD) $(CFLAGS) $(MUTEX) $(LDFLAGS) -o tecnicofs-mutex lib/bst.o lib/hash.o fsmutex.o mainmutex.o

tecnicofs-rwlock: lib/bst.o lib/hash.o fsrwlock.o mainrwlock.o
	$(LD) $(CFLAGS) $(RWLOCK) $(LDFLAGS) -o tecnicofs-rwlock lib/bst.o lib/hash.o fsrwlock.o mainrwlock.o

lib/hash.o: lib/hash.c lib/hash.h
	$(CC) $(CFLAGS) -o lib/hash.o -c lib/hash.c

lib/bst.o: lib/bst.c lib/bst.h
	$(CC) $(CFLAGS) -o lib/bst.o -c lib/bst.c

fs.o: fs.c fs.h lib/bst.h
	$(CC) $(CFLAGS) -o fs.o -c fs.c

main.o: main.c fs.h lib/bst.h lib/hash.h
	$(CC) $(CFLAGS) -o main.o -c main.c

fsmutex.o: fs.c fs.h lib/bst.h lib/hash.h
	$(CC) $(CFLAGS) $(MUTEX) -o fsmutex.o -c fs.c

mainmutex.o: main.c fs.h lib/bst.h lib/hash.h
	$(CC) $(CFLAGS) $(MUTEX) -o mainmutex.o -c main.c

fsrwlock.o: fs.c fs.h lib/bst.h lib/hash.h
	$(CC) $(CFLAGS) $(RWLOCK) -o fsrwlock.o -c fs.c

mainrwlock.o: main.c fs.h lib/bst.h lib/hash.h
	$(CC) $(CFLAGS) $(RWLOCK) -o mainrwlock.o -c main.c

clean:
	@echo Cleaning...
	rm -f lib/*.o *.o tecnicofs-nosync
	rm -f lib/*.o *.o tecnicofs-mutex
	rm -f lib/*.o *.o tecnicofs-rwlock

run: tecnicofs-nosync tecnicofs-mutex tecnicofs-rwlock
	./tecnicofs-nosync
	./tecnicofs-mutex
	./tecnicofs-rwlock

