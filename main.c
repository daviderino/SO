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

// Macros para escolher em qual dos locks operar
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

/* Da lock com mutex ou rwlock write, consoante o argumento recebido*/
void lockMutexOrWrite(const char *lock) {
	if(!strcmp(lock, COMMAND)) {
		#if RWLOCK
			if(pthread_rwlock_wrlock(&fs->rwlockcommand) != 0 ) {
                fprintf(stderr, "Error while RW Locking (wr)");
                exit(EXIT_FAILURE);
            }
		#elif MUTEX
			if(pthread_mutex_lock(&fs->mutexlockcommand) != 0) {
                fprintf(stderr, "Error while Mutex locking");
                exit(EXIT_FAILURE);
            }
		#endif
	}
	else if (!strcmp(lock, OPERATION)) {
		#if RWLOCK
			if(pthread_rwlock_wrlock(&fs->rwlockoperation)) {
                fprintf(stderr, "Error while RW Locking (wr)");
                exit(EXIT_FAILURE);
            }
		#elif MUTEX
			if(pthread_mutex_lock(&fs->mutexlockoperation) != 0 ) {
                fprintf(stderr, "Error while Mutex locking");
                exit(EXIT_FAILURE);
            }
		#endif
	}
}

/* Da lock com mutex ou rwlock read, consoante o argumento recebido*/
void lockMutexOrRead(const char *lock) {
	if(!strcmp(lock, COMMAND)) {
		#if RWLOCK
			if(pthread_rwlock_rdlock(&fs->rwlockcommand) != 0) {
                fprintf(stderr, "Error while RW Locking (rd)");
                exit(EXIT_FAILURE);
            }
		#elif MUTEX
			if(pthread_mutex_lock(&fs->mutexlockcommand) != 0) {
                fprintf(stderr, "Error while Mutex locking");
                exit(EXIT_FAILURE);
            }
		#endif
	}
	else if (!strcmp(lock, OPERATION)) {
		#if RWLOCK
			if(pthread_rwlock_rdlock(&fs->rwlockoperation)) {
                fprintf(stderr, "Error while RW Locking (rd)");
                exit(EXIT_FAILURE);
            }
		#elif MUTEX
			if(pthread_mutex_lock(&fs->mutexlockoperation) != 0 ) {
                fprintf(stderr, "Error while Mutex locking");
                exit(EXIT_FAILURE);
            }
		#endif
	}
}

/* Da unlock do lock mutex ou rwlock */
void unlockMutexOrRW(const char *lock) {
	if(!strcmp(lock, COMMAND)) {
		#if RWLOCK
			if(pthread_rwlock_unlock(&fs->rwlockcommand) != 0) {
                fprintf(stderr, "Error while RW unlocking");
                exit(EXIT_FAILURE);
            }
		#elif MUTEX
			if(pthread_mutex_unlock(&fs->mutexlockcommand) != 0) {
                fprintf(stderr, "Error while Mutex unlocking");
                exit(EXIT_FAILURE);
            }
		#endif
	}
	else if (!strcmp(lock, OPERATION)) {
		#if RWLOCK
			if(pthread_rwlock_unlock(&fs->rwlockoperation) != 0) {
                fprintf(stderr, "Error while RW unlocking");
                exit(EXIT_FAILURE);
            }
		#elif MUTEX
			 if(pthread_mutex_unlock(&fs->mutexlockoperation) != 0) {
                fprintf(stderr, "Error while Mutex unlocking");
                exit(EXIT_FAILURE);
            }
		#endif
	}
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
		lockMutexOrWrite(COMMAND);
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

        if(token != 'c'){
			unlockMutexOrRW(COMMAND);
        }

        int searchResult;
        int iNumber;
        switch (token) {
            case 'c':
                iNumber = obtainNewInumber(fs);
				unlockMutexOrRW(COMMAND);
				lockMutexOrWrite(OPERATION);
                create(fs, name, iNumber);
				unlockMutexOrRW(OPERATION);
                break;

             case 'l':
			 	lockMutexOrRead(OPERATION);
                searchResult = lookup(fs, name);
				unlockMutexOrRW(OPERATION);
                if(!searchResult)
                    printf("%s not found\n", name);
                else
                    printf("%s found with inumber %d\n", name, searchResult);
                break;

            case 'd':
				lockMutexOrWrite(OPERATION);
                delete(fs, name);
				unlockMutexOrRW(OPERATION);
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

    fs = new_tecnicofs();
    processInput();

    if(gettimeofday(&start, NULL)!=0)
         fprintf(stderr, "Error while initializing the time\n");

    createThreadPool();
    print_tecnicofs_tree(outputFile, fs);

    free_tecnicofs(fs);

    if(gettimeofday(&end, NULL)!=0)
        fprintf(stderr, "Error while initializing the time\n");

    t = (end.tv_sec - start.tv_sec) + ((end.tv_usec - start.tv_usec)/1000000.0);
    print_time(t);

    exit(EXIT_SUCCESS);
}
