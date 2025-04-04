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

void readClient (ClientRequest buf, char* docPath, int cacheSize, GHashTable* table) {
    //printf ("Path: %s%s\n", docPath, buf.fifoPath);
    for (int i = 0; i<8; i++) {
        printf ("%d. %s\n", i, buf.command[i]);
    }
    int fifoWrite = open (buf.fifoPath, O_WRONLY);
    if (fifoWrite == -1) {
        perror ("Failed to open client FIFO");
    }

    char** commands = decodeClientInfo(buf);
    char* reply = processCommands(commands, docPath, cacheSize, table);
    
    if (strcmp (reply, "exit") == 0) {
        close (fifoWrite);
        for (int i = 0; i < 8 && commands[i][0] != '\0'; i++) {
            free(commands[i]);
        }
        free(commands);
        _exit (1);
    }
    write (fifoWrite, reply, strlen(reply));

    close (fifoWrite);
    for (int i = 0; i < 8 && commands[i][0] != '\0'; i++) {
        free(commands[i]);
    }
    free(commands);
    
    _exit(0);
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
    GHashTable* docTable = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, free);

    Message buf;
    int status;
    while (1) {
        int bytesRead = read (fifoRead, &buf, sizeof (Message));
        if (bytesRead <=0) continue;
        printf ("read %d\n", buf.type);
        
        if (buf.type == CLIENT) {
            pid_t pid = fork();
            if (pid == 0) readClient(buf.data.clientReq, argv[1], cacheNumber, docTable);

            wait(&status);
            if (WEXITSTATUS(status) == 1) {  
                printf("Shutting down server...\n");
                break;
            }
        }
        else {
            Document doc = buf.data.childReq.doc;
            int id = getDocumentId(&doc);

            switch (buf.data.childReq.cmd) {
            case ADD:
            printf ("adding doc %d\n", id);
                g_hash_table_insert (docTable, GINT_TO_POINTER (id), &doc);
                break;
            
            case DELETE:
                printf ("removing doc %d\n", id);
                g_hash_table_remove (docTable, &id);
                break;
            
            default: break;
            }
        }
        
    }
    close (fifoRead);
    unlink (SERVER_PATH);
    return 0;
}


// TODO: fix exiting the server