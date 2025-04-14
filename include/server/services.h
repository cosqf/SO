#include <glib.h>
#include <protocol.h>

#ifndef SERVICES_H
#define SERVICES_H

ChildRequest* convertChildInfo (enum ChildCommand cmd, Document* doc);

char* closeServer ();

char* addDoc (GHashTable* table, char* title, char* author, short year, char* fileName, char* pathDocs);

char* consultDoc (GHashTable* table, int id);

char* deleteDoc (GHashTable* table, int id);

char* lookupKeyword (GHashTable* table, int id, char* keyword);

char* lookupDocsWithKeyword (GHashTable* table, char* keyword, int nrProcesses);

void sendMessageToServer (enum ChildCommand cmd, Document* doc);

#endif