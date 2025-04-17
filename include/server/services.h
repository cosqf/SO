#include <glib.h>
#include <protocol.h>
#include <persistence.h>

#ifndef SERVICES_H
#define SERVICES_H

ChildRequest* convertChildInfo (enum ChildCommand cmd, Document* doc);

char* closeServer ();

char* addDoc (char* title, char* author, short year, char* fileName, char* pathDocs);

char* consultDoc (DataStorage* ds, int id);

char* deleteDoc (DataStorage* ds, int id);

char* lookupKeyword (DataStorage* ds, int id, char* keyword);

char* lookupDocsWithKeyword (DataStorage* ds, char* keyword, int nrProcesses);

void sendMessageToServer (enum ChildCommand cmd, Document* doc);

#endif