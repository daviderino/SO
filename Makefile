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
TARGETS = tecnicofs-nosync tecnicofs-mutex tecnicofs-rwlock

.PHONY: all clean

all: $(TARGETS)

$(TARGETS):
	$(LD) $(CFLAGS) $^ -o $@ $(LDFLAGS)

### no sync ###
lib/bst.o: lib/bst.c lib/bst.h
lib/hash.o: lib/hash.c lib/hash.h
lib/inodes.o: lib/inodes.c lib/inodes.h tecnicofs-api-constants.h

api-tests/client-api-test-create.o: api-tests/client-api-test-create.c tecnicofs-api-constants.h tecnicofs-client-api.h
api-tests/client-api-test-delete.o: api-tests/client-api-test-delete.c tecnicofs-api-constants.h tecnicofs-client-api.h
api-tests/client-api-test-read.o: api-tests/client-api-test-read.c tecnicofs-api-constants.h tecnicofs-client-api.h
api-tests/client-api-test-success.o: api-tests/client-api-test-success.c tecnicofs-api-constants.h tecnicofs-client-api.h

fs.o: fs.c fs.h lib/bst.h lib/hash.h lib/inodes.h
sync.o: sync.c sync.h constants.h
sem.o: sem.c sem.h

main.o: main.c fs.h lib/bst.h lib/hash.o lib/inodes.o constants.h lib/timer.h sync.h sem.h
tecnicofs-nosync: lib/bst.o lib/hash.o lib/inodes.o fs.o sync.o sem.o main.o

### MUTEX ###
lib/bst-mutex.o: CFLAGS+=-DMUTEX
lib/bst-mutex.o: lib/bst.c lib/bst.h
lib/inodes.o: CFLAGS+=-DMUTEX
lib/inodes.o: lib/inodes.c lib/inodes.h tecnicofs-api-constants.h
lib/hash-mutex.o: CFLAGS+=-DMUTEX
lib/hash-mutex.o: lib/hash.c lib/hash.h

api-tests/client-api-test-create-mutex.o: CFLAGS+=-DMUTEX
api-tests/client-api-test-create-mutex.o: api-tests/client-api-test-create.c tecnicofs-api-constants.h tecnicofs-client-api.h
api-tests/client-api-test-delete-mutex.o: CFLAGS+=-DMUTEX
api-tests/client-api-test-delete-mutex.o: api-tests/client-api-test-delete.c tecnicofs-api-constants.h tecnicofs-client-api.h
api-tests/client-api-test-read-mutex.o: CFLAGS+=-DMUTEX
api-tests/client-api-test-read-mutex.o: api-tests/client-api-test-read.c tecnicofs-api-constants.h tecnicofs-client-api.h
api-tests/client-api-test-success-mutex.o: CFLAGS+=-DMUTEX
api-tests/client-api-test-success-mutex.o: api-tests/client-api-test-success.c tecnicofs-api-constants.h tecnicofs-client-api.h

fs-mutex.o: CFLAGS+=-DMUTEX
fs-mutex.o: fs.c fs.h lib/bst.h
sync-mutex.o: CFLAGS+=-DMUTEX
sync-mutex.o: sync.c sync.h constants.h
sem-mutex.o: CFLAGS+=-DMUTEX
sem-mutex.o: sem.c sem.h

main-mutex.o: CFLAGS+=-DMUTEX
main-mutex.o: main.c fs.h lib/bst.h constants.h lib/timer.h sync.h sem.h
tecnicofs-mutex: lib/bst-mutex.o lib/hash-mutex.o lib/inodes.o fs-mutex.o sync-mutex.o sem-mutex.o main-mutex.o

### RWLOCK ###
lib/bst-rwlock.o: CFLAGS+=-DRWLOCK
lib/bst-rwlock.o: lib/bst.c lib/bst.h
lib/inodes-rwlock.o: CFLAGS+=-DRWLOCK
lib/inodes-rwlock.o: lib/inodes.c lib/inodes.h tecnicofs-api-constants.h
lib/hash-rwlock.o: CFLAGS+=-DRWLOCK
lib/hash-rwlock.o: lib/hash.c lib/hash.h

api-tests/client-api-test-create-rwlock.o: CFLAGS+=-DRWLOCK
api-tests/client-api-test-create-rwlock.o: api-tests/client-api-test-create.c tecnicofs-api-constants.h tecnicofs-client-api.h
api-tests/client-api-test-delete-rwlock.o: CFLAGS+=-DRWLOCK
api-tests/client-api-test-delete-rwlock.o: api-tests/client-api-test-delete.c tecnicofs-api-constants.h tecnicofs-client-api.h
api-tests/client-api-test-read-rwlock.o: CFLAGS+=-DRWLOCK
api-tests/client-api-test-read-rwlock.o: api-tests/client-api-test-read.c tecnicofs-api-constants.h tecnicofs-client-api.h
api-tests/client-api-test-success-rwlock.o: CFLAGS+=-DRWLOCK
api-tests/client-api-test-success-rwlock.o: api-tests/client-api-test-success.c tecnicofs-api-constants.h tecnicofs-client-api.h


fs-rwlock.o: CFLAGS+=-DRWLOCK
fs-rwlock.o: fs.c fs.h lib/bst.h
sync-rwlock.o: CFLAGS+=-DRWLOCK
sync-rwlock.o: sync.c sync.h constants.h
sem-rwlock.o: CFLAGS+=-DRWLOCK
sem-rwlock.o: sem.c sem.h

main-rwlock.o: CFLAGS+=-DRWLOCK
main-rwlock.o: main.c fs.h lib/bst.h lib/hash.o lib/inodes.h constants.h lib/timer.h sync.h sem.h
tecnicofs-rwlock: lib/bst-rwlock.o lib/hash-rwlock.o lib/inodes-rwlock.o fs-rwlock.o sync-rwlock.o sem-rwlock.o main-rwlock.o

%.o:
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	@echo Cleaning...
	rm -f lib/*.o *.o  $(TARGETS)
