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

int tfsDelete(char *filename) {
    char buffer[BUFFSIZE];

    sprintf(buffer, "d %s", filename);

    if(write(sockfd, buffer, BUFFSIZE) < 0) {
        return TECNICOFS_ERROR_OTHER;
    }

    if(read(sockfd, buffer, BUFFSIZE) < 0) {
        return TECNICOFS_ERROR_OTHER;
    }

    if(!strcmp(buffer, "WRONG_UID")) {
        return TECNICOFS_ERROR_PERMISSION_DENIED;
    }

    return 0;
}

int tfsRename(char *filenameOld, char *filenameNew) {
    char buffer[BUFFSIZE];

    sprintf(buffer, "r %s %s", filenameOld, filenameNew);

    if(write(sockfd, buffer, BUFFSIZE) < 0) {
        return TECNICOFS_ERROR_OTHER;
    }

    if(read(sockfd, buffer, BUFFSIZE) < 0) {
        return TECNICOFS_ERROR_OTHER;
    }

    if(!strcmp(buffer, "OLD_NAME_WRONG")) {
        return TECNICOFS_ERROR_FILE_NOT_FOUND;
    }

    if(!strcmp(buffer, "NEW_NAME_WRONG")) {
        return TECNICOFS_ERROR_FILE_ALREADY_EXISTS;
    }

    if(!strcmp(buffer, "WRONG_UID")) {
        return TECNICOFS_ERROR_PERMISSION_DENIED;
    }

    return 0;
}

int tfsWrite(int fd, char *buffer, int len) {
    char buff[BUFFSIZE];

    sprintf(buff, "w %s %d", fd, buffer);

    if(write(sockfd, buffer, BUFFSIZE) < 0) {
        return TECNICOFS_ERROR_OTHER;
    }

    if(read(sockfd, buffer, BUFFSIZE) < 0) {
        return TECNICOFS_ERROR_OTHER;
    }

    if(!strcmp(buff, "FILE_NOT_OPEN")) {
        return TECNICOFS_ERROR_FILE_NOT_OPEN;
    }

    if(!strcmp(buff, "WRONG_MODE")) {
        return TECNICOFS_ERROR_INVALID_MODE;
    }

    return 0;
}