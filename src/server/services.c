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
    char* message = malloc(550);
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

    snprintf(message, 550, 
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

    int count = checkDocForKeywordCount (doc, keyword);
    if (count != 0) snprintf (message, maxSizeMessage, "-- Keyword \'%s\' appears in the file %d times\n", keyword, count);
    else snprintf (message, maxSizeMessage, "-- Keyword \'%s\' doesn\'t appear in the file\n", keyword);

    return message;
}

/*
dclient -s "keyword"
Ainda, deve ser possível (opção -s) devolver uma lista de identificadores de documentos que contêm uma dada palavra-chave
(keyword).

Pesquisa concorrente. A operação de pesquisa por documentos que contêm uma dada palavra-chave (opção -s) deve poder ser
efetuada concorrentemente por vários processos. Ao suportar esta operação avançada, a mesma passa a receber um argumento
extra, nomeadamente o número máximo de processos a executar simultaneamente.
dclient -s "keyword" "nr_processes"
*/

int checkDocForKeyword (Document* doc, char* keyword, int fildes[]){
    int status, any = 0;
    pid_t pid = fork();
    if (pid == 0) {
        execlp ("grep", "grep", "-q", "-w", keyword, doc->path, NULL); // -q -> quiet, no output
        _exit (1);
    }
    else {
        wait(&status);
        if (WEXITSTATUS(status) != 1) {
            any = 1;
            int id = doc->id;
            write (fildes[1], &id, sizeof (id));
        }
    }
    return any;
}

void setUpChildren (int nrProcesses, int tableSize, int fildes[], char* keyword, Document** docs) {
    int chunkSize = (tableSize + nrProcesses - 1) / nrProcesses;
    for (int i = 0; i < nrProcesses; i++) {
        pid_t pid = fork();
        int any = 0;
        if (pid == 0) {
            int start = i * chunkSize;
            int end = (start + chunkSize > tableSize) ? tableSize : start + chunkSize;

            for (int j = start; j < end; ++j) {
                Document* doc = docs[j];
                int result = checkDocForKeyword(doc, keyword, fildes);
                if (!any && result) any = 1; 
            }
            _exit(any);
        }
    }
    close (fildes[1]);
}

int readIds(int fildes[], int** allDocuments, int arraySize) {
    int docId, i = 0;
    while (read(fildes[0], &docId, sizeof(docId)) > 0) {
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


char* lookupDocsWithKeyword (GHashTable* table, char* keyword, int nrProcesses) {
    int tableSize = g_hash_table_size (table);
    void** docs = gettingValuesOfHashTable (table);
    nrProcesses = (nrProcesses > tableSize) ? tableSize : nrProcesses; // cap the nr of processes at size of table

    int status;
    int fildes[2];
    pipe (fildes);
    setUpChildren (nrProcesses, tableSize, fildes, keyword, (Document**) docs);

    printf ("receiving ids\n");    
    int arrayInitialSize = 100;
    int* allDocuments = calloc(arrayInitialSize, sizeof(int));
    if (!allDocuments) {
        perror("calloc");
        return NULL;
    }
    int idCount = readIds(fildes, &allDocuments, arrayInitialSize);
    close(fildes[0]);

    printf ("catching children\n");
    int any = 0;
    for (int i = 0; i<nrProcesses; i++){
        wait(&status);
        if (WEXITSTATUS (status) == 1) any = 1;
    } 
    free (docs);
    if (!any) return "-- NO DOCUMENTS HAVE THE KEYWORD\n";

    int estimatedLength = idCount * 7; // in digits, max 6 digits + /n
    char* message = malloc(estimatedLength + 50);
    if (!message) {
        perror ("Malloc error");
        free (allDocuments);
        return NULL;
        }
    int msgUsed = snprintf(message, 50, "-- The documents with the keyword are:\n");
    for (int j = 0; j < idCount; j++) {
        msgUsed += sprintf(message + msgUsed, "%d\n", allDocuments[j]);
    }
    message[msgUsed] = '\0';
    free (allDocuments);
    printf ("finished iterating\n");
    return message;
}