#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <stdint.h>
#include <pthread.h>
#include <unistd.h>
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
    // int fd;
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
            create(fs, arg1, iNumber);
            break;
        case 'd':
            delete(fs, arg1);
            break;
        case 'r':
            if(arg1 != NULL && arg2 != NULL) {
                swap_name(fs, arg1, arg2);
            }
            break;
        default: { /* error */
            fprintf(stderr, "Error: command to apply\n");
            exit(EXIT_FAILURE);
        }
    }
    //semMech_post(&semProducer);
}

 void *session(void *arg) {
    struct ucred ucred;
    char buffer[BUFFSIZE];
    char arg1[STRSIZE];
    char arg2[STRSIZE];
    char token;
    int error;
    int i;
    int sessionActive = 1;
    int socketFd = *((int*)arg);
    unsigned int socklen = sizeof(int);

    input_t *vector = malloc(sizeof(input_t) * 5); 

    for(int i = 0; i < 5; i++) {

        vector[i].flag=0;
    }
   
    if(getsockopt(socketFd, SOL_SOCKET, SO_PEERCRED, &ucred, &socklen) < 0) {
       perror("Error when calling getsockopt");
       exit(EXIT_FAILURE);
    }

    while(sessionActive) {   
        permission othersPerm;
        permission ownerPerm;
        uid_t owner;
        int status=0;
        int len;
        int fd;
        int mode;
        int counter = 0;
        int iNumber = -1;
        int iNumberNew = 0;

        if(read(socketFd, buffer, BUFFSIZE) > 0) {
            sscanf(buffer, "%c %s %s", &token, arg1, arg2);

            switch(token) {
                case 'c':
                    if(lookup(fs,arg1) != -1) {
                        error = TECNICOFS_ERROR_FILE_ALREADY_EXISTS;
                        write(socketFd, &error, sizeof(error));
                        break;
                    }
                    

                    if(strlen(arg2) != 2) {
                        error = TECNICOFS_ERROR_OTHER;
                        write(socketFd, &error, sizeof(error));
                        break;
                    }

                    ownerPerm = (int)arg2[0] - 48;
                    othersPerm = (int)arg2[1] - 48;
                    
                    iNumber = inode_create(ucred.uid, ownerPerm, othersPerm);

                    sprintf(buffer, "c %s %d", arg1, iNumber);

                    applyCommands(buffer);

                    write(socketFd, &status, sizeof(int));

                    break;
                case 'd':
                    iNumber= lookup(fs,arg1);

                    if(iNumber == -1) {
                        error= TECNICOFS_ERROR_FILE_NOT_FOUND;
                        write(socketFd, &error, sizeof(error));
                        break;
                    }

                    for(i=0;i<5;i++)
                        if(vector[i].iNumber == iNumber && vector[i].flag==1){
                            error= TECNICOFS_ERROR_FILE_IS_OPEN;
                            write(socketFd, &error, sizeof(error));
                            break;
                        }

                    if(inode_get(iNumber, &owner, NULL, NULL, NULL, 0) < 0){
                        perror("Error when calling inode_get");
                        break;
                    }

                    if(owner != ucred.uid) {
                        error = TECNICOFS_ERROR_PERMISSION_DENIED;
                        write(socketFd, &error, sizeof(error));
                        break;
                    }

                    sprintf(buffer, "d %s", arg1);

                    applyCommands(buffer);

                    write(socketFd, &status, sizeof(int));

                    break;
                case 'r':
                    iNumber=lookup(fs,arg1);
                    iNumberNew=lookup(fs,arg2);

                    if(iNumber == -1) {
                        error= TECNICOFS_ERROR_FILE_NOT_FOUND;
                        write(socketFd, &error, sizeof(error));
                        break;
                    }

                    if(iNumberNew != -1) {
                        error= TECNICOFS_ERROR_FILE_ALREADY_EXISTS;
                        write(socketFd, &error, sizeof(error));
                        break;
                    }

                    if(inode_get(iNumber, &owner, NULL, NULL, NULL, 0) < 0){
                        perror("Error when calling inode_get");
                        break;
                    }

                    if(owner != ucred.uid) {
                        error = TECNICOFS_ERROR_PERMISSION_DENIED;
                        write(socketFd, &error, sizeof(error));
                        break;
                    }

                    sprintf(buffer, "r %s %s", arg1, arg2);

                    applyCommands(buffer);

                    write(socketFd, &status, sizeof(int));

                    break;
                case 'o':
                    //counts the number of opened files
                    for(i = 0; i < 5; i++) {
                        if(vector[i].flag==1) {
                            counter++;
                        }
                    }
                    
                    if(counter == 5) {
                        error = TECNICOFS_ERROR_MAXED_OPEN_FILES;
                        write(socketFd, &error, sizeof(error));
                        break;
                    }

                    iNumber = lookup(fs,arg1);

                    if(iNumber == -1) {
                        error = TECNICOFS_ERROR_FILE_NOT_FOUND;
                        write(socketFd, &error, sizeof(error));
                        break;
                    }

                    if(inode_get(iNumber, &owner, &ownerPerm, &othersPerm, NULL, 0) < 0) {
                        error=TECNICOFS_ERROR_OTHER;
                        write(socketFd, &error, sizeof(error));
                        break;
                    }

                    mode =strtol(arg2,NULL,10);

                    if(owner != ucred.uid){
                        if(othersPerm != mode) {
                            error=mode;
                            error= TECNICOFS_ERROR_PERMISSION_DENIED;
                            write(socketFd, &error, sizeof(error));
                            break;
                        }
                    } else {
                            if(ownerPerm != mode){
                                error=mode;
                                error= TECNICOFS_ERROR_PERMISSION_DENIED;
                                write(socketFd, &error, sizeof(error));
                                break;
                            }
                        }

                    for(i = 0; i < 5; i++) {
                        if(vector[i].flag==0){
                            vector[i].mode=mode;
                            vector[i].iNumber=iNumber;
                            vector[i].flag=1;
                            break;
                        }
                    }

                    write(socketFd, &i, sizeof(int));
                    
                    break;

                case 'x':
                    fd = (int)strtol(arg1,NULL,10);

                    if( vector[fd].flag == 0) {
                        error= TECNICOFS_ERROR_FILE_NOT_OPEN;
                        write(socketFd, &error, sizeof(error));
                        break;
                    }

                    vector[fd].flag=0;

                    write(socketFd, &status, sizeof(int));

                    break;

                case 'l':
                    fd = (int)strtol(arg1,NULL,10);
                    len = (int)strtol(arg2,NULL,10);
                    char buffer[STRSIZE];

                    if(vector[fd].flag == 0 ) {
                        error= TECNICOFS_ERROR_FILE_NOT_OPEN;
                        write(socketFd, &error, sizeof(error));
                        break;
                    }

                    // verifies the mode client opened the file
                    if(vector[i].mode!=READ || vector[i].mode!=RW) {
                        error= TECNICOFS_ERROR_INVALID_MODE;
                        write(socketFd, &error, sizeof(error));
                        break;
                    }

                    iNumber = vector[fd].iNumber;

                    if(inode_get(iNumber, NULL, NULL, NULL,buffer, len) < 0){
                        perror("Error when calling inode_get");
                        break;
                    }

                    write(socketFd,buffer,len);

                    break;

                case 'w':
                    fd = (int)strtol(arg1,NULL,10);

                    if(vector[fd].flag == 0) {
                        error= TECNICOFS_ERROR_FILE_NOT_OPEN;
                        write(socketFd, &error, sizeof(error));
                        break;
                    }

                    // verifies which mode client has opened the file as                      
                    if(vector[i].mode != WRITE || vector[i].mode != RW) {
                        error= TECNICOFS_ERROR_INVALID_MODE;
                        write(socketFd, &error, sizeof(error));
                        break;
                    }

                    iNumber= vector[fd].iNumber;

                    inode_set(iNumber,arg2,strlen(arg2));
                    write(socketFd, &status, sizeof(int));
                    break;

                case '0':
                    sessionActive = 0;
                    break;
                default:
                    error= TECNICOFS_ERROR_INVALID_MODE;
                    write(socketFd, &error, sizeof(error));
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

        // close(newsockfd)

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

    //setSignal();
    inode_table_init();

    parseArgs(argc, argv);
    fs = new_tecnicofs(numberBuckets);

    mutex_init(&commandsLock);

    global_sockfd = serverSocketMount(server_addr, global_socketname, &server_len);

    TIMER_READ(startTime);
    acceptClients();
    TIMER_READ(endTime);

    if(serverSocketUnmount() != 0) {
        perror("Error when unmounting server socket");
    }
    
    fprintf(stdout, "TecnicoFS completed in %.4f seconds.\n", TIMER_DIFF_SECONDS(startTime, endTime));
    
    outputFp = openOutputFile();
    print_tecnicofs_tree(outputFp, fs);
    closeOutputFile(outputFp);

    mutex_destroy(&commandsLock);
    free_tecnicofs(fs);
    
    exit(EXIT_SUCCESS);
}
