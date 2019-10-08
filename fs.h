#ifndef FS_H
#define FS_H
#include "lib/bst.h"
#include <pthread.h>

typedef struct tecnicofs {
    node* bstRoot;
    int nextINumber;
	pthread_mutex_t mutexlockcommand;
    pthread_mutex_t mutexlockoperation;
	pthread_rwlock_t rwlockcommand;
    pthread_rwlock_t rwlockoperation;
} tecnicofs;

int obtainNewInumber(tecnicofs* fs);
tecnicofs* new_tecnicofs();
void free_tecnicofs(tecnicofs* fs);
void create(tecnicofs* fs, char *name, int inumber);
void delete(tecnicofs* fs, char *name);
int lookup(tecnicofs* fs, char *name);
void print_tecnicofs_tree(char* title, tecnicofs *fs);

#endif /* FS_H */
