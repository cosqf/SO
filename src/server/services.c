#include <sys/types.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <utils.h>
#include <glib.h>
#include <sys/wait.h>

#include <server/services.h>

int getDocumentId (Document* doc) {
    return doc->id;
}

ChildRequest* convertChildInfo (enum ChildCommand cmd, Document* doc) {
    ChildRequest* req = malloc (sizeof (ChildRequest));
    if (!req) {
        perror ("Malloc error");
        return NULL;
    }
    req->cmd = cmd;
    req->doc = *doc;
    return req;
}

void sendMessageToServer (enum ChildCommand, Document* doc) {
    ChildRequest* cr = convertChildInfo (ADD, doc);
    if (!cr) {
        free (doc);
    }
    Message* msg = childToMessage (cr);
    if (!msg) {
        free (doc);
        free (cr);
    }

    int fifoWrite = open (SERVER_PATH, O_WRONLY);
    if (fifoWrite == -1) {
        perror ("Server didn't open");
        free (doc);
        free (cr);
    }   
    write (fifoWrite, msg, sizeof(Message));
    close (fifoWrite);
    free (cr);
    free (msg);
}

// dclient -a "title" "authors" "year" "path"
char* addDoc (GHashTable* table, char* title, char* author, short year, char* fileName, char* pathDocs) {
    char fullPath[100]= {0};
    snprintf (fullPath, 100, "%s%s", pathDocs, fileName);

    if (open (fullPath, O_RDONLY) == -1) { 
        perror ("File doesn't exist");
        return NULL;
    }

    int id = getpid();

    Document* doc = malloc (sizeof (Document));
    if (!doc) {
        perror("Malloc error");
        return NULL;
    }

    doc->id = id;
    strncpy(doc->title, title, sizeof(doc->title));
    strncpy(doc->authors, author, sizeof(doc->authors));
    strncpy(doc->path, fullPath, sizeof(doc->path));
    doc->year = year;

    sendMessageToServer (ADD, doc);

    char* message = malloc (50);
    if (!message) {
        perror ("Malloc error");
        free (doc);
        return NULL;
    }

    snprintf (message, 50, "Document indexed -- id: %d\n", id);
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
        snprintf (message, 30, "DOCUMENT ISN'T INDEXED\n");
        return message;
    }
    
    snprintf(message, MAX_RESPONSE_SIZE, 
        "--DOCUMENT INFORMATION--\nId: %d\nTitle: %s\nAuthors: %s\nYear: %d\nFile name: %s\n", 
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
    Document* doc = g_hash_table_lookup (table, GINT_TO_POINTER (id));
    if (doc) {

        sendMessageToServer (DELETE, NULL);

        snprintf (message, 35, "DOCUMENT %d REMOVED\n", id);
        return message;
    }
    else {
        snprintf (message, 35, "DOCUMENT %d ISN'T INDEXED\n", id);
        return message;
    }
    return 0;
}

/*
dclient -l "key" "keyword"
Em detalhe, deve ser possível (opção -l) devolver o número de linhas de um dado documento (i.e., identificado pela sua key) que
contêm uma dada palavra-chave (keyword).
*/
char* lookupKeyword (GHashTable* table, int id, char* keyword, char* pathDocs) {
    char* message = malloc(MAX_RESPONSE_SIZE);
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

    char fullPath[100]= {0};
    snprintf (fullPath, 100, "%s%s", pathDocs, doc->path);

    int fildes[2];
    pipe(fildes);
    pid_t pid = fork();

    if (pid == 0) {  
        close(fildes[0]);  // close read end
        dup2(fildes[1], STDOUT_FILENO);  // redirect stdout to pipe
        close(fildes[1]);

        execlp("grep", "grep", "-c", keyword, fullPath, NULL);
        perror("execlp failed");
        exit(1);
    } else {  
        close(fildes[1]);  // close write end
        char buffer[100];
        int bytesRead = read(fildes[0], buffer, sizeof(buffer) - 1);
        if (bytesRead <= 0) perror ("Error in grep");
        else {
            buffer[bytesRead] = '\0';
            snprintf (message, MAX_RESPONSE_SIZE, "\'%s\' APPEARS IN FILE NUMBER %d %s TIMES\n", keyword, id, buffer);
        }
        close(fildes[0]);
        wait(NULL);
    }

    return message;
}
