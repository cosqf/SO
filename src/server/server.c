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

char* processCommands(char **commands, char* pathDocs, int cacheSize, GHashTable* table) {
    if (!commands || !commands[0]) return "Invalid command";
    if (strcmp(commands[0], "-f") == 0) {
        printf ("closing\n");
        return closeServer();
    }
    else if (strcmp(commands[0], "-a") == 0) {
        printf ("adding doc\n");
        int year = convertToNumber (commands[3]);
        if (year == -1) return NULL;
        return addDoc (table, commands[1], commands[2], year, commands[4], pathDocs);
    }
    else if (strcmp(commands[0], "-c") == 0){
        int id = convertToNumber (commands[1]);
        if (id == -1) return NULL;
        return consultDoc (table, id);
    }
    else if (strcmp(commands[0], "-d") == 0){
        int id = convertToNumber (commands[1]);
        if (id == -1) return NULL;
        return deleteDoc (table, id);
    } 
    else if (strcmp (commands[0], "-l") == 0) {
        int id = convertToNumber (commands[1]);
        if (id == -1) return NULL;
        return lookupKeyword (table, id, commands[2]);
    }
    else if (strcmp (commands[0], "-s") == 0) {
        return lookupDocsWithKeyword (table, commands[1]);
    }
    
    else return "Yippee\n"; 
}
