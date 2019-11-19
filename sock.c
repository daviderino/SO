#include "sock.h"

/* Creats a socket stream, UNIX domain, and returns its file descriptor */
int socketCreateStream() {
    int sockfd;

    if((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) != 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    return sockfd;
}

int socketNameStream(struct sockaddr_un serv_addr, char *name) {
    int serv_len;
    if(unlink(name) != 0) {
        perror("Error unlinking socket name");
    }

    bzero((char*)&serv_addr, sizeof(serv_addr));

    serv_addr.sun_family = AF_UNIX;
    strcpy(serv_addr.sun_path, name);
    serv_len = strlen(serv_addr.sun_path) + sizeof(serv_addr.sun_family);
}

void socketBind(int sockfd, struct sockaddr_un serv_addr, int len_serv) {
    if(bind(sockfd, (struct sockaddr*)&serv_addr, len_serv) < 0) {
        perror("Failed to bind local address");
    }
}

void socketListen(int sockfd, int n) {
    if(listen(sockfd, n) != 0) {
        perror("Error while listening");
    }
}