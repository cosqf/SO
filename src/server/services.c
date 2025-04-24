#include <sys/types.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <utils.h>
#include <glib.h>
#include <sys/wait.h>
#include <persistence.h>

#include <services.h>

/**
 * @brief Creates and initializes a ChildRequest structure with the specified command and document. */
ChildRequest* convertChildInfo (enum ChildCommand cmd, Document* doc) {
    ChildRequest* req = calloc (1, sizeof (ChildRequest));
    if (!req) {
        perror ("Calloc error");
        return NULL;
    }
    req->cmd = cmd;
    if (doc) req->doc = *doc;
    return req;
}

/**
 * @brief Sends a message from a child process to the server through the server FIFO.
 *
 * Converts a given command and a document into a ChildRequest, wraps it into a Message,
 * and writes it to the server's FIFO. 
 * 
 * @param cmd The command type to send (e.g., ADD, DELETE, LOOKUP, etc.).
 * @param doc A pointer to the Document related to the command.
 */
void sendMessageToServer (enum ChildCommand cmd, Document* doc) {
    ChildRequest* cr = convertChildInfo (cmd, doc);
    if (!cr) {
        perror ("Error converting doc into a command in child");
        return;
    }
    
    Message* msg = childToMessage (cr);
    if (!msg) {
        perror ("Error converting child request into message");
        free (cr);
        return;
    }
    int fifoWrite = open (SERVER_PATH, O_WRONLY);
    if (fifoWrite == -1) {
        perror ("Server didn't open");
        free (msg);
        free (cr);
    }   
    write (fifoWrite, msg, sizeof(Message));
    free(msg);
    close (fifoWrite);
    free (cr);
}

char* closeServer () {
    char* message = malloc (20);
    if (!message) {
        perror ("Malloc error");
        return NULL;
    }

    char msg[30];
    int len = snprintf(msg, sizeof(msg), "Shutting down server...\n");
    write(STDOUT_FILENO, msg, len);
    
    snprintf (message, 20, "-- Closed server!\n");
    sendMessageToServer (EXIT, NULL);
    return message;
}

char* addDoc (char* title, char* author, short year, char* fileName, char* pathDocs) {
    char fullPath[100]= {0};
    snprintf (fullPath, 100, "%s%s", pathDocs, fileName);
    
    char* message = malloc (50);
    if (!message) {
        perror ("Malloc error");
        return NULL;
    }  

    if (open (fullPath, O_RDONLY) == -1) { 
        perror ("File doesn't exist");
        perror (fullPath);
        snprintf (message, 20, "-- FILE NOT FOUND\n");
        return message;
    }

    int id = (int) time(NULL) ^ getpid();

    char msg[30];
    int len = snprintf(msg, sizeof(msg), "Adding doc %d\n", id);
    write(STDOUT_FILENO, msg, len);

    Document* doc = calloc (1, sizeof (Document)); 
    if (!doc) {
        perror("Calloc error");
        free (message);
        return NULL;
    }

    doc->id = id;
    strncpy(doc->title, title, sizeof(doc->title));
    strncpy(doc->authors, author, sizeof(doc->authors));
    strncpy(doc->path, fullPath, sizeof(doc->path));
    doc->year = year;
    
    sendMessageToServer (ADD, doc);
    free (doc);

    snprintf (message, 50, "-- Document indexed -- id: %d\n", id);
    return message;
}

char* consultDoc (DataStorage* ds, int id) {
    char* message = malloc(600);
    if (!message) {
        perror ("Malloc error");
        return NULL;
    }
    char msg[30];
    int len = snprintf(msg, sizeof(msg), "Looking up doc %d\n", id);
    write(STDOUT_FILENO, msg, len);
    
    Document* doc = lookupDoc (ds, id);
    if (!doc) {
        snprintf (message, 35, "-- DOCUMENT %d ISN'T INDEXED\n", id);
        return message;
    }
    sendMessageToServer (LOOKUP, doc);

    snprintf(message, 600, 
        "\n-- Document Information--\nId: %d\nTitle: %s\nAuthors: %s\nYear: %d\nPath: %s\n", 
        doc->id, doc->title, doc->authors, doc->year, doc->path);
    return message;  
}

