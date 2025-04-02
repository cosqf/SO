#include <glib.h>

typedef struct doc Document;

char* addDoc (GHashTable* table, char* title, char* author, short year, char* fileName, char* pathDocs);

char* consultDoc (GHashTable* table, int id);

char* deleteDoc (GHashTable* table, int id);