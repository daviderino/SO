#include <stdio.h>
#include <stdlib.h>
#include "sem.h"

void semMech_init(sem_t *sem, int control) {
    if(sem_init(sem, 0, control) != 0) {
        perror("semMech_init failed\n");
        exit(EXIT_FAILURE);
    }
}

void semMech_wait(sem_t *sem) {
    if(sem_wait(sem) != 0) {
        perror("semMech_wait failed\n");
        exit(EXIT_FAILURE);
    }
}

void semMech_post(sem_t *sem) {
    if(sem_post(sem) != 0) {
        perror("semMech_post failed\n");
        exit(EXIT_FAILURE);
    }
}

void semMech_destroy(sem_t *sem){
    if(sem_destroy(sem) != 0) {
        perror("semMech_destroy failed\n");
        exit(EXIT_FAILURE);
    }
}
