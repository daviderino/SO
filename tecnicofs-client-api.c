#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "tecnicofs-api-constants.h"
#include "lib/inodes.h"

#define BUFFSIZE 512

int sockfd = -1;

int tfsMount(char *address) {
    int server_len;
    struct sockaddr_un server_addr;
    
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

    if(close(sockfd) != 0) {
        return TECNICOFS_ERROR_NO_OPEN_SESSION;   
    }

    return 0;
}

int tfsCreate(char *filename, int ownerPermissions, int othersPermissions) {
    char buffer[BUFFSIZE];

    sprintf(buffer, "c %s %d%d", filename, ownerPermissions, othersPermissions);

    if(write(sockfd, buffer, BUFFSIZE) < 0) {
        return TECNICOFS_ERROR_OTHER;
    }

    if(read(sockfd, buffer, BUFFSIZE) < 0) {
        return TECNICOFS_ERROR_OTHER;
    }

    if(!strcmp(buffer, "FILE_EXISTS")) {
        return TECNICOFS_ERROR_FILE_ALREADY_EXISTS;
    }


    return 0;
}

int tfsOpen (char *filename, int mode){
    char buffer[BUFFSIZE];

    sprintf(buffer, "o %s %d", filename, mode);

     if(write(sockfd, buffer, BUFFSIZE) < 0) {
        return TECNICOFS_ERROR_OTHER;
    }

     if(read(sockfd, buffer, BUFFSIZE) < 0) {
        return TECNICOFS_ERROR_OTHER;
    }

    if(!strcmp(buffer, "PERMISSION_DENIED")) {
        return TECNICOFS_ERROR_PERMISSION_DENIED;
    }

    if(!strcmp(buffer, "FILE_DOESNT_EXISTS")) {
        return TECNICOFS_ERROR_FILE_NOT_FOUND;
    }

    if(!strcmp(buffer, "MAX_FILES_OPEN")) {
        return TECNICOFS_ERROR_MAXED_OPEN_FILES;
    }

    return (int)strtol(buffer,NULL,10);

}

int tfsRead (int fd, char *buffer, int len){
    char buff[BUFFSIZE];

    sprintf(buff, "l %d %d", fd, len);

     if(write(sockfd, buff,BUFFSIZE) < 0) {
        return TECNICOFS_ERROR_OTHER;
    }

    if(read(sockfd, buffer, len) < 0) {
        return TECNICOFS_ERROR_OTHER;
    }

    return strlen(buffer);

}

int tfsClose(int fd){
    char buffer[BUFFSIZE];

    sprintf(buffer, "c %d", fd);

    if(write(sockfd, buffer, BUFFSIZE) < 0) {
        return TECNICOFS_ERROR_OTHER;
    }

    if(read(sockfd, buffer, BUFFSIZE) < 0) {
        return TECNICOFS_ERROR_OTHER;
    }

    if(!strcmp(buffer, "FILE_NOT_OPEN")) {
        return TECNICOFS_ERROR_FILE_NOT_OPEN;
    }

    return 0;

}