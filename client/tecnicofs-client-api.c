#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "tecnicofs-api-constants.h"

#define BUFFSIZE 512

int sockfd = -1;

int tfsMount(char *address) {
    int server_len;
    struct sockaddr_un server_addr;

    if(strlen(address) == 0) {
        printf("Usage: ./program socket_name");
        exit(EXIT_FAILURE);
    }
    
    if(sockfd != -1) {
        return TECNICOFS_ERROR_OPEN_SESSION;
    }

    if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0) ) < 0) {
        return TECNICOFS_ERROR_OTHER;
    }


    bzero((char*) &server_addr, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strcpy(server_addr.sun_path, address);
    server_len = strlen(server_addr.sun_path) + sizeof(server_addr.sun_family);

    if(connect(sockfd, (struct sockaddr*) &server_addr, server_len) < 0) {
        return TECNICOFS_ERROR_CONNECTION_ERROR;
    }

    return 0;
}

int tfsUnmount() {
    if(sockfd == -1) {
        return TECNICOFS_ERROR_NO_OPEN_SESSION;   
    }
    
    if(write(sockfd, "0", sizeof(char) * 2) < 0) {
        return TECNICOFS_ERROR_OTHER;           
    }

    if(close(sockfd) != 0) {
        return TECNICOFS_ERROR_OTHER;   
    }

    sockfd = -1;

    return 0;
}

int tfsCreate(char *filename, int ownerPermissions, int othersPermissions) {
    char buffer[BUFFSIZE] = {'\0'};
    int ret;

    sprintf(buffer, "c %s %d%d", filename, ownerPermissions, othersPermissions);

    if(write(sockfd, buffer, sizeof(buffer)) < 0) {
        return TECNICOFS_ERROR_OTHER;
    }

    if(read(sockfd, &ret, sizeof(int)) < 0) {
        return TECNICOFS_ERROR_OTHER;
    }
    
    return ret;
}

int tfsDelete(char *filename) {
    char buffer[BUFFSIZE] = {'\0'};
    int ret;

    sprintf(buffer, "d %s", filename);

    if(write(sockfd, buffer, sizeof(buffer)) < 0) {
        return TECNICOFS_ERROR_OTHER;
    }

    if(read(sockfd, &ret, sizeof(int)) < 0) {
        return TECNICOFS_ERROR_OTHER;
    }

    return ret;
}

int tfsRename(char *filenameOld, char *filenameNew) {
    char buffer[BUFFSIZE] = {'\0'};
    int ret;

    sprintf(buffer, "r %s %s", filenameOld, filenameNew);

    if(write(sockfd, buffer, sizeof(buffer)) < 0) {
        return TECNICOFS_ERROR_OTHER;
    }

    if(read(sockfd, &ret, sizeof(int)) < 0) {
        return TECNICOFS_ERROR_OTHER;
    }

    return ret;
}

int tfsOpen(char *filename, int mode){
    char buffer[BUFFSIZE] = {'\0'};
    int ret;

    sprintf(buffer, "o %s %d", filename, mode);

    if(write(sockfd, buffer, BUFFSIZE) < 0) {
        return TECNICOFS_ERROR_OTHER;
    }

    if(read(sockfd, &ret, sizeof(int)) < 0) {
        return TECNICOFS_ERROR_OTHER;
    }

    return ret;
}

int tfsClose(int fd){
    char buffer[BUFFSIZE] = {'\0'};
    int ret;

    sprintf(buffer, "x %d", fd);

    if(write(sockfd, buffer, sizeof(buffer)) < 0) {
        return TECNICOFS_ERROR_OTHER;
    }

    if(read(sockfd, &ret, sizeof(int)) < 0) {
        return TECNICOFS_ERROR_OTHER;
    }

    return ret;
}

int tfsRead(int fd, char *buffer, int len){
    char msg[BUFFSIZE] = {'\0'};
    int n;
    int ret;

    sprintf(msg, "l %d %d", fd, len);

    if(write(sockfd, msg, sizeof(msg)) < 0) {
        return TECNICOFS_ERROR_OTHER;
    }

    if(read(sockfd, &ret, sizeof(int)) < 0) {
        return TECNICOFS_ERROR_OTHER;
    }

    if(ret != 0){
        return ret;
    }

    if((n = read(sockfd, buffer, len)) < 0) {
        return TECNICOFS_ERROR_OTHER;
    }  

    return n - 1;
}

int tfsWrite(int fd, char *buffer, int len) {
    char msg[BUFFSIZE] = {'\0'};
    int ret;
    int n = strlen(buffer);

    if(n < len) {
        len = n;
    }

    sprintf(msg, "w %d %s", fd, buffer);
    msg[len + 4] = '\0';

    if(write(sockfd, msg, sizeof(char) * (len + 5)) < 0) {
        return TECNICOFS_ERROR_OTHER;
    }

    if(read(sockfd, &ret, sizeof(int)) < 0) {
        return TECNICOFS_ERROR_OTHER;
    }

    return ret;
}