char* deleteDoc (DataStorage* ds, int id){
    char* message = malloc(45);
    if (!message) {
        perror ("Malloc error");
        return NULL;
    }
    char msg[30];
    int len = snprintf(msg, sizeof(msg), "Deleting doc %d\n", id);
    write(STDOUT_FILENO, msg, len);
    
    Document* doc = lookupDoc (ds, id);
    if (doc) {
        sendMessageToServer (DELETE, doc);
        free (doc);

        snprintf (message, 45, "-- DOCUMENT %d REMOVED\n", id);
    }
    else snprintf (message, 45, "-- DOCUMENT %d ISN'T INDEXED\n", id);
    return message;
}

/**
 * @brief Counts the number of lines in a document that contain a specific keyword.
 *
 * This function uses a child process and pipes to execute the "grep -c -w" command,
 * which counts how many lines in the file at doc->path contain the given keyword
 * as a whole word.
 *
 * @param doc     Pointer to the Document whose file will be searched.
 * @param keyword The keyword to search for in the document.
 *
 * @return The number of matching lines, or 0 if not found or on error.
 */
int checkDocForKeywordCount (Document* doc, char* keyword) {
    int fildes[2];
    pipe(fildes);
    pid_t pid = fork();
    int number = 0;
    if (pid == 0) {  
        close(fildes[0]);  // close read end
        dup2(fildes[1], STDOUT_FILENO);  // redirect stdout to pipe
        close(fildes[1]);

        execlp("grep", "grep", "-c", "-w", keyword, doc->path, NULL);

        perror("execlp failed");
        _exit(1);
    } else {  
        close(fildes[1]);  // close write end
        char buffer[100];
        int bytesRead = read(fildes[0], buffer, sizeof(buffer) - 1);
        if (bytesRead <= 0) perror ("Error in grep");
        else {
            buffer[bytesRead-1] = '\0'; // to overwrite the \n
            number = convertToNumber (buffer);
        }
        close(fildes[0]);
        wait(NULL);
    }
    return number;
}

char* lookupKeyword (DataStorage* ds, int id, char* keyword) {
    int maxSizeMessage = 60;
    char* message = malloc(maxSizeMessage);
    if (!message) {
        perror ("Malloc error");
        return NULL;
    }

    char msg[30];
    int len = snprintf(msg, sizeof(msg), "Looking up keyword %s on doc %d\n", keyword, id);
    write(STDOUT_FILENO, msg, len);
    
    // gets doc from hash table
    Document* doc = lookupDoc (ds, id);
    if (!doc) {
        snprintf (message, 30, "DOCUMENT ISN'T INDEXED\n");
        return message;
    }
    sendMessageToServer (LOOKUP, doc);

    int count = checkDocForKeywordCount (doc, keyword);
    if (count != 0) snprintf (message, maxSizeMessage, "-- Keyword \'%s\' appears in the file %d times\n", keyword, count);
    else snprintf (message, maxSizeMessage, "-- Keyword \'%s\' doesn\'t appear in the file\n", keyword);
    return message;
}

/**
 * @brief Checks if a document contains the given keyword using 'grep'.
 *
 * Uses 'grep -q -w' via execlp to perform a quiet search for the keyword.
 * If the keyword is found, the document's ID is written to the given pipe.
 *
 * @param doc    Pointer to the Document to search.
 * @param keyword Keyword to look for.
 * @param fildes Pipe for sending the result to the parent.
 *
 * @return 1 if keyword is found, 0 otherwise.
 */
int checkDocForKeyword (Document* doc, char* keyword, int fildes[]){
    int status;
    pid_t pid = fork();
    if (pid == 0) {
        execlp ("grep", "grep", "-q", "-w", keyword, doc->path, NULL); // -q -> quiet, no output
        _exit (1);
    }
    else {
        wait(&status);
        if (WEXITSTATUS(status) != 1) {
            int id = doc->id;
            write (fildes[1], &id, sizeof (id));
            return 1;
        }
    }
    return 0;
}

