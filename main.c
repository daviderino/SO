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

int numberThreads = 0;
tecnicofs* fs;

char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
char *inputFile, *outputFile;

int numberCommands = 0;
int headQueue = 0;


int lockInit() {
	#ifdef MUTEX
		return pthread_mutex_init(&fs->mutexlock, NULL);
	#elif RWLOCK
		return pthread_rwlock_init(&fs->rwlock, NULL);
	#endif
	return 1;
}

void lockWrite() {
	#ifdef MUTEX
		pthread_mutex_lock(&fs->mutexlock);
	#elif RWLOCK
		pthread_rwlock_wrlock(&fs->rwlock);
	#endif
}

void lockRead() {
	#ifdef MUTEX
		pthread_mutex_lock(&fs->mutexlock);
	#elif RWLOCK
		pthread_rwlock_rdlock(&fs->rwlock);
	#endif
}

void unlock() {
	#ifdef MUTEX
		pthread_mutex_unlock(&fs->mutexlock);
	#elif RWLOCK
		pthread_rwlock_unlock(&fs->rwlock);
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
        lockRead();
        const char* command = removeCommand();
		unlock();

        if (command == NULL) {
            return;
        }

        char token;
        char name[MAX_INPUT_SIZE];

        // acho que aqui não e preciso
        int numTokens = sscanf(command, "%c %s", &token, name);

        if (numTokens != 2) {
            fprintf(stderr, "Error: invalid command in Queue\n");
            exit(EXIT_FAILURE);
        }

        int searchResult;
        int iNumber;
        switch (token) {
            case 'c':
                lockWrite();
                printf("Creating %s\n", name);  // TODO: Delete this line
                iNumber = obtainNewInumber(fs);
                create(fs, name, iNumber);
               	unlock();
                break;

            case 'l':
                lockRead();
                searchResult = lookup(fs, name);
                if(!searchResult)
                    printf("%s not found\n", name);
                else
                    printf("%s found with inumber %d\n", name, searchResult);
				unlock();
                break;

            case 'd':
                lockWrite();
                printf("Deleting %s\n", name); // TODO: Delete this line
                delete(fs, name);
                unlock();
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

    threads = (pthread_t*)malloc(sizeof(pthread_t) * numberThreads);

	if(lockInit() != 0) {
        fprintf(stderr, "Error while creating a mutex\n");
	}

    for(i = 0; i < numberThreads; i++) {
        if(pthread_create(&threads[i], NULL, threadApplyCommands, NULL) != 0) {
            fprintf(stderr, "Error while creating a thread\n");
        }
    }

    for(i = 0; i < numberThreads; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            fprintf(stderr, "Error while joining a thread\n");
        }
    }
    
    free(threads);
}

void print_time(double t) {
    FILE *file= fopen(outputFile, "a");
    fprintf(file, "Program executed in %g seconds\n", t);
    printf( "Program executed in %g seconds\n", t); // delete later
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
