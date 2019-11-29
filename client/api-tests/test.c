#include "../tecnicofs-api-constants.h"
#include "../tecnicofs-client-api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

int main(int argc, char** argv) {
    int n1, n2, n3, n4, n5;
    char buffer[256];

    for(int i=0; i< 255; i++){
        buffer[i] = 'r';
    }

    assert(tfsMount(argv[1]) == 0);
    assert(tfsMount(argv[1]) == TECNICOFS_ERROR_OPEN_SESSION);
    
    assert(tfsCreate("abc", RW, READ) == 0);
    assert(tfsCreate("abc", RW, READ) == TECNICOFS_ERROR_FILE_ALREADY_EXISTS);
    assert(tfsCreate("bcd", READ, READ) == 0);
    assert(tfsCreate("cde", WRITE, READ) == 0);
    assert(tfsCreate("def", NONE, NONE) == 0);
    assert(tfsCreate("efg", RW, READ) == 0);
    assert(tfsCreate("fgh", RW, READ) == 0);
    assert(tfsCreate("ghi", RW, READ) == 0);

    assert(tfsDelete("z") == TECNICOFS_ERROR_FILE_NOT_FOUND);
    assert(tfsDelete("def") == 0);
    assert(tfsCreate("def", NONE, NONE) == 0);

    assert(tfsRename("def", "bcd") == TECNICOFS_ERROR_FILE_ALREADY_EXISTS);
    assert(tfsRename("z", "LOL") == TECNICOFS_ERROR_FILE_NOT_FOUND);
    assert(tfsRename("def", "LOL") == 0);

    assert(tfsCreate("LOL", RW, READ) == TECNICOFS_ERROR_FILE_ALREADY_EXISTS);
    assert(tfsCreate("def", RW, READ) == 0);

    assert((n1 = tfsOpen("abc", RW)) >= 0);
    assert(tfsOpen("bcd", WRITE) == TECNICOFS_ERROR_PERMISSION_DENIED);
    assert((n2 = tfsOpen("bcd", READ)) >= 0);
    assert(tfsOpen("cde", READ) == TECNICOFS_ERROR_PERMISSION_DENIED);
    assert(tfsOpen("cde", RW) == TECNICOFS_ERROR_PERMISSION_DENIED);
    assert((n3 = tfsOpen("cde", WRITE)) >= 0);
    assert(tfsOpen("LOL", READ) == TECNICOFS_ERROR_PERMISSION_DENIED);
    assert((n4 = tfsOpen("efg", RW)) >= 0);
    assert((n5 = tfsOpen("fgh", WRITE)) >= 0);
    assert(tfsOpen("ghi", READ) == TECNICOFS_ERROR_MAXED_OPEN_FILES);

    assert(tfsWrite(n1, "qwertyuiop", 100) == 0);
    assert(tfsRead(n1, buffer, 7) == 6);
    assert(strcmp(buffer, "qwerty") == 0);
    assert(tfsRead(n1, buffer, 50) == 10);
    assert(strcmp(buffer, "qwertyuiop") == 0);

    assert(tfsWrite(n2, "lol", 100) == TECNICOFS_ERROR_INVALID_MODE);
    assert(tfsRead(n2, buffer, 100) == 0);
    
    assert(tfsWrite(n3, "lol", 100) == 0);
    assert(tfsRead(n3, buffer, 100) == TECNICOFS_ERROR_INVALID_MODE);

    assert(tfsWrite(n4, "asdfghjkl", 4) == 0);
    assert(tfsRead(n4, buffer, 100) == 4);
    assert(strcmp(buffer, "asdf") == 0);

    assert(tfsRename("fgh", "xyz") == 0);
    assert(tfsWrite(n5, "hello", 10) == 0);
    assert(tfsClose(n5) == 0);
    assert((n5 = tfsOpen("xyz", RW)) >= 0);
    assert(tfsRead(n5, buffer, 10) == 5);
    assert(strcmp(buffer, "hello") == 0);

    assert(tfsClose(n1) == 0);
    assert(tfsClose(n1) == TECNICOFS_ERROR_FILE_NOT_OPEN);

    assert(tfsUnmount() == 0);
    assert(tfsUnmount() == TECNICOFS_ERROR_NO_OPEN_SESSION);

    printf("Success!\n");

    return 0;
} 
