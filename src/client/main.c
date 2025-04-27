#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include "client/client.h"

/**
 * @file main.c
 * The users must use a client program to interact with the service (i.e. with the server program). This interaction will allow the 
 * users to add or remove the indexation of a document and make searches about the indexed documents. It's worth noting that the
 * client program only executes one operation per execution, which means it isn't an interactive program (i.e. that will 
 * read many operations from stdin).
 */

/**
 * @brief Main function of the client.
 * 
 * The client establishes communication with the document server via FIFOs. 
 * It sends a request, waits for the response, and outputs it to the terminal.
 *
 * Usage:
 *  - dclient -f                                   (close server)
 *  - dclient -a "title" "authors" "year" "file"   (add document)
 *  - dclient -c "key"                             (consult document by ID)
 *  - dclient -d "key"                             (delete document)
 *  - dclient -l "key" "keyword"                   (count keyword occurrences in a document)
 *  - dclient -s "keyword" ["nr_procs"]            (search documents with keyword using concurrent search)
 */
int main(int argc, char** argv) {
    // open server FIFO (write only)
    int fifoWrite = open (SERVER_PATH, O_WRONLY);
    if (fifoWrite == -1) {
        perror ("Server didn't open");
		return 1;
    }

    // create client FIFO (read only)
    pid_t pidNo = getpid();
    char pathFifo[20];

    createClientFifo (pathFifo, pidNo);

    // convert args to ClientRequest
    ClientRequest* cInfo = convertInfo (argc, argv, pathFifo);
    if (!cInfo){
        close(fifoWrite);
        return 1;
    }
    Message* msg = clientToMessage (cInfo);

    char msg1[30];
    int len1 = snprintf(msg1, sizeof(msg1), "Talking to the server...\n");
    write(STDOUT_FILENO, msg1, len1);

    // send client info to server
    int bytesWritten = write (fifoWrite, msg, sizeof (Message));
    if (bytesWritten < 0) {
        perror ("No bytes written to server");
        free (msg);
        freeClientRequest(cInfo);
        close(fifoWrite);
        unlink(pathFifo);
        return 1;
    }

    // open pipe to get server's response
    int fifoRead = open (pathFifo, O_RDONLY);
    if (fifoRead == -1) {
        perror ("Client didn't open");
        close(fifoWrite);
        unlink(pathFifo);
        return 1;
    }
    free (msg);
    freeClientRequest(cInfo);
    close(fifoWrite);
  
    char msg2[35];
    int len2 = snprintf(msg2, sizeof(msg2), "Waiting for server's response\n");
    write(STDOUT_FILENO, msg2, len2);
 
    // read server's resonse
    int sizeResponse; // first read the size of the message
    if (read(fifoRead, &sizeResponse, sizeof(sizeResponse)) <= 0) {
        perror("Failed to read response size");
        close(fifoRead);
        unlink(pathFifo);
        return 1;
    }

    char buf[sizeResponse + 1];
    if (read (fifoRead, buf, sizeResponse) != sizeResponse) perror ("Read different size than announced");
    buf[sizeResponse] = '\0';

    // write to user
    write (STDOUT_FILENO, buf, sizeResponse);

    // cleanup
    close(fifoRead);
    unlink(pathFifo);

    return 0;
}
