# Makefile, versao 1
# Sistemas Operativos, DEI/IST/ULisboa 2019-20

CC      = gcc
LD      = gcc
CFLAGS  =-Wall -pthread -std=gnu99 -I../
RWLOCK  = -DRWLOCK
LDFLAGS = -lm

# A phony target is one that is not really the name of a file
# https://www.gnu.org/software/make/manual/html_node/Phony-Targets.html
.PHONY: all clean run

all: tecnicofs


tecnicofs: lib/bst.o lib/hash.o lib/inodes.o fsrwlock.o mainrwlock.o
	$(LD) $(CFLAGS) $(RWLOCK) $(LDFLAGS) -o tecnicofs lib/bst.o lib/inodes.o lib/hash.o fsrwlock.o mainrwlock.o

lib/hash.o: lib/hash.c lib/hash.h
	$(CC) $(CFLAGS) -o lib/hash.o -c lib/hash.c

lib/bst.o: lib/bst.c lib/bst.h
	$(CC) $(CFLAGS) -o lib/bst.o -c lib/bst.c

lib/inodes.o: lib/inodes.c lib/inodes.h
	$(CC) $(CFLAGS) -o lib/inodes.o -c lib/inodes.c

fs.o: fs.c fs.h lib/bst.h lib/inodes.h
	$(CC) $(CFLAGS) -o fs.o -c fs.c

fsrwlock.o: fs.c fs.h lib/bst.h lib/hash.h lib/inodes.h
	$(CC) $(CFLAGS) $(RWLOCK) -o fsrwlock.o -c fs.c

mainrwlock.o: main.c fs.h lib/bst.h lib/hash.h lib/inodes.h tecnicofs-api-constants.h
	$(CC) $(CFLAGS) $(RWLOCK) -o mainrwlock.o -c main.c


clean:
	@echo Cleaning...
	rm -f lib/*.o *.o tecnicofs

run: tecnicofs
	./tecnicofs