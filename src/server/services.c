#include <sys/types.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <utils.h>
#include <glib.h>
#include <sys/wait.h>

#include <server/services.h>

void printDoc2(Document* doc) { // debug
    printf("\nId: %d\nTitle: %s\nAuthors: %s\nPath: %s\nYear: %d\n", 
           doc->id, doc->title, doc->authors, doc->path, doc->year);
}


ChildRequest* convertChildInfo (enum ChildCommand cmd, Document* doc) {
    ChildRequest* req = malloc (sizeof (ChildRequest));
    if (!req) {
        perror ("Malloc error");
        return NULL;
    }
    memset(req, 0, sizeof(ChildRequest)); // avoid using uninitialized memory

    req->cmd = cmd;
    if (doc) req->doc = *doc;
    return req;
}

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

// dclient -f
char* closeServer () {
    char* message = malloc (20);
    if (!message) {
        perror ("Malloc error");
        return NULL;
    }
    
    snprintf (message, 20, "-- Closed server!\n");
    sendMessageToServer (EXIT, NULL);
    return message;
}

// dclient -a "title" "authors" "year" "path"
char* addDoc (GHashTable* table, char* title, char* author, short year, char* fileName, char* pathDocs) {
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

    int id = getpid();

    Document* doc = malloc (sizeof (Document));
    if (!doc) {
        perror("Malloc error");
        free (message);
        return NULL;
    }
    memset(doc, 0, sizeof(Document)); // avoid using uninitialized memory

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

//dclient -c "key"
char* consultDoc (GHashTable* table, int id) {
    char* message = malloc(MAX_RESPONSE_SIZE);
    if (!message) {
        perror ("Malloc error");
        return NULL;
    }

    printf ("looking up id %d\n", id);
    Document* doc = g_hash_table_lookup (table, GINT_TO_POINTER (id));

    if (!doc) {
        snprintf (message, 35, "-- DOCUMENT %d ISN'T INDEXED\n", id);
        return message;
    }

    snprintf(message, MAX_RESPONSE_SIZE, 
        "\n-- Document Information--\nId: %d\nTitle: %s\nAuthors: %s\nYear: %d\nPath: %s\n", 
        doc->id, doc->title, doc->authors, doc->year, doc->path);
    return message;  
}

//dclient -d "key"
char* deleteDoc (GHashTable* table, int id){
    char* message = malloc(35);
    if (!message) {
        perror ("Malloc error");
        return NULL;
    }
    printf ("looking up id %d\n", id);
    Document* doc = g_hash_table_lookup (table, GINT_TO_POINTER (id));
    if (doc) {
        sendMessageToServer (DELETE, doc);
        free (doc);

        snprintf (message, 35, "-- Document %d removed\n", id);
        return message;
    }
    else {
        snprintf (message, 35, "-- Document %d isn\'t indexed\n", id);
        return message;
    }
    return 0;
}

/*
dclient -l "key" "keyword"
Em detalhe, deve ser possível (opção -l) devolver o número de linhas de um dado documento (i.e., identificado pela sua key) que
contêm uma dada palavra-chave (keyword).
*/

int checkDocForKeyword (Document* doc, char* keyword) {
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
        exit(1);
    } else {  
        close(fildes[1]);  // close write end
        char buffer[100];
        int bytesRead = read(fildes[0], buffer, sizeof(buffer) - 1);
        if (bytesRead <= 0) perror ("Error in grep");
        else {
            buffer[bytesRead-1] = '\0'; //to overwrite the \n
            number = convertToNumber (buffer);
        }
        close(fildes[0]);
        wait(NULL);
    }
    return number;
}

char* lookupKeyword (GHashTable* table, int id, char* keyword) {
    int maxSizeMessage = 60;
    char* message = malloc(maxSizeMessage);
    if (!message) {
        perror ("Malloc error");
        return NULL;
    }
    // gets doc from hash table
    printf ("looking up id %d\n", id);
    Document* doc = g_hash_table_lookup (table, GINT_TO_POINTER (id));
    if (!doc) {
        snprintf (message, 30, "DOCUMENT ISN'T INDEXED\n");
        return message;
    }

    int count = checkDocForKeyword (doc, keyword);
    if (count != 0) snprintf (message, maxSizeMessage, "-- Keyword \'%s\' appears in the file %d times\n", keyword, count);
    else snprintf (message, maxSizeMessage, "-- Keyword \'%s\' doesn\'t appear in the file\n", keyword);

    return message;
}

/*
dclient -s "keyword"
Ainda, deve ser possível (opção -s) devolver uma lista de identificadores de documentos que contêm uma dada palavra-chave
(keyword).
*/

char* lookupDocsWithKeyword (GHashTable* table, char* keyword) {
    char* message = malloc(MAX_RESPONSE_SIZE);
    if (!message) {
        perror ("Malloc error");
        return NULL;
    }
    snprintf (message, MAX_RESPONSE_SIZE, "-- The documents with the keyword are:\n");

    int any = 0;
    printf ("iterating hash table\n");
    GHashTableIter iter;
    gpointer key, value;
    g_hash_table_iter_init (&iter, table);
    while (g_hash_table_iter_next (&iter, &key, &value)) {
        Document* doc = (Document*) value;
        int status;
        pid_t pid = fork();
        if (pid == 0) {
            execlp ("grep", "grep", "-q", "-w", keyword, doc->path, NULL); // -q -> quiet, no output
            exit (1);
        }
        else {
            wait(&status);
            if (WEXITSTATUS(status) != 1) {
                any = 1;
                char addMessage[10];
                snprintf(addMessage, sizeof(addMessage), "%d\n", doc->id);
                strcat(message, addMessage);
            }
        }
    }
    printf ("finished iterating\n");
    if (!any) snprintf (message, MAX_RESPONSE_SIZE, "-- NO DOCUMENTS HAVE THE KEYWORD \'%s\'\n", keyword);
    return message;
}