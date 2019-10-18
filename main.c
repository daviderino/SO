#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <semaphore.h>
#include <math.h>
#include <sys/time.h>
#include "fs.h"

#define MAX_COMMANDS 150000
#define MAX_INPUT_SIZE 100

// Macros para escolher em qual dos locks operar
#define COMMAND "command"       
#define OPERATION "operation"   

int numberThreads = 0;
tecnicofs* fs;

char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
char *inputFile, *outputFile;
int numberBuckets;

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

void lockMutex(pthread_mutex_t lock) {
    #if MUTEX
    if(pthread_mutex_lock(&lock) != 0) {
        fprintf(stderr, "Error while Mutex locking");
        exit(EXIT_FAILURE);
    }
    #endif
}

void lockWrite(pthread_rwlock_t lock) {
    #if RWLOCK
		if(pthread_rwlock_wrlock(&lock) != 0 ) {
            fprintf(stderr, "Error while RW Locking (wr)");
            exit(EXIT_FAILURE);
        }
    #endif
}

void lockWrite(pthread_rwlock_t lock) {
    #if RWLOCK
		if(pthread_rwlock_rdlock(&lock) != 0 ) {
            fprintf(stderr, "Error while RW Locking (wr)");
            exit(EXIT_FAILURE);
        }
    #endif
}

void unlockMutex(pthread_mutex_t lock) {
    #if MUTEX
        if(pthread_mutex_unlock(&lock) != 0) {
            fprintf(stderr, "Error while Mutex unlocking");
            exit(EXIT_FAILURE);
        }
    #endif
}

void unlockRWLock(pthread_rwlock_t lock) {
    if(pthread_mutex_unlock(&lock) != 0) {
        fprintf(stderr, "Error while RWLock unlocking");
        exit(EXIT_FAILURE);
    }
}

static void displayUsage (const char* appName){
    printf("Usage: %s\n", appName);
    exit(EXIT_FAILURE);
}

static void parseArgs (long argc, char* const argv[]){
    if (argc != 5) {
        fprintf(stderr, "Invalid format:\n");
        displayUsage("./tecnicofs inputfile outputfile numthreads numbuckets");
    }

    inputFile = argv[1];
    outputFile = argv[2];
    numberThreads = (int)strtol(argv[3], NULL, 10);
    numberBuckets = argv[4];
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
        exit(EXIT_FAILURE);
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
		lockMutex(fs->mutexlockcommand);
        lockWrite(fs->rwlockcommand);
        const char* command = removeCommand();
        
        if (command == NULL) {
			unlockMutexOrRW(COMMAND);
            return;
        }
   
        char token;
        char name[MAX_INPUT_SIZE];

        int numTokens = sscanf(command, "%c %s", &token, name);

        if (numTokens != 2) {
            fprintf(stderr, "Error: invalid command in Queue\n");
            exit(EXIT_FAILURE);
        }

        if(token != 'c') {
            unlockMutex(fs->mutexlockcommand);
            unlockRWLock(fs->rwlockcommand);
        }

        int searchResult;
        int iNumber;
        switch (token) {
            case 'c':
                iNumber = obtainNewInumber(fs);
                unlockMutex(fs->mutexlockcommand);
                unlockRWLock(fs->rwlockcommand);
	            lockMutex(fs->mutexlockoperation);
                lockWrite(fs->rwlockoperation);
                create(fs, name, iNumber);
                unlockMutex(fs->mutexlockoperation);
                unlockRWLock(fs->rwlockoperation);
                break;

             case 'l':
                lockMutex(fs->mutexlockoperation);
                lockRead(fs->rwlockoperation);
                searchResult = lookup(fs, name);
                unlockMutex(fs->mutexlockoperation);
                unlockRWLock(fs->rwlockoperation);
                if(!searchResult)
                    printf("%s not found\n", name);
                else
                    printf("%s found with inumber %d\n", name, searchResult);
                break;

            case 'd':
                lockMutex(fs->mutexlockoperation);
                lockWrite(fs->rwlockoperation);                
                delete(fs, name);
                unlockMutex(fs->mutexlockoperation);
                unlockRWLock(fs->rwlockoperation);
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

/* Cria as threads todas, efetua a verificacao de erros e faz join das respetivas threads*/
void createThreadPool() {
    pthread_t *threads;
    int i;

    #if !defined(MUTEX) && !defined(RWLOCK)
        numberThreads = 1;
    #endif


    threads = (pthread_t*)malloc(sizeof(pthread_t) * numberThreads);

	if(lockInit() != 0) {
        fprintf(stderr, "Error while creating a lock\n");
        exit(EXIT_FAILURE);
	}

    for(i = 0; i < numberThreads; i++) {
        if(pthread_create(&threads[i], NULL, threadApplyCommands, NULL) != 0) {
            fprintf(stderr, "Error while creating a thread\n");
            exit(EXIT_FAILURE);
        }
    }

    for(i = 0; i < numberThreads; i++) {
        if(pthread_join(threads[i], NULL) != 0) {
            fprintf(stderr, "Error while joining a thread\n");
            exit(EXIT_FAILURE);
        }
    }

    if(pthread_mutex_destroy(&fs->mutexlockcommand) != 0) {
        fprintf(stderr, "Error while destroying a mutex(command)\n");
        exit(EXIT_FAILURE);
    };

    if(pthread_mutex_destroy(&fs->mutexlockoperation) != 0) {
        fprintf(stderr, "Error while destroying a mutex(operation)\n");
        exit(EXIT_FAILURE);
    };

    if(pthread_rwlock_destroy(&fs->rwlockcommand) != 0) {
        fprintf(stderr, "Error while destroying a rwlock(command)\n");
        exit(EXIT_FAILURE);
    };

    if(pthread_rwlock_destroy(&fs->rwlockoperation) != 0) {
        fprintf(stderr, "Error while destroying a rwlock(operation)\n");
        exit(EXIT_FAILURE);
    };
    
    free(threads);
}

void print_time(double t) {
    FILE *file= fopen(outputFile, "a");
    fprintf(file, "TecnicoFS completed in %.4f seconds.\n", t);
    printf( "TecnicoFS completed in %.4f seconds.\n", t);
    fclose(file);
}

int main(int argc, char* argv[]) {
    struct timeval start, end;
    double t;

    parseArgs(argc, argv);

    fs = new_tecnicofs(numberBuckets);
    fs->hashSize = numberBuckets;
    
    if(gettimeofday(&start, NULL) != 0) {
        fprintf(stderr, "Error while initializing the time\n");
        exit(EXIT_FAILURE);
    }

    processInput();

    createThreadPool();
    print_tecnicofs_tree(outputFile, fs);

    free_tecnicofs(fs);

    if(gettimeofday(&end, NULL)!=0) {
        fprintf(stderr, "Error while initializing the time\n");
        exit(EXIT_FAILURE);
    }

    t = (end.tv_sec - start.tv_sec) + ((end.tv_usec - start.tv_usec)/1000000.0);
    print_time(t);

    exit(EXIT_SUCCESS);
}
