//#include <glib.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <server/services.h>

typedef struct doc {
    int id;
    char title[200];
    char authors[200];
    char path[64];
    short year;
} Document;

// dclient -a "title" "authors" "year" "path"
char* addDoc (char* title, char* author, short year, char* fileName, char* pathDocs) {
    char fullPath[100] = strcat (fullPath, fileName);
    if (open (fullPath, O_RDONLY) == -1) {   // too slow ?
        perror ("File doesn't exist");
        return -1;
    }
    pid_t pid = getpid();

    Document doc;
    doc.id = pid;
    strcpy (doc.title, title);
    strcpy (doc.authors, author);
    strcpy (doc.path, fileName);
    doc.year = year;
    // add to hash (pid, doc)
    char* message = malloc (40);
    if (!message) {
        perror ("Malloc error");
        return NULL;
    }

    snprintf (message, 40, "Document indexed -- id: %d\n", pid);
    return message;
}

//dclient -c "key"
char* consultDoc (int id) {
    // if id not in hash return null;
    Document doc; // gotten from hash (id)
    char* message = malloc(500);
    if (!message) {
        perror ("Malloc error");
        return NULL;
    }

    snprintf(message, 500, "--DOCUMENT INFORMATION--\nId: %d\nTitle: %s\nAuthors: %s\nYear: %d\nFile name: %s\n", 
        doc.id, doc.title, doc.authors, doc.year, doc.path);
    
    return message;  
}

//dclient -d "key"
int deleteDoc (int id){
    // if id not in hash return 1;
    // remove from hash (id)
    return 0;
}