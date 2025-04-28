#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <server/server.h>
#include <server/services.h>
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

void readClient (ClientRequest buf, char* docPath, DataStorage* ds, int idCount) {
    int fifoWrite = open (buf.fifoPath, O_WRONLY);
    if (fifoWrite == -1) {
        perror ("Failed to open client FIFO");
        notifyChildExit();
        _exit(1);
    }
    int argc;
    char** commands = decodeClientInfo(buf, &argc);
    char* reply = processCommands(commands, buf.noCommand, docPath, ds, idCount);
    if (reply) {
        int replySize = strlen(reply);
        write (fifoWrite, &replySize, sizeof (replySize));
        write (fifoWrite, reply, replySize);
        free (reply);
    }
    else {
        char reply[] = "ERROR\n";
        int replySize = strlen (reply);
        write (fifoWrite, &replySize, sizeof (replySize));
        write (fifoWrite, reply, replySize);
    }
    close (fifoWrite);

    for (int i = 0; i < argc; i++) {
        free(commands[i]);
    }
    free(commands);

    notifyChildExit ();
    _exit(0);
}

int readChild (DataStorage* ds, ChildRequest childReq, int* idCount) { 
    switch (childReq.cmd) {
        case ADD:
            Document* docA = malloc(sizeof(Document));
            if (!docA) {
                perror("Malloc error");
                return 1;
            }
            *docA = childReq.doc;
            addDocToCache (ds, docA);
            (*idCount)++;
            return 0;     

        case DELETE:
            int idD = childReq.doc.id;
            removeDocIndexing (ds, idD);
            return 0;

        case LOOKUP:
            Document* docL = malloc (sizeof (Document));
            if (!docL) {
                perror ("Malloc error");
                return 1;
            }
            *docL = childReq.doc;
            addDocToCache (ds, docL); // will update cache positions
            return 0;

        case EXIT:
            destroyDataInMemory (ds);
            return 1;

        case CHILD_EXIT: 
            pid_t pid = childReq.doc.id;
            int wstatus;
            waitpid(pid, &wstatus, 0); 
            return 0;
        default:
            break;
    }
    return 0;
}

char* processCommands(char **commands, int noCommands, char* pathDocs, DataStorage* ds, int idCount) {
    if (!commands || !commands[0]) return strdup ("Invalid command\n");
    if (strcmp(commands[0], "-f") == 0) {
        return closeServer();
    }
    else if (strcmp(commands[0], "-a") == 0) {
        if (!commands[1] || !commands[2] || !commands[3] || !commands[4]) return strdup ("Invalid command\n");
        int year = convertToNumber (commands[3]);
        if (year == -1) return strdup ("Invalid command\n");
        return addDoc (commands[1], commands[2], year, commands[4], pathDocs, idCount);
    }
    else if (strcmp(commands[0], "-c") == 0){
        if (!commands[1]) return strdup ("Invalid command\n");
        int id = convertToNumber (commands[1]);
        if (id == -1) return NULL;
        return consultDoc (ds, id);
    }
    else if (strcmp(commands[0], "-d") == 0){
        if (!commands[1]) return strdup ("Invalid command\n");
        int id = convertToNumber (commands[1]);
        if (id == -1) return NULL;
        return deleteDoc (ds, id);
    } 
    else if (strcmp (commands[0], "-l") == 0) {
        if (!commands[1] || !commands[2]) return strdup ("Invalid command\n");
        int id = convertToNumber (commands[1]);
        if (id == -1) return NULL;
        return lookupKeyword (ds, id, commands[2]);
    }
    else if (strcmp (commands[0], "-s") == 0) {
        if (!commands[1]) return strdup ("Invalid command\n");
        int nr;
        if (noCommands == 2) nr = convertToNumber (commands[2]);
        else nr = 1;
        return lookupDocsWithKeyword (ds, commands[1], nr);
    }
    else return strdup ("Invalid command\n");
}