/**
 * @brief Forks child processes to check subsets of documents in parallel.
 *
 * Each child checks a "chunk" of the documents array. If any document in
 * its chunk matches the keyword, it writes the ID to a pipe.
 *
 * @param nrProcesses Number of child processes to create.
 * @param tableSize   Total number of documents.
 * @param fildes      Pipe to write found IDs to.
 * @param keyword     Keyword to search for.
 * @param docs        Array of document pointers.
 */
void setUpChildren (int nrProcesses, int tableSize, int fildes[], char* keyword, GPtrArray* docs) {
    int chunkSize = (tableSize + nrProcesses - 1) / nrProcesses;
    for (int i = 0; i < nrProcesses; i++) {
        pid_t pid = fork();
        int any = 0;
        if (pid == 0) {
            int start = i * chunkSize;
            int end = (start + chunkSize > tableSize) ? tableSize : start + chunkSize;

            for (int j = start; j < end; ++j) {
                Document* doc = docs->pdata[j];
                int result = checkDocForKeyword(doc, keyword, fildes);
                if (!any && result) any = 1; 
            }
            _exit(any);
        }
    }
    close (fildes[1]);
}

/**
 * @brief Reads document IDs from the pipe sent by child processes.
 *
 * Dynamically reallocates array if more IDs are found than expected.
 *
 * @param fildes      Pipe file descriptors.
 * @param allDocuments Output array of document IDs.
 * @param arraySize   Initial allocated size.
 *
 * @return Number of document IDs read.
 */
int readIds(int fildes[], int** allDocuments, int arraySize) {
    int docId, i = 0;
    while (read (fildes[0], &docId, sizeof(docId)) > 0) {
        if (i >= arraySize) {
            int* biggerArray = realloc(*allDocuments, arraySize * 2 * sizeof(int));
            if (!biggerArray) {
                perror("Realloc failed");
                break;
            }
            *allDocuments = biggerArray;
            arraySize *= 2;
        }
        (*allDocuments)[i++] = docId;
    }
    return i;
}


char* lookupDocsWithKeyword (DataStorage* ds, char* keyword, int nrProcesses) {
    char msg[60];
    int len = snprintf(msg, sizeof(msg), "Searching for keyword \'%s\' on all documents\n", keyword);
    write(STDOUT_FILENO, msg, len);
    
    GPtrArray* docs = getAllDocuments(ds);
    int tableSize = docs->len; 
    nrProcesses = (nrProcesses > tableSize) ? tableSize : nrProcesses; // cap the nr of processes at size of table

    int status;
    int fildes[2];
    pipe (fildes);
    if (pipe(fildes) == -1) {
        perror("pipe");
        return NULL;
    }
    setUpChildren (nrProcesses, tableSize, fildes, keyword, docs);

    // receiving ids 
    int arrayInitialSize = 100;
    int* allDocuments = calloc(arrayInitialSize, sizeof(int));
    if (!allDocuments) {
        perror("Calloc error");
        return NULL;
    }
    int idCount = readIds(fildes, &allDocuments, arrayInitialSize);
    close(fildes[0]);

    //catching children
    int any = 0;
    for (int i = 0; i<nrProcesses; i++){
        wait(&status);
        if (WEXITSTATUS (status) == 1) any = 1;
    } 
    g_ptr_array_free(docs, TRUE);

    if (!any) {
        free (allDocuments);
        return "-- NO DOCUMENTS HAVE THE KEYWORD\n";
    }

    int estimatedLength = idCount * 12; // in digits, max 11 digits + /n
    char* message = malloc(estimatedLength + 50);
    if (!message) {
        perror ("Malloc error");
        free (allDocuments);
        return NULL;
        }
    int msgUsed = snprintf(message, 50, "-- The documents with the keyword are:\n");
    for (int j = 0; j < idCount; j++) {
        msgUsed += snprintf(message + msgUsed, estimatedLength + 50, "%d\n", allDocuments[j]);
    }
    message[msgUsed] = '\0';
    free (allDocuments);
    return message;
}