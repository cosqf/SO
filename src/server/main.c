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
 * @brief Handles a single client request in a child process.
 *
 * This function is executed by a forked child. It decodes the client's request,
 * processes the command(s), and sends a response back through the provided FIFO path.
 * It safely manages resources, ensures the response is written to the FIFO, and then
 * terminates the child process cleanly.
 *
 * @param buf         The ClientRequest structure containing the command info and FIFO path.
 * @param docPath     The path to the document directory.
 * @param ds          A pointer to the DataStorage structure used to access cached/indexed documents.
 */
void readClient (ClientRequest buf, char* docPath, DataStorage* ds) {
    int fifoWrite = open (buf.fifoPath, O_WRONLY);
    if (fifoWrite == -1) {
        perror ("Failed to open client FIFO");
        notifyChildExit();
        _exit(1);
    }
    int argc;
    char** commands = decodeClientInfo(buf, &argc);
    char* reply = processCommands(commands, buf.noCommand, docPath, ds);
    if (reply) {
        int replySize = strlen(reply);
        write (fifoWrite, &replySize, sizeof (replySize));
        write (fifoWrite, reply, replySize);
        free (reply);
    }
    close (fifoWrite);

    for (int i = 0; i < argc; i++) {
        free(commands[i]);
    }
    free(commands);

    notifyChildExit ();
    _exit(0);
}

/**
 * @brief Handles internal communication from child processes and manages server-side state.
 *
 * This function is called by the parent process to process requests sent from child processes.
 * These requests can be adding, removing or looking up documents, catching a dead child or shutting down the server.
 *
 * @return int       Returns 0 for normal operations, or 1 in the following cases:
 *                   - Memory allocation failure
 *                   - When the server is instructed to shut down (EXIT command)
 *
 * @note This function should only be called by the parent process.
 *       Child processes must not invoke this function.
 */
int readChild (DataStorage* ds, ChildRequest childReq) { 
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