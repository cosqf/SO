#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "client/client.h"
#include <sys/types.h>
#include <sys/stat.h>

void createClientFifo (char path[], int number) {
    sprintf (path, CLIENT_FIFO "%d", number);

    if (mkfifo(path, 0666) == -1) {
    perror("mkfifo failed");
    exit(EXIT_FAILURE);
    }
}


ClientRequest* convertInfo (int argc, char** args, char path[]) {
    ClientRequest* req = malloc (sizeof (ClientRequest));
    if (!req) {
        perror ("Malloc error");
        return NULL;
    }
    strncpy(req->fifoPath, path, sizeof(req->fifoPath) - 1);
    req->fifoPath[sizeof(req->fifoPath) - 1] = '\0';

    // copy the args while excluding the first one (unnecessary)
    int i;
    int max_arg = (argc-1 < 5) ? (argc - 1) : 4;
    for (i = 0; i < max_arg; i++) {
        snprintf(req->command[i], sizeof(req->command[i]), "%s", args[i + 1]);
    }
    req->noCommand = i-1;    
    return req;
}


void freeClientRequest(ClientRequest* req) {
    if (!req) return;
    free(req);
}
