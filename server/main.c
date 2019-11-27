#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <pthread.h>
#include <math.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "constants.h"
#include "fs.h"
#include "sem.h"
#include "sock.h"
#include "sync.h"
#include "lib/timer.h"
#include "lib/inodes.h"

typedef struct input_t {
    int iNumber;
    int fd;
    int mode;
    int flag;   //file opened:flag=1
} input_t;


tecnicofs* fs;
pthread_mutex_t commandsLock;

struct sockaddr_un server_addr;

int global_sockfd;
int canAccept = 1;

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
    if (argc != 4) {
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

void applyCommands(char *args) {    
    char token;
    char name[MAX_INPUT_SIZE];
    char newName[MAX_INPUT_SIZE];
    char arg1[STRSIZE];
    char arg2[STRSIZE];
    
//    semMech_wait(&semConsumer);
    mutex_lock(&commandsLock);
    sscanf(args, "%c %s %s", &token, arg1, arg2);
    
    if(token != 'c') {
        mutex_unlock(&commandsLock);
    }

    int iNumber;
    switch (token) {
        case 'c':
            iNumber = strtol(arg2, NULL, 10);
            mutex_unlock(&commandsLock);
            create(fs, name, iNumber);
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
            exit(EXIT_FAILURE);
        }
    }
    //semMech_post(&semProducer);
}

void *session( void * arg) {
    struct ucred ucred;
    char buffer[BUFFSIZE];
    char arg1[STRSIZE];
    char arg2[STRSIZE];
    char token;
    int sessionActive = 1;
    input_t *vector = malloc(sizeof(input_t) * 5); 

    int socketFd = *((int*)arg);

    for(int i = 0; i < 5; i++) {
        vector[i].flag=0;
    }

    while(sessionActive) {   
        permission othersPerm;
        permission ownerPerm;
        uid_t owner;
        int len;
        int fd;
        int error;
        int mode;
        int counter = 0;
        int iNumber = 0;
        int iNumberNew = 0;
        unsigned int socklen = sizeof(int);

        if(read(socketFd, buffer, BUFFSIZE) > 0) {
            sscanf(buffer, "%c %s %s", &token, arg1, arg2);

            switch(token) {
                case 'c':
                    if(lookup(fs,arg1)) {
                        error = TECNICOFS_ERROR_FILE_ALREADY_EXISTS;
                        write(socketFd, (void *)&error, sizeof(error));
                        break;
                    }
                    
                    if(getsockopt(socketFd, SOL_SOCKET, SO_PEERCRED, &ucred, &socklen) < 0) {
                        error = TECNICOFS_ERROR_OTHER;
                        write(socketFd, (void *)&error, sizeof(error));
                        break;
                    }

                    if(strlen(arg2) != 2) {
                        error = TECNICOFS_ERROR_OTHER;
                        write(socketFd, (void *)&error, sizeof(error));
                        break;
                    }

                    ownerPerm = (int)arg2[0] - 48;
                    othersPerm = (int)arg2[1] - 48;
                    
                    iNumber = inode_create(ucred.uid, ownerPerm, othersPerm);

                    sprintf(buffer, "c %s %d", arg1, iNumber);

                    applyCommands(buffer);

                    break;
                case 'd':
                    iNumber= lookup(fs,arg1);

                    if(!iNumber) {
                        error= TECNICOFS_ERROR_FILE_NOT_FOUND;
                        write(socketFd, (void*)&error, sizeof(error));
                        break;
                    }

                    if(inode_get(iNumber, &owner, NULL, NULL, NULL, 0) < 0){
                        perror("Error when calling inode_get");
                        break;
                    }

                    if(getsockopt(socketFd, SOL_SOCKET, SO_PEERCRED, &ucred, NULL) < 0) {
                        perror("Error when calling getsockopt");
                    }

                    if(owner != ucred.uid) {
                        error = TECNICOFS_ERROR_PERMISSION_DENIED;
                        write(socketFd, (void*)&error, sizeof(error));
                        break;
                    }

                    sprintf(buffer, "d %s", arg1);

                    applyCommands(buffer);

                    break;
                case 'r':
                    iNumber=lookup(fs,arg1);
                    iNumberNew=lookup(fs,arg2);

                    if(!iNumber) {
                        error= TECNICOFS_ERROR_FILE_NOT_FOUND;
                        write(socketFd, (void*)&error, sizeof(error));
                        break;
                    }

                    if(iNumberNew) {
                        error= TECNICOFS_ERROR_FILE_ALREADY_EXISTS;
                        write(socketFd, (void *)&error, sizeof(error));
                        break;
                    }

                    if(inode_get(iNumber, &owner, NULL, NULL, NULL, 0) < 0){
                        perror("Error when calling inode_get");
                        break;
                    }

                    if(getsockopt(socketFd, SOL_SOCKET, SO_PEERCRED, &ucred, NULL) < 0) {
                        perror("Error when calling getsockopt");
                        break;
                    }

                    if(owner != ucred.uid) {
                        error = TECNICOFS_ERROR_PERMISSION_DENIED;
                        write(socketFd, (void*)&error, sizeof(error));
                        break;
                    }

                    sprintf(buffer, "r %s %s", arg1, arg2);

                    applyCommands(buffer);

                    break;
                case 'o':
                    //counts the number of opened files
                    for(int i = 0; i < 5; i++) {
                        if(vector[i].flag == 1) {
                            counter++;
                        }
                    }

                    if(counter == 5) {
                        error = TECNICOFS_ERROR_MAXED_OPEN_FILES;
                        write(socketFd, (void*)&error, sizeof(error));
                        break;
                    }

                    iNumber = lookup(fs,arg1);

                    if(!iNumber) {
                        error = TECNICOFS_ERROR_FILE_NOT_FOUND;
                        write(socketFd, (void *)&error, sizeof(error));
                        break;
                    }


                    if(inode_get(iNumber, NULL, NULL, &othersPerm, NULL, 0) < 0) {
                        perror("Error when calling inode_get");
                        break;
                    }

                    mode = (int)strtol(arg2,NULL,10);

                    if(othersPerm != mode) {
                        error= TECNICOFS_ERROR_PERMISSION_DENIED;
                        write(socketFd, (void *)&error, sizeof(error));
                        break;
                    }

                    fd = open(arg1,mode);

                    for(int i = 0; i < 5; i++) {
                        if(vector[i].flag==0){
                            vector[i].flag=1;
                            vector[i].mode=mode;
                            vector[i].fd=fd;
                            vector[i].iNumber=iNumber;
                            break;
                        }
                    }

                    break;

                case 'x':
                    fd = (int)strtol(arg1,NULL,10);

                    for(int i=0;i<5;i++) {
                        if (vector[i].fd==fd) {
                            iNumber=vector[i].iNumber;
                            break;
                        }
                    }

                    if(!iNumber) {
                        error= TECNICOFS_ERROR_FILE_NOT_OPEN;
                        write(socketFd, (void *)&error, sizeof(error));
                        break;
                    }

                    for(int i=0;i<5;i++) {
                        if (vector[i].fd==fd) {
                            vector[i].flag=0;
                        }
                    }

                    close(fd);
                    break;

                case 'l':
                    fd = (int)strtol(arg1,NULL,10);
                    len = (int)strtol(arg2,NULL,10);
                    char buffer[STRSIZE];

                    for(int i=0;i<5;i++) {
                        if (vector[i].fd==fd) {
                            iNumber=vector[i].iNumber;

                            //verifies the mode client opened the file
                            if(vector[i].mode!=READ || vector[i].mode!=RW) {
                                error= TECNICOFS_ERROR_INVALID_MODE;
                                write(socketFd, (void *)&error, sizeof(error));
                                break;
                            }
                            break;
                        }
                    }

                    if(!iNumber) {
                        error= TECNICOFS_ERROR_FILE_NOT_OPEN;
                        write(socketFd, (void *)&error, sizeof(error));
                        break;
                    }

                    if(inode_get(iNumber, NULL, NULL, NULL,buffer, len) < 0){
                        perror("Error when calling inode_get");
                        break;
                    }

                    write(socketFd,buffer,len);
                    break;

                case 'w':
                    iNumber = 0;
                    fd = (int)strtol(arg1,NULL,10);

                    for(int i=0;i<5;i++) {
                       if (vector[i].fd == fd){
                           iNumber = vector[i].iNumber;  

                           //verifies the mode client opened the file                       
                           if(vector[i].mode!=WRITE || vector[i].mode!=RW) {
                               error= TECNICOFS_ERROR_INVALID_MODE;
                               write(socketFd, (void *)&error, sizeof(error));
                               break;
                           }
                           break;
                       }
                    }


                    if(!iNumber) {
                        error= TECNICOFS_ERROR_FILE_NOT_OPEN;
                        write(socketFd, (void *)&error, sizeof(error));
                        break;
                    }

                    inode_set(iNumber,arg2,strlen(arg2));
                    break;
                case '0':
                    sessionActive = 0;
                    break;
            }
        }
    }

    close(socketFd);
    return NULL;
}

