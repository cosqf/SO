#include <sys/types.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <utils.h>
#include <glib.h>

#include <server/services.h>
#include <protocol.h>

typedef struct doc {
    int id;
    char title[200];
    char authors[200];
    char path[64];
    short year;
} Document;

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

    gboolean b = g_hash_table_insert (table, GINT_TO_POINTER (id), doc);
 
    char* message = malloc (50);
    if (!message) {
        perror ("Malloc error");
        free (doc);
        return NULL;
    }

    snprintf (message, 50, "Document indexed -- id: %d\n", id);
    printf ("message sent: %s\n", message);
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
        printf ("message sent: %s\n", message);
        return message;
    }
    
    snprintf(message, MAX_RESPONSE_SIZE, 
        "--DOCUMENT INFORMATION--\nId: %d\nTitle: %s\nAuthors: %s\nYear: %d\nFile name: %s\n", 
        doc->id, doc->title, doc->authors, doc->year, doc->path);
    printf ("message sent: %s\n", message);
    return message;  
}

//dclient -d "key"
char* deleteDoc (GHashTable* table, int id){
    char* message = malloc(35);
    if (!message) {
        perror ("Malloc error");
        return NULL;
    }
    gboolean bool = g_hash_table_remove (table, &id);
    if (bool) {
        snprintf (message, 35, "DOCUMENT %d REMOVED\n", id);
        printf ("message sent: %s\n", message);
        return message;
    }
    else {
        snprintf (message, 35, "DOCUMENT %d ISN'T INDEXED\n", id);
        printf ("message sent: %s\n", message);
        return message;
    }
    return 0;
}