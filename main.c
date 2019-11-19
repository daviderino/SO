#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <pthread.h>
#include <math.h>
#include "constants.h"
#include "fs.h"
#include "sem.h"
#include "sock.h"
#include "sync.h"
#include "lib/timer.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

tecnicofs* fs;
pthread_mutex_t commandsLock;

sem_t semProducer;
sem_t semConsumer;

char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
char *global_socketname = NULL;
char *global_outputfile = NULL;

int numberCommands = 0;
int headQueue = 0;

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
    if(fflush(outputFp) != 0) {
        perror("Error flushing output file");
    }
    if(fclose(outputFp) != 0) {
        perror("Error closing output file");
    }
}

static void displayUsage (const char* appName){
    printf("Usage: %s\n", appName);
    exit(EXIT_FAILURE);
}

static void parseArgs (long argc, char* const argv[]){
    if (argc != 3) {
        fprintf(stderr, "Invalid format:\n");
        displayUsage("./tecnicofs nomesocket outputfile numbuckets");
    }

    global_socketname = argv[1];
    global_outputfile = argv[2];
    numberBuckets = (int)strtol(argv[3], NULL, 10);

    if(numberBuckets <= 0) {
        fprintf(stderr, "Invalid buckets number\n");
        exit(EXIT_FAILURE);
    }
}

int insertCommand(char* data) {
    static int commandsPtr = 0;
    if(numberCommands != MAX_COMMANDS) {
        strcpy(inputCommands[commandsPtr], data);
        commandsPtr = (commandsPtr + 1) % MAX_COMMANDS;
        numberCommands++;
        return 1;
    }
    return 0;
}

char* removeCommand() {
    if((numberCommands > 0)){
        char *ret = inputCommands[headQueue];

        if(strcmp(ret, END_OF_COMMANDS)) {
            numberCommands--;
            headQueue = (headQueue + 1) % MAX_COMMANDS;

        }
        return ret;
    }
    return NULL;
}

void errorParse(int lineNumber){
    fprintf(stderr, "Error: line %d invalid\n", lineNumber);
    exit(EXIT_FAILURE);
}

void *applyCommands() {    
    while(1) {        
        char token;
        char name[MAX_INPUT_SIZE];
        char newName[MAX_INPUT_SIZE];
        
        semMech_wait(&semConsumer);
        mutex_lock(&commandsLock);
        
        const char* command = removeCommand();

        if (command == NULL) {
            mutex_unlock(&commandsLock);
            semMech_post(&semProducer);
            continue;
        }
        
        if(!strcmp(command, END_OF_COMMANDS)) {
            mutex_unlock(&commandsLock);
            semMech_post(&semConsumer);
            break;
        }

        sscanf(command, "%c %s %s", &token, name, newName);

        if(token != 'c') {
            mutex_unlock(&commandsLock);
        }

        int searchResult;
        int iNumber;
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
                if(name != NULL && newName != NULL) {
                    swap_name(fs, name, newName);
                }
                break;
            default: { /* error */
                fprintf(stderr, "Error: command to apply\n");
                semMech_post(&semProducer);
                exit(EXIT_FAILURE);
            }
        }
        semMech_post(&semProducer);
    }
    return NULL;
}

void acceptClients() {
    while(1) {
        
    }
}


int main(int argc, char* argv[]) {
    struct sockaddr_un serv_addr;

    TIMER_T startTime, endTime;
    FILE * outputFp;

    parseArgs(argc, argv);
    fs = new_tecnicofs(numberBuckets);

    semMech_init(&semProducer, MAX_COMMANDS);
    semMech_init(&semConsumer, 0);
    mutex_init(&commandsLock);

    int sockfd = socketCreateStream();
    int len_serv = socketNameStream(serv_addr, global_socketname);
    socketBind(sockfd, serv_addr, len_serv);
    socketListen(sockfd, MAX_CONNECTIONS);

    TIMER_READ(startTime);
    acceptClients();
    TIMER_READ(endTime);
    
    fprintf(stdout, "TecnicoFS completed in %.4f seconds.\n",
    TIMER_DIFF_SECONDS(startTime, endTime));
    
    outputFp = openOutputFile();
    print_tecnicofs_tree(outputFp, fs);

    mutex_destroy(&commandsLock);
    semMech_destroy(&semProducer);
    semMech_destroy(&semConsumer);
    
    free_tecnicofs(fs);
    closeOutputFile(outputFp);
    
    exit(EXIT_SUCCESS);
}
