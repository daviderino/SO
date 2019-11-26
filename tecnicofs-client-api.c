#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "tecnicofs-api-constants.h"
#include "lib/inodes.h"

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

int tfsOpen(char *filename, int mode) {

}

int tfsCreate(char *filename, int ownerPermissions, int othersPermissions) {
    int inumber = inode_create(getuid(), ownerPermissions, othersPermissions);
    
    
    return 0;
}