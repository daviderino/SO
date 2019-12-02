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
    int mode;
    int flag;   //file opened:flag=1
} input_t;


tecnicofs* fs;
int global_sockfd;
sem_t semProducer;
sem_t semConsumer;
pthread_mutex_t commandsLock;
struct sockaddr_un server_addr;

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
}

void writeIntValidate(int fd, int *val) {
    if(write(fd, val, sizeof(int)) < 0) {
        perror("Error when callign write()");
        exit(EXIT_FAILURE);
    }
}

 void *session(void *arg) {
    char token;
    char msg[BUFFSIZE] = {'\0'};
    char arg1[STRSIZE] = {'\0'};
    char arg2[STRSIZE] = {'\0'};
    struct ucred ucred;
    int error;
    int sessionActive = 1;
    int socketFd = *((int*)arg);
    unsigned int socklen = sizeof(struct ucred);

    input_t vector[5];

    int i;
    for(int i = 0; i < 5; i++) {
        vector[i].flag = 0;
    }

    if(getsockopt(socketFd, SOL_SOCKET, SO_PEERCRED, &ucred, &socklen) < 0) {
        perror("Error when calling getsockopt");
        exit(EXIT_FAILURE);
    }

    while(sessionActive) {   
        permission othersPerm;
        permission ownerPerm;
        uid_t owner;
        int len = -1;
        int fd = -1;
        int mode = -1;
        int status = 0;
        int counter = 0;
        int iNumber = -1;
        int iNumberNew = 0;
        char buffer[STRSIZE] = {0};

        int n = read(socketFd, msg, BUFFSIZE);

        if(n > 0) {
            sscanf(msg, "%c %s %s", &token, arg1, arg2);
            switch(token) {
                case 'c':
                    if(lookup(fs,arg1) != -1) {
                        error = TECNICOFS_ERROR_FILE_ALREADY_EXISTS;
                        
                        
                        writeIntValidate(socketFd, &error);
                        break;
                    }

                    if(strlen(arg2) != 2) {
                        error = TECNICOFS_ERROR_OTHER;
                        writeIntValidate(socketFd, &error);
                        break;
                    }

                    ownerPerm = (int)arg2[0] - 48;
                    othersPerm = (int)arg2[1] - 48;
                    
                    iNumber = inode_create(ucred.uid, ownerPerm, othersPerm);

                    sprintf(msg, "c %s %d", arg1, iNumber);

                    applyCommands(msg);
                    writeIntValidate(socketFd, &status);

                    break;
                case 'd':
                    iNumber= lookup(fs,arg1);

                    if(iNumber == -1) {
                        error= TECNICOFS_ERROR_FILE_NOT_FOUND;
                        writeIntValidate(socketFd, &error);
                        break;
                    }

                    if(inode_get(iNumber, &owner, NULL, NULL, NULL, 0) < 0) {
                        error = TECNICOFS_ERROR_OTHER;
                        writeIntValidate(socketFd, &error);
                        break;
                    }

                    if(owner != ucred.uid) {
                        error = TECNICOFS_ERROR_PERMISSION_DENIED;
                        writeIntValidate(socketFd, &error);
                        break;
                    }

                    sprintf(msg, "d %s", arg1);
                    applyCommands(msg);

                    writeIntValidate(socketFd, &status);

                    break;
                case 'r':
                    iNumber = lookup(fs,arg1);
                    iNumberNew = lookup(fs,arg2);

                    if(iNumber == -1) {
                        error = TECNICOFS_ERROR_FILE_NOT_FOUND;
                        writeIntValidate(socketFd, &error);
                        break;
                    }

                    if(iNumberNew != -1) {
                        error = TECNICOFS_ERROR_FILE_ALREADY_EXISTS;
                        writeIntValidate(socketFd, &error);
                        break;
                    }

                    if(inode_get(iNumber, &owner, NULL, NULL, NULL, 0) < 0) {
                        error = TECNICOFS_ERROR_OTHER;
                        writeIntValidate(socketFd, &error);
                        break;
                    }

                    if(owner != ucred.uid) {
                        error = TECNICOFS_ERROR_PERMISSION_DENIED;
                        writeIntValidate(socketFd, &error);
                        break;
                    }

                    sprintf(msg, "r %s %s", arg1, arg2);
                    applyCommands(msg);

                    writeIntValidate(socketFd, &status);

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
                        writeIntValidate(socketFd, &error);
                        break;
                    }

                    iNumber = lookup(fs,arg1);

                    if(iNumber == -1) {
                        error = TECNICOFS_ERROR_FILE_NOT_FOUND;
                        writeIntValidate(socketFd, &error);
                        break;
                    }

                    if(inode_get(iNumber, &owner, &ownerPerm, &othersPerm, NULL, 0) < 0) {
                        error = TECNICOFS_ERROR_OTHER;
                        writeIntValidate(socketFd, &error);
                        break;
                    }

                    mode = strtol(arg2, NULL, 10);

                    if(owner == ucred.uid) {
                       if(ownerPerm != mode && ownerPerm != RW) {
                            error = TECNICOFS_ERROR_PERMISSION_DENIED;
                            writeIntValidate(socketFd, &error);
                            break;
                        }
                    }
                    else {
                         if(othersPerm != mode && othersPerm != RW) {
                            error = TECNICOFS_ERROR_PERMISSION_DENIED;
                            writeIntValidate(socketFd, &error);
                            break;
                        }
                    }

                    for(i = 0; i < 5; i++) {
                        if(vector[i].flag == 0) {
                            vector[i].mode = mode;
                            vector[i].iNumber = iNumber;
                            vector[i].flag = 1;
                            break;
                        }
                    }

                    write(socketFd, &i, sizeof(i));
                    
                    break;

                case 'x':
                    fd = (int)strtol(arg1, NULL, 10);

                    if(vector[fd].flag == 0) {
                        error= TECNICOFS_ERROR_FILE_NOT_OPEN;
                        writeIntValidate(socketFd, &error);
                        break;
                    }

                    vector[fd].flag = 0;

                    writeIntValidate(socketFd, &status);

                    break;

                case 'l':
                    fd = (int)strtol(arg1, NULL, 10);
                    len = (int)strtol(arg2, NULL, 10);

                    if(vector[fd].flag == 0) {
                        error= TECNICOFS_ERROR_FILE_NOT_OPEN;
                        writeIntValidate(socketFd, &error);
                        break;
                    }

                    // verifies the mode client opened the file
                    if(vector[fd].mode != READ && vector[fd].mode != RW) {
                        error = TECNICOFS_ERROR_INVALID_MODE;
                        writeIntValidate(socketFd, &error);
                        break;
                    }

                    iNumber = vector[fd].iNumber;

                    int n;

                    if((n = inode_get(iNumber, NULL, NULL, NULL, buffer, len - 1)) < 0) {
                        error = TECNICOFS_ERROR_OTHER;
                        writeIntValidate(socketFd, &error);
                        break;
                    }

                    writeIntValidate(socketFd, &status);

                    if(write(socketFd, buffer, n + 1) < 0) {
                        perror("Error when calling write()");
                        exit(EXIT_FAILURE);
                    };

                    break;
                case 'w':
                    fd = (int)strtol(arg1, NULL, 10);

                    if(vector[fd].flag == 0) {
                        error = TECNICOFS_ERROR_FILE_NOT_OPEN;
                        writeIntValidate(socketFd, &error);
                        break;
                    }

                    // verifies which mode client has opened the file as                      
                    if(vector[fd].mode != WRITE && vector[fd].mode != RW) {
                        error= TECNICOFS_ERROR_INVALID_MODE;
                        writeIntValidate(socketFd, &error);
                        break;
                    }

                    iNumber = vector[fd].iNumber;
                    inode_set(iNumber, arg2, strlen(arg2));

                    writeIntValidate(socketFd, &status);
                    break;
                case '0':
                    sessionActive = 0;
                    break;
                default:
                    error = TECNICOFS_ERROR_INVALID_MODE;
                    writeIntValidate(socketFd, &error);
                    break;
            }
        }
        else if (n == 0) {
            sessionActive = 0;
        }
        else {
            perror("Error when calling read()");
            exit(EXIT_FAILURE);
        }
    }

    if(close(socketFd) < 0) {
        perror("Error when closing socket fd");
        exit(EXIT_FAILURE);
    }

    return NULL;
}

void acceptClients() {
    int c;
    int fd[100];
    unsigned int client_dimension;
    struct sockaddr_un client_addr;
    int i = 0;
    int num_threads = 4;
    pthread_t *slaves = (pthread_t*) malloc(num_threads * sizeof(pthread_t));

    while(1) { 
        client_dimension = sizeof(client_addr);

        int newsockfd = accept(global_sockfd, &client_addr, &client_dimension);
        fd[i] = newsockfd;

        if(fd[i] == -1) {
            i--;
            break;
        }

        sigset_t set;
        sigemptyset(&set);
        sigaddset(&set, SIGINT);

        if(pthread_sigmask(SIG_BLOCK, &set, NULL) != 0) {
            perror("Error when setting sigmask to slave thread");
            exit(EXIT_FAILURE);
        }

        if(pthread_create(&slaves[i++], NULL, session, &newsockfd) != 0) {
            perror("Error when creating slave thread");
            exit(EXIT_FAILURE);
        }

        pthread_sigmask(SIG_UNBLOCK, &set, NULL);

        if(i == num_threads) {
            num_threads = num_threads + 4;
            slaves = realloc(slaves, num_threads * sizeof(pthread_t));
        }
    }

    for(c = 0; c <= i; c++) {
        if(pthread_join(slaves[c], NULL) != 0) {
            perror("Error joining slave thread");
            exit(EXIT_FAILURE);
        }
    }

    free(slaves);
}

void handle_sigint() {
    serverSocketUnmount();
}

void setSignal() {
    signal(SIGINT, handle_sigint);
}

int main(int argc, char* argv[]) {
    TIMER_T startTime, endTime;
    FILE * outputFp;
    int server_len;

    setSignal();
    inode_table_init();

    parseArgs(argc, argv);
    fs = new_tecnicofs(numberBuckets);

    mutex_init(&commandsLock);

    global_sockfd = serverSocketMount(server_addr, global_socketname, &server_len);

    TIMER_READ(startTime);
    acceptClients();
    TIMER_READ(endTime);
    
    fprintf(stdout, "TecnicoFS completed in %.4f seconds.\n", TIMER_DIFF_SECONDS(startTime, endTime));
    
    outputFp = openOutputFile();
    print_tecnicofs_tree(outputFp, fs);
    closeOutputFile(outputFp);

    mutex_destroy(&commandsLock);
    free_tecnicofs(fs);
    inode_table_destroy();
    exit(EXIT_SUCCESS);
}
