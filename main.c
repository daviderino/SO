#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include "fs.h"

#define MAX_COMMANDS 150000
#define MAX_INPUT_SIZE 100

int numberThreads = 0;
tecnicofs* fs;
pthread_mutex_t lock;

int counter = 0;


char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
char *inputFile, *outputFile;

int numberCommands = 0;
int headQueue = 0;

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
    if((numberCommands + 1)){
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

void *applyCommands(){

    pthread_mutex_lock(&lock);

    /*counter += 1;
    printf("Starting process %d\n", counter);*/

    while(numberCommands > 0){
        const char* command = removeCommand();
        if (command == NULL){
            continue;
        }

        char token;
        char name[MAX_INPUT_SIZE];
        int numTokens = sscanf(command, "%c %s", &token, name);
        if (numTokens != 2) {
            fprintf(stderr, "Error: invalid command in Queue\n");
            exit(EXIT_FAILURE);
        }

        int searchResult;
        int iNumber;
        switch (token) {
            case 'c':
                printf("%s\n",name);
                iNumber = obtainNewInumber(fs);
                create(fs, name, iNumber);
                break;
            case 'l':
                printf("%s\n",name);
                searchResult = lookup(fs, name);
                if(!searchResult)
                    printf("%s not found\n", name);
                else
                    printf("%s found with inumber %d\n", name, searchResult);
                break;
            case 'd':
                printf("%s\n",name);
                delete(fs, name);
                break;
            default: { /* error */
                fprintf(stderr, "Error: command to apply\n");
                exit(EXIT_FAILURE);
            }
        }
    }
    /*printf("Ending process %d\n", counter);*/

    pthread_mutex_unlock(&lock);
    pthread_exit(NULL);
}


void createThreadPool() {
    pthread_t *threads;
    int i;
    
    threads = (pthread_t*)malloc(sizeof(pthread_t) * numberThreads);

     if (pthread_mutex_init(&lock, NULL) != 0){
        fprintf(stderr, "Error while creating a mutex\n");
        
     }

    for(i = 0; i < numberThreads; i++) {
        if(pthread_create(&threads[i], NULL, applyCommands, NULL) != 0) {
            fprintf(stderr, "Error while creating a thread\n");
        }
    }

    for(i = 0; i < numberThreads; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            fprintf(stderr, "Error while waiting for a thread\n");
        }
    }
}


int main(int argc, char* argv[]) {
    parseArgs(argc, argv);

    fs = new_tecnicofs();
    processInput();

    createThreadPool();
    print_tecnicofs_tree(outputFile, fs);

    free_tecnicofs(fs);
    exit(EXIT_SUCCESS);
}