void acceptClients() {
    unsigned int client_dimension;
    struct sockaddr_un client_addr;
    int i = 0;
    int num_threads = 4;
    pthread_t *slaves = (pthread_t*) malloc(num_threads * sizeof(pthread_t));

    while(canAccept) {
        client_dimension = sizeof(client_addr);
        int newsockfd = accept(global_sockfd, &client_addr, &client_dimension);
        
        if(pthread_create(&slaves[i++], NULL, session, &newsockfd) != 0) {
            perror("Error when creating slave thread");
            exit(EXIT_FAILURE);
        }

        if(i == num_threads) {
            num_threads = num_threads + 4;
            slaves = realloc(slaves, num_threads * sizeof(pthread_t));
        }
    }

    for(int c = 0; c <= i; c++) {
        if(pthread_join(slaves[c], NULL) != 0) {
            perror("Error joining slave thread");
            exit(EXIT_FAILURE);
        }
    }
}

void handle_sigint() {
    signal(SIGINT, handle_sigint);
    canAccept = 0;
}

void setSignal() {
    if (signal(SIGINT, handle_sigint) ) {
        perror("Error while setting SIGINT\n");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char* argv[]) {
    TIMER_T startTime, endTime;
    FILE * outputFp;
    int server_len;

    setSignal();

    parseArgs(argc, argv);
    fs = new_tecnicofs(numberBuckets);

    //semMech_init(&semProducer, MAX_COMMANDS);
    //semMech_init(&semConsumer, 0);
    mutex_init(&commandsLock);

    global_sockfd = serverSocketMount(server_addr, global_socketname, &server_len);

    TIMER_READ(startTime);
    acceptClients();
    TIMER_READ(endTime);
    
    fprintf(stdout, "TecnicoFS completed in %.4f seconds.\n",
    TIMER_DIFF_SECONDS(startTime, endTime));
    
    outputFp = openOutputFile();
    print_tecnicofs_tree(outputFp, fs);

    mutex_destroy(&commandsLock);
    //semMech_destroy(&semProducer);
    //semMech_destroy(&semConsumer);
    
    free_tecnicofs(fs);
    closeOutputFile(outputFp);
    
    exit(EXIT_SUCCESS);
}
