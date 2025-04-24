#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <server/server.h>
#include <server/services.h>
#include <utils.h>
#include <glib.h>
#include <protocol.h>

void createServerFifo () {
    if (mkfifo(SERVER_PATH, 0666) == -1) {
    perror("mkfifo failed");
    exit(EXIT_FAILURE);
    }
}

void notifyChildExit() {
    Document doc = { .id = getpid() }; 
    ChildRequest req = { .cmd = CHILD_EXIT, .doc = doc };

    Message msg = { .type = CHILD, .data.childReq = req };

    int fd = open(SERVER_PATH, O_WRONLY);
    if (fd == -1) {
        perror ("Failed to open server FIFO");
        return;
    }
    write(fd, &msg, sizeof(msg));
    close(fd);
}

char* processCommands(char **commands, int noCommands, char* pathDocs, DataStorage* ds) {
    char invalidMsg[] = "Invalid command\n";
    if (!commands || !commands[0]) return invalidMsg;
    if (strcmp(commands[0], "-f") == 0) {
        return closeServer();
    }
    else if (strcmp(commands[0], "-a") == 0) {
        if (!commands[1] || !commands[2] || !commands[3] || !commands[4]) return invalidMsg;
        int year = convertToNumber (commands[3]);
        if (year == -1) return NULL;
        return addDoc (commands[1], commands[2], year, commands[4], pathDocs);
    }
    else if (strcmp(commands[0], "-c") == 0){
        if (!commands[1]) return invalidMsg;
        int id = convertToNumber (commands[1]);
        if (id == -1) return NULL;
        return consultDoc (ds, id);
    }
    else if (strcmp(commands[0], "-d") == 0){
        if (!commands[1]) return invalidMsg;
        int id = convertToNumber (commands[1]);
        if (id == -1) return NULL;
        return deleteDoc (ds, id);
    } 
    else if (strcmp (commands[0], "-l") == 0) {
        if (!commands[1] || !commands[2]) return invalidMsg;
        int id = convertToNumber (commands[1]);
        if (id == -1) return NULL;
        return lookupKeyword (ds, id, commands[2]);
    }
    else if (strcmp (commands[0], "-s") == 0) {
        if (!commands[1]) return invalidMsg;
        int nr;
        if (noCommands == 2) nr = convertToNumber (commands[2]);
        else nr = 1;
        return lookupDocsWithKeyword (ds, commands[1], nr);
    }
    else return invalidMsg; 
}
