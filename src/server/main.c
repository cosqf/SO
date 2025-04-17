#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <glib.h>

#include <utils.h>
#include <client/client.h>
#include <server/server.h>
#include <server/services.h>
#include <persistence.h>

/*
O programa servidor é responsável por registar meta-informação sobre cada documento (p.ex, identificador único,
título, ano, autor, localização), permitindo também um conjunto de interrogações relativamente a esta meta-informação e ao
conteúdo dos documentos
*/

void readClient (ClientRequest buf, char* docPath, int cacheSize, DataStorage* ds) { // run by child  
    printf ("\n");
    for (int i = 0; i<5; i++) {
        printf ("%d. %s\n", i, buf.command[i]);
    }
    int fifoWrite = open (buf.fifoPath, O_WRONLY);
    if (fifoWrite == -1) {
        perror ("Failed to open client FIFO");
    }

    char** commands = decodeClientInfo(buf);
    char* reply = processCommands(commands, buf.noCommand, docPath, cacheSize, ds);
    
    int replySize = strlen(reply);
    write (fifoWrite, &replySize, sizeof (replySize));
    write (fifoWrite, reply, strlen(reply));
    close (fifoWrite);

    for (int i = 0; i < buf.noCommand; i++) {
        free(commands[i]);
    }
    free(commands);

    notifyChildExit ();

    _exit(0);
}

int readChild (DataStorage* ds, ChildRequest childReq) { // read by parent
    switch (childReq.cmd) {
        case ADD:
            Document* docA = malloc(sizeof(Document));
            if (!docA) {
                perror("Malloc error");
                return 1;
            }
            *docA = childReq.doc;
            int id = docA->id;

            printf ("adding doc %d\n", id);
            addDocToCache (ds, docA);
            return 0;        
        case DELETE:
            int idD = childReq.doc.id;

            printf ("removing doc %d\n", idD);
            removeDocIndexing (ds, idD);
            return 0;
        case LOOKUP:
            Document* docL = malloc (sizeof (Document));
            if (!docL) {
                perror ("Malloc error");
                return 1;
            }
            *docL = childReq.doc;

            int idL = childReq.doc.id;
            printf ("looking up %d\n", idL);
            addDocToCache (ds, docL); // will update cache positions
            return 0;
        case EXIT:
            destroyDataInMemory (ds);
            printf("shutting down server...\n");
            return 1;

        case CHILD_EXIT: {
            pid_t pid = childReq.doc.id;
            int wstatus;
            waitpid(pid, &wstatus, 0); 
            return 0;
        }
        default:
            break;
    }
    return 0;
}

    // $ ./dserver document_folder cache_size
int main(int argc, char **argv) {
    if (argc != 3) {
        perror ("Wrong number of arguments");
        exit (EXIT_FAILURE);
    }
    int cacheNumber = convertToNumber (argv[2]);
    if (cacheNumber == -1) exit (EXIT_FAILURE);;

    printf ("creating server fifo\n");

    // create server FIFO (read only)
    createServerFifo ();
    printf ("waiting for client\n");
    int fifoRead = open (SERVER_PATH, O_RDONLY);
    if (fifoRead == -1) {
        perror ("Server didn't open");
		return 1;
    }
    DataStorage* ds = initializeDataStorage (cacheNumber);

    Message buf;
    int keepGoing = 1;
    while (keepGoing) {
        int bytesRead = read (fifoRead, &buf, sizeof (Message));
        if (bytesRead <=0) continue;
   
        if (buf.type == CLIENT) {
            pid_t pid = fork();
            if (pid == 0) readClient(buf.data.clientReq, argv[1], cacheNumber, ds);
        }
        else {
            if (readChild (ds, buf.data.childReq)) break;   
        }
    }
    // cleaning up
    close (fifoRead);
    unlink (SERVER_PATH);
    return 0;
}


// double free