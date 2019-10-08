#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <math.h>
#include <sys/time.h>
#include "fs.h"

#define MAX_COMMANDS 150000
#define MAX_INPUT_SIZE 100

#define COMMAND "command"
#define OPERATION "operation"

int numberThreads = 0;
tecnicofs* fs;

char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
char *inputFile, *outputFile;

int numberCommands = 0;
int headQueue = 0;

int lockInit() {
	#ifdef MUTEX
        int a, b;
        a = pthread_mutex_init(&fs->mutexlockcommand, NULL);
        b = pthread_mutex_init(&fs->mutexlockoperation, NULL);
		return a || b;
	#elif RWLOCK
        int a, b;
        a = pthread_rwlock_init(&fs->rwlockcommand, NULL);
        b = pthread_rwlock_init(&fs->rwlockoperation, NULL);
		return a || b;
	#endif
	return 0;
}

void lockWrite(const char *lock) {
    #if RWLOCK
        if(!strcmp(lock, COMMAND)) {
            if(pthread_rwlock_wrlock(&fs->rwlockcommand) != 0 ) {
                printf("Error while RW Locking (wr)");
            }
        }
        else if (!strcmp(lock, OPERATION)) {
        	if(pthread_rwlock_wrlock(&fs->rwlockoperation)){
                printf("Error while RW Locking (wr)");
            }
        }
	#endif
}

void lockRead(const char *lock) {
    #if RWLOCK
        if(!strcmp(lock, COMMAND)) {
		    if(pthread_rwlock_rdlock(&fs->rwlockcommand) != 0) {
                printf("Error while RW Locking (rd)");
            }
        }
        else if (!strcmp(lock, OPERATION)) {
        	if(pthread_rwlock_rdlock(&fs->rwlockoperation)) {
                printf("Error while RW Locking (rd)");
            }
        }
	#endif
}

void lockMutex(const char *lock) {
    #ifdef MUTEX
        if(!strcmp(lock, COMMAND)) {
		    if(pthread_mutex_lock(&fs->mutexlockcommand) != 0) {
                printf("Error while Mutex locking");
            }
        }
        else if (!strcmp(lock, OPERATION)) {
        	if(pthread_mutex_lock(&fs->mutexlockoperation) != 0 ) {
                printf("Error while Mutex locking");
            }
        }
    #endif
}

void unlockMutex(const char *lock) {
	#ifdef MUTEX
        if(!strcmp(lock, COMMAND)) {
		    if(pthread_mutex_unlock(&fs->mutexlockcommand) != 0) {
                printf("Error while Mutex unlockiog");
            }
        }
        else if (!strcmp(lock, OPERATION)) {
		    if(pthread_mutex_unlock(&fs->mutexlockoperation) != 0) {
                printf("Error while Mutex unlocking");
            }
        }
	#endif
}

void unlockRWLock(const char *lock) {
    #ifdef RWLOCK
        if(!strcmp(lock, COMMAND)) {
            if(pthread_rwlock_unlock(&fs->rwlockcommand) != 0) {
                printf("Error while RW unlocking");
            }
        }
        else if (!strcmp(lock, OPERATION)) {
		    if(pthread_rwlock_unlock(&fs->rwlockoperation) != 0) {
                printf("Error while RW unlocking");
            }
        }
    #endif
}

static void displayUsage (const char* appName){
    printf("Usage: %s\n", appName);
    exit(EXIT_FAILURE);
}

static void parseArgs (long argc, char* const argv[]){
    if (argc != 4) {
        fprintf(stderr, "Invalid format:\n");
        displayUsage("./tecnicofs inputfile outputfile numthreads");
    }

    numberThreads = (int)strtol(argv[3], NULL, 10);
    inputFile = argv[1];
    outputFile = argv[2];
}

int insertCommand(char* data) {
    if(numberCommands != MAX_COMMANDS) {
        strcpy(inputCommands[numberCommands++], data);
        return 1;
    }
    return 0;
}

char* removeCommand() {
    if((numberCommands > 0)){
        numberCommands--;
        return inputCommands[headQueue++];
    }
    return NULL;
}

void errorParse(){
    fprintf(stderr, "Error: command invalid\n");
    exit(EXIT_FAILURE);
}

