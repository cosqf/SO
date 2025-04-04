#include <glib.h>
#include <protocol.h>

#ifndef SERVICES_H
#define SERVICES_H

int getDocumentId (Document* doc);

ChildRequest* convertChildInfo (enum ChildCommand cmd, Document* doc);

char* addDoc (GHashTable* table, char* title, char* author, short year, char* fileName, char* pathDocs);

char* consultDoc (GHashTable* table, int id);

char* deleteDoc (GHashTable* table, int id);

#endif