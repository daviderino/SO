#include "constants.h"
#include "sock.h"

int serverSocketMount(struct sockaddr_un server_addr, char *name, int *length) {
    int sockfd;

    if((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        return -1;
    }

    if(unlink(name) != 0) {
        perror("Error unlinking socket name");
        return -1;
    }

    bzero((char*)&server_addr, sizeof(server_addr));

    server_addr.sun_family = AF_UNIX;
    strcpy(server_addr.sun_path, name);
    *length = strlen(server_addr.sun_path) + sizeof(server_addr.sun_family);

    if(bind(sockfd, (struct sockaddr*)&server_addr, *length) < 0) {
        perror("Failed to bind local address");
        return - 1;
    }   

    if(listen(sockfd, MAX_CONNECTIONS) < 0) {
        perror("Error while listening");
        return - 1;
    }

    return sockfd;
}
