#include "sync.h"
#include <stdio.h>
#include <stdlib.h>

void sync_init(syncMech* sync) {
    if(syncMech_init(sync, NULL) != 0) {
        perror("sync_init failed\n");
        exit(EXIT_FAILURE);
    }
}

void sync_destroy(syncMech* sync) {
    if(syncMech_destroy(sync) != 0) {
        perror("sync_destroy failed\n");
        exit(EXIT_FAILURE);
    }
}

void sync_wrlock(syncMech* sync) {
    if(syncMech_wrlock(sync) != 0) {
        perror("sync_wrlock failed");
        exit(EXIT_FAILURE);
    }
}

void sync_rdlock(syncMech* sync) {
    if(syncMech_rdlock(sync) != 0) {
        perror("sync_rdlock failed");
        exit(EXIT_FAILURE);
    }
}

void sync_unlock(syncMech* sync) {
    if(syncMech_unlock(sync) != 0) {
        perror("sync_unlock failed");
        exit(EXIT_FAILURE);
    }
}

void mutex_init(pthread_mutex_t* mutex) {
    if(pthread_mutex_init(mutex, NULL) != 0) {
        perror("mutex_init failed\n");
        exit(EXIT_FAILURE);
    }
}

void mutex_destroy(pthread_mutex_t* mutex) {
    if(pthread_mutex_destroy(mutex) != 0) {
        perror("mutex_destroy failed\n");
        exit(EXIT_FAILURE);
    }
}

void mutex_lock(pthread_mutex_t* mutex) {
    if(pthread_mutex_lock(mutex) != 0) {
        perror("mutex_lock failed");
        exit(EXIT_FAILURE);
    }
}

void mutex_unlock(pthread_mutex_t* mutex) {
    if(pthread_mutex_unlock(mutex) != 0) {
        perror("mutex_unlock failed");
        exit(EXIT_FAILURE);
    }
}

int do_nothing(void* a) {
    (void)a;
    return 0;
}
