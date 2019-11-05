#ifndef FS_H
#define FS_H

#include "sync.h"
#include "sem.h"
#include "lib/bst.h"

typedef struct tecnicofs {
    node **hashTable;
    int hashSize;
    int nextINumber;
    syncMech *bstLock;
} tecnicofs;

int obtainNewInumber(tecnicofs* fs);
tecnicofs* new_tecnicofs();
void free_tecnicofs(tecnicofs* fs);
void create(tecnicofs* fs, char *name, int inumber);
void delete(tecnicofs* fs, char *name);
int lookup(tecnicofs* fs, char *name);
void swap_name(tecnicofs* fs, char *oldNodeName, char *newNodeName);
void print_tecnicofs_tree(FILE * fp, tecnicofs *fs);

#endif
