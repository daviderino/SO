# Makefile, versao 1
# Sistemas Operativos, DEI/IST/ULisboa 2019-20

SOURCES = main.c fs.c sync.c sem.c
SOURCES+= lib/bst.c
OBJS_NOSYNC = $(SOURCES:%.c=%.o)
OBJS_MUTEX  = $(SOURCES:%.c=%-mutex.o)
OBJS_RWLOCK = $(SOURCES:%.c=%-rwlock.o)
OBJS = $(OBJS_NOSYNC) $(OBJS_MUTEX) $(OBJS_RWLOCK)
CC   = gcc
LD   = gcc
CFLAGS =-Wall -std=gnu99 -I../ -g
LDFLAGS=-lm -pthread
TARGETS = tecnicofs-rwlock

.PHONY: all clean

all: $(TARGETS)

$(TARGETS):
	$(LD) $(CFLAGS) $^ -o $@ $(LDFLAGS)

### RWLOCK ###
lib/bst-rwlock.o: CFLAGS+=-DRWLOCK
lib/bst-rwlock.o: lib/bst.c lib/bst.h
lib/inodes-rwlock.o: CFLAGS+=-DRWLOCK
lib/inodes-rwlock.o: lib/inodes.c lib/inodes.h tecnicofs-api-constants.h
lib/hash-rwlock.o: CFLAGS+=-DRWLOCK
lib/hash-rwlock.o: lib/hash.c lib/hash.h

fs-rwlock.o: CFLAGS+=-DRWLOCK
fs-rwlock.o: fs.c fs.h lib/bst.h
sync-rwlock.o: CFLAGS+=-DRWLOCK
sync-rwlock.o: sync.c sync.h constants.h
sem-rwlock.o: CFLAGS+=-DRWLOCK
sem-rwlock.o: sem.c sem.h
sock-rwlock.o: CFLAGS+=-DRWLOCK
sock-rwlock.o: sock.c sock.h

main-rwlock.o: CFLAGS+=-DRWLOCK
main-rwlock.o: main.c fs.h lib/bst.h lib/hash.h lib/inodes.h constants.h lib/timer.h sync.h sem.h
tecnicofs-rwlock: lib/bst-rwlock.o lib/hash-rwlock.o lib/inodes-rwlock.o fs-rwlock.o sync-rwlock.o sem-rwlock.o sock-rwlock.o main-rwlock.o

%.o:
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	@echo Cleaning...
	rm -f lib/*.o *.o  $(TARGETS)
