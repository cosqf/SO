#include <string.h>
#include <stdlib.h>
#include <string.h>

typedef struct clientinfo {
    char* fifoPath;
    char** command;
} ClientInfo;

void createClientFifo (char *path, int number) {
    sprintf (path, "tmp/fifoClient%d", number);

    if (mkfifo(path, 0666) == -1) {
    perror("mkfifo failed");
    exit(EXIT_FAILURE);
    }
}

ClientInfo* convertInfo (int argc, char** args, char path[]) {
    ClientInfo* ci = malloc (sizeof (ClientInfo));
    if (!ci) {
        perror ("Malloc error");
        return NULL;
    }
    ci->fifoPath = strdup (path);
    if (!ci->fifoPath) {
        perror ("Malloc error");
        free (ci);
        return NULL;
    }
    
    ci->command = malloc (argc * sizeof (char*));
    if (!ci->command) {
        perror ("Malloc error");
        free (ci->fifoPath);
        free (ci);
        return NULL;
    }
    // copy the args while excluding the first one (unnecessary)
    for (int i = 0; i < argc-1; i++) {
        ci->command[i] = strdup (args[i+1]);

        if (!ci->command[i]) {
            perror ("Malloc error");
            for (int j = 0; j < i; j++) free(ci->command[j]);
            free(ci->command);
            free(ci->fifoPath);
            free(ci);
            return NULL;
        }
    }
    ci->command[argc] = NULL;
    return ci;
}


void freeClientInfo(ClientInfo* ci) {
    if (!ci) return;
    free(ci->fifoPath);
    if (ci->command) {
        for (int i = 0; ci->command[i]; i++) {
            free(ci->command[i]);
        }
        free(ci->command);
    }
    free(ci);
}