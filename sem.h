#ifndef SEM_H
#define SEM_H

#include <semaphore.h>

#define semMech sem_t

void semMech_init(semMech *sem, int control);
void semMech_wait(semMech *sem);
void semMech_post(semMech *sem);
void semMech_destroy(semMech *sem);

#endif