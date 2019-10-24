#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <math.h>
#include "constants.h"
#include "fs.h"
#include "sem.h"
#include "sync.h"
#include "lib/timer.h"

tecnicofs* fs;
pthread_mutex_t commandsLock;

sem_t semMechProcessInput;
sem_t semMechCommands;

char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
char *global_inputfile = NULL;
char *global_outputfile = NULL;

int numberCommands = 0;
int headQueue = 0;

int numberThreads = 0;
int numberBuckets = 0;


FILE *openOutputFile() {
    FILE *fp;
    fp = fopen(global_outputfile, "w");
    if(!fp) {
        perror("Error opening output file");
        exit(EXIT_FAILURE);
    }
    return fp;
}

void closeOutputFile(FILE *outputFp) {
    fflush(outputFp);
    fclose(outputFp);
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

    global_inputfile = argv[1];
    global_outputfile = argv[2];
    numberThreads = (int)strtol(argv[3], NULL, 10);
    numberBuckets = (int)strtol(argv[4], NULL, 10);

    if(!numberThreads) {
        fprintf(stderr, "Invalid thread number\n");
    }
}

int insertCommand(char* data) {
    if(numberCommands != MAX_COMMANDS) {
        strcpy(inputCommands[numberCommands++], data);
        numberCommands = numberCommands % MAX_COMMANDS;
        return 1;
    }
    return 0;
}

char* removeCommand() {
    if((numberCommands > 0)){
        char *ret = inputCommands[headQueue];
        headQueue = (headQueue + 1) % MAX_COMMANDS;
        return ret;
    }
    return NULL;
}

void errorParse(int lineNumber){
    fprintf(stderr, "Error: line %d invalid\n", lineNumber);
    exit(EXIT_FAILURE);
}

void *processInput(){
    FILE *inputFile = fopen(global_inputfile, "r");;
	char line[MAX_INPUT_SIZE];
    int lineNumber = 0;

    if(!inputFile) {
    	fprintf(stderr, "Error: %s is an invalid file\n", global_inputfile);
        exit(EXIT_FAILURE);
    }

    while (fgets(line, sizeof(line)/sizeof(char), inputFile)) {
        char token;
        char name[MAX_INPUT_SIZE];

        semMech_wait(&semMechProcessInput);

        int numTokens = sscanf(line, "%c %s", &token, name);
    
        lineNumber++;

        /* perform minimal validation */
        if (numTokens < 1) {
            continue;
        }
        switch (token) {
            case 'c':
            case 'l':
            case 'd':
                if(numTokens != 2)
                    errorParse(lineNumber);
                if(insertCommand(line))
                    break;
                return NULL;
            case 'r':
                if(numTokens != 3)
                    errorParse(lineNumber);
                if(insertCommand(line))
                    break;
                return NULL;
            case '#':
                break;
            default: { /* error */
                errorParse(lineNumber);
            }
        }

        semMech_post(&semMechCommands);
    }

    fclose(inputFile);
    return NULL;
}

void *applyCommands() {
    mutex_init(&commandsLock);
    while(1) {
        semMech_wait(&semMechCommands);
        mutex_lock(&commandsLock);
        if(numberCommands > 0) {
            char token;
            char name[MAX_INPUT_SIZE];
            const char* command = removeCommand();
    
            if (command == NULL) {
                semMech_post(&semMechCommands);
                mutex_unlock(&commandsLock);
                continue;
            }

            sscanf(command, "%c %s", &token, name);
            if(token != 'c') {
                mutex_unlock(&commandsLock);
            }

            int searchResult;
            int iNumber;
            char *oldNodeName;
            char *newNodeName;
            switch (token) {
                case 'c':
                    iNumber = obtainNewInumber(fs);
                    mutex_unlock(&commandsLock);
                    create(fs, name, iNumber);
                    break;
                 case 'l':
                    searchResult = lookup(fs, name);
                    if(!searchResult)
                        printf("%s not found\n", name);
                    else
                        printf("%s found with inumber %d\n", name, searchResult);
                    break;
                case 'd':
                    delete(fs, name);
                    break;
                case 'r':
                    oldNodeName = strtok(name, " ");
                    newNodeName = strtok(NULL, " ");

                    if(oldNodeName != NULL && newNodeName != NULL) {
                        fs_rename(fs, oldNodeName, newNodeName);
                    }
                    break;
                default: { /* error */
                    fprintf(stderr, "Error: command to apply\n");
                    exit(EXIT_FAILURE);
                }
            }
        }
        else {
            mutex_unlock(&commandsLock);
            return NULL;
        }
        semMech_post(&semMechProcessInput);
    }
    mutex_destroy(&commandsLock);
}

void runThreads() {
    pthread_t producer;

    #if defined (RWLOCK) || defined (MUTEX)
        pthread_t *workers = (pthread_t*) malloc(numberThreads * sizeof(pthread_t));
       
        for(int i = 0; i < numberThreads; i++) {
            int err = pthread_create(&workers[i], NULL, applyCommands, NULL);
            if (err != 0){
                perror("Can't create worker thread\n");
                exit(EXIT_FAILURE);
            }
        }
    #else
        applyCommands();
    #endif

    if(pthread_create(&producer, NULL, processInput, NULL) != 0){
        perror("Can't create producer thread");
        exit(EXIT_FAILURE);
    }

    if(pthread_join(producer, NULL)) {
        perror("Can't join producer tread\n");
        exit(EXIT_FAILURE);
    }
    
    #if defined (RWLOCK) || defined (MUTEX)
        for(int i = 0; i < numberThreads; i++) {
            if(pthread_join(workers[i], NULL)) {
                perror("Can't join worker thread\n");
                exit(EXIT_FAILURE);
            }
        }
        free(workers);
    #endif
}


int main(int argc, char* argv[]) {
    TIMER_T startTime, endTime;
    FILE * outputFp;
    parseArgs(argc, argv);

    fs = new_tecnicofs(numberBuckets);

    semMech_init(&semMechCommands, 0);
    semMech_init(&semMechProcessInput, MAX_COMMANDS);

    TIMER_READ(startTime);
    runThreads();
    TIMER_READ(endTime);

    fprintf(stdout, "TecnicoFS completed in %.4f seconds.\n", 
    TIMER_DIFF_SECONDS(startTime, endTime));

    outputFp = openOutputFile();

    print_tecnicofs_tree(outputFp, fs);
    free_tecnicofs(fs);

    closeOutputFile(outputFp);

    exit(EXIT_SUCCESS);
}
