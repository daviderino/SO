#ifndef SOCK_H
#define SOCK_H

#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

int serverSocketMount(struct sockaddr_un server_addr, char *name, int *length);
int serverSocketUnmount();

#endif