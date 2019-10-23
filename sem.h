#ifndef SEM_H
#define SEM_H

#include <semaphore.h>

void semMech_init(sem_t *sem, int control);
void semMech_wait(sem_t *sem);
void semMech_post(sem_t *sem);
void semMech_destroy(sem_t *sem);

#endif