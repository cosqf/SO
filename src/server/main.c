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
/*
O programa servidor é responsável por registar meta-informação sobre cada documento (p.ex, identificador único,
título, ano, autor, localização), permitindo também um conjunto de interrogações relativamente a esta meta-informação e ao
conteúdo dos documentos
*/

void readClient (ClientRequest buf, char* docPath, int cacheSize, GHashTable* table) { // run by child 
    for (int i = 0; i<5; i++) {
        printf ("%d. %s\n", i, buf.command[i]);
    }
    int fifoWrite = open (buf.fifoPath, O_WRONLY);
    if (fifoWrite == -1) {
        perror ("Failed to open client FIFO");
    }

    char** commands = decodeClientInfo(buf);
    char* reply = processCommands(commands, buf.noCommand, docPath, cacheSize, table);
    
    write (fifoWrite, reply, strlen(reply));
    close (fifoWrite);
    for (int i = 0; i < buf.noCommand; i++) {
        free(commands[i]);
    }
    free(commands);

    notifyChildExit ();

    _exit(0);
}

int readChild (GHashTable* docTable, ChildRequest childReq) { // read by parent
    switch (childReq.cmd) {
        case ADD:
            Document* docA = malloc(sizeof(Document));
            if (!docA) {
                perror("Malloc error");
                return 0;
            }
            *docA = childReq.doc;
            int id = getDocumentId(docA);

            printf ("adding doc %d\n", id);
            g_hash_table_insert (docTable, GINT_TO_POINTER (id), docA);
            return 0;        
        case DELETE:
            Document* docD = malloc(sizeof(Document));
            if (!docD) {
                perror("Malloc error");
                return 0;
            }
            *docD = childReq.doc;
            int idD = getDocumentId(docD);

            printf ("removing doc %d\n", idD);
            g_hash_table_remove (docTable, GINT_TO_POINTER (idD));
            free (docD);
            return 0;

        case EXIT:
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
        perror ("server didn't open");
		return 1;
    }
    GHashTable* docTable = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, free);

    Message buf;
    int keepGoing = 1;
    while (keepGoing) {
        int bytesRead = read (fifoRead, &buf, sizeof (Message));
        if (bytesRead <=0) continue;
        printf ("read %d\n", buf.type);
        
        if (buf.type == CLIENT) {
            pid_t pid = fork();
            if (pid == 0) readClient(buf.data.clientReq, argv[1], cacheNumber, docTable);
        }
        else {
            if (readChild (docTable,buf.data.childReq)) break;   
        }
    }
    // cleaning up
    g_hash_table_destroy (docTable);
    close (fifoRead);
    unlink (SERVER_PATH);
    return 0;
}