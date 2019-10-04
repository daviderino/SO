# Makefile, versao 1
# Sistemas Operativos, DEI/IST/ULisboa 2019-20

CC   = gcc
LD   = gcc
CFLAGS =-Wall -g -pthread -std=gnu99 -I../
LDFLAGS=-lm
MUTEXFLAG=-DMUTEX
RWLOCKFLAG=-DRWLOCK

# A phony target is one that is not really the name of a file
# https://www.gnu.org/software/make/manual/html_node/Phony-Targets.html
.PHONY: all clean run

all: tecnicofs-nosync tecnicofs-mutex tecnicofs-rwlock

tecnicofs-nosync: lib/bst.o fs.o main_nosync.o
	$(LD) $(CFLAGS) $(LDFLAGS) -o tecnicofs-nosync lib/bst.o fs.o main_nosync.o

tecnicofs-mutex: lib/bst.o fs.o main_mutex.o
	$(LD) $(CFLAGS) $(LDFLAGS) -o tecnicofs-mutex lib/bst.o fs.o main_mutex.o

tecnicofs-rwlock: lib/bst.o fs.o main_rwlock.o
	$(LD) $(CFLAGS) $(LDFLAGS) -o tecnicofs-rwlock lib/bst.o fs.o main_rwlock.o

lib/bst.o: lib/bst.c lib/bst.h
	$(CC) $(CFLAGS) -o lib/bst.o -c lib/bst.c

fs.o: fs.c fs.h lib/bst.h
	$(CC) $(CFLAGS) -o fs.o -c fs.c

main_nosync.o: main.c fs.h lib/bst.h
	$(CC) $(CFLAGS) -o main_nosync.o -c main.c

main_mutex.o: main.c fs.h lib/bst.h
	$(CC) $(CFLAGS) $(MUTEXFLAG) -o main_mutex.o -c main.c

main_rwlock.o: main.c fs.h lib/bst.h
	$(CC) $(CFLAGS) $(RWLOCKFLAG) -o main_rwlock.o -c main.c

clean:
	@echo Cleaning...
	rm -f lib/*.o *.o tecnicofs-nosync tecnicofs-mutex tecnicofs-rwlock

run: tecnicofs
	./tecnicofs-nosync