void processInput(){
    FILE *file=fopen(inputFile, "r");;
	char line[MAX_INPUT_SIZE];

    if(file == NULL) {
    	fprintf(stderr, "Error: invalid file");
    }

    while (fgets(line, sizeof(line)/sizeof(char), file)) {
        char token;
        char name[MAX_INPUT_SIZE];

        int numTokens = sscanf(line, "%c %s", &token, name);

        /* perform minimal validation */
        if (numTokens < 1) {
            continue;
        }
        switch (token) {
            case 'c':
                if(numTokens != 2)
                    errorParse();
                if(insertCommand(line))
                    break;
            case 'l':
                if(numTokens != 2)
                    errorParse();
                if(insertCommand(line))
                    break;
            case 'd':
                if(numTokens != 2)
                    errorParse();
                if(insertCommand(line))
                    break;
            case '#':
                break;
            default: { /* error */
                errorParse();
            }
        }
    }

    fclose(file);
}

void applyCommands() {
    while(numberCommands > 0) {
        lockMutex(COMMAND);
        lockRead(COMMAND);
        const char* command = removeCommand();
        
        if (command == NULL) {
            unlockRWLock(COMMAND);
            unlockMutex(COMMAND);
            return;
        }
   
        char token;
        char name[MAX_INPUT_SIZE];

        int numTokens = sscanf(command, "%c %s", &token, name);

        if (numTokens != 2) {
            fprintf(stderr, "Error: invalid command in Queue\n");
            exit(EXIT_FAILURE);
        }

        if(token != 'c'){
            unlockRWLock(COMMAND);
            unlockMutex(COMMAND);
        }

        int searchResult;
        int iNumber;
        switch (token) {
            case 'c':
                iNumber = obtainNewInumber(fs);
                unlockRWLock(COMMAND);
                unlockMutex(COMMAND);
                lockMutex(OPERATION);
                lockWrite(OPERATION);             
                create(fs, name, iNumber);
                unlockRWLock(OPERATION);
                unlockMutex(OPERATION);
                break;

             case 'l':
                lockMutex(OPERATION);
                lockRead(OPERATION);
                searchResult = lookup(fs, name);
                unlockRWLock(OPERATION);
                unlockMutex(OPERATION);
                if(!searchResult)
                    printf("%s not found\n", name);
                else
                    printf("%s found with inumber %d\n", name, searchResult);
                break;

            case 'd':
                lockMutex(OPERATION);
                lockWrite(OPERATION);
                delete(fs, name);
                unlockRWLock(OPERATION);
                unlockMutex(OPERATION);
                break;

            default: { /* error */
                fprintf(stderr, "Error: command to apply\n");
                exit(EXIT_FAILURE);
            }
        }   
    }
}

void *threadApplyCommands() {
    applyCommands();
    pthread_exit(NULL);
}

void createThreadPool() {
    pthread_t *threads;
    int i;

    #if !defined(MUTEX) && !defined(RWLOCK)
        numberThreads = 1;
    #endif


    threads = (pthread_t*)malloc(sizeof(pthread_t) * numberThreads);

	if(lockInit() != 0) {
        fprintf(stderr, "Error while creating a lock\n");
	}

    for(i = 0; i < numberThreads; i++) {
        if(pthread_create(&threads[i], NULL, threadApplyCommands, NULL) != 0) {
            fprintf(stderr, "Error while creating a thread\n");
        }
    }

    for(i = 0; i < numberThreads; i++) {
        if(pthread_join(threads[i], NULL) != 0) {
            fprintf(stderr, "Error while joining a thread\n");
        }
    }
    
    free(threads);
}

void print_time(double t) {
    FILE *file= fopen(outputFile, "a");
    fprintf(file, "Program executed in %.4f seconds\n", t);
    printf( "Program executed in %.4f seconds\n", t); // delete later
    fclose(file);
}

int main(int argc, char* argv[]) {
    struct timeval start, end;
    double t;
    gettimeofday(&start, NULL);

    parseArgs(argc, argv);

    fs = new_tecnicofs();
    processInput();

    createThreadPool();
    print_tecnicofs_tree(outputFile, fs);

    free_tecnicofs(fs);
    gettimeofday(&end, NULL);

    t = (end.tv_sec - start.tv_sec) + ((end.tv_usec - start.tv_usec)/1000000.0);
    print_time(t);

    exit(EXIT_SUCCESS);
}
