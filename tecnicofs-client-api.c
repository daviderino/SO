#include <stdio.h>
#include <stdlib.h>
#include "tecnicofs-api-constants.h"

int tfsMount(char * address){
        int servlen=strlen(address);
        int fd= fopen(&argv[0],"r+");

        if(connect(fd,&argv[1],servlen)<0)
            err_dump("client: can't connect to server");
}