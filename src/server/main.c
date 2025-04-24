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

/**
 * @file main.c
 * The server program is responsible for registering meta-information about each document 
 * (eg. unique id, title, year, author, file location), allowing also for requests related to that meta-info and document content. 
 */


/**
 * Main function of the server.
 * It will create a server fifo, which it will always be reading, and will await for messages.
 * Messages can come from clients, with requests that will be handled by children of the main processor.
 * The children will then send a message to the parent indicating if any change needs to be done to the
 *  data base, such as adding or removing a document or simply to inform they've finished their jobs and 
 *  must be cleaned. 
 * To initialize use: $ ./dserver document_folder cache_size
 */
int main(int argc, char **argv) {
    if (argc != 3) {
        perror ("Wrong number of arguments");
        exit (EXIT_FAILURE);
    }
    int cacheNumber = convertToNumber (argv[2]);
    if (cacheNumber == -1) exit (EXIT_FAILURE);;

    createServerFifo ();

    char msg1 [] = "Waiting for client\n";
    write (STDOUT_FILENO, msg1, sizeof(msg1));

    int fifoRead = open (SERVER_PATH, O_RDONLY);
    if (fifoRead == -1) {
        perror ("Server didn't open");
		return 1;
    }
    DataStorage* ds = initializeDataStorage (cacheNumber);

    Message buf;
    while (1) {
        int bytesRead = read (fifoRead, &buf, sizeof (Message));
        if (bytesRead <=0) continue;
   
        if (buf.type == CLIENT) {
            pid_t pid = fork();
            if (pid == 0) readClient(buf.data.clientReq, argv[1], ds);
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