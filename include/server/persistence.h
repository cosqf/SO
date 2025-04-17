#include <glib.h>
#include <protocol.h>

#ifndef PERSISTENCE_H
#define PERSISTENCE_H

#define DISKFILE_PATH "tmp/data.bin"

typedef struct {
    GHashTable* table; // stores key: int ids; value: Document*
    GQueue* LRUList; // LRU is the head; stores doc ids
    int cacheSize;
} Cache;

typedef struct dataStorage {
    Cache* cache;
    GHashTable* indexSet; // stores the ids of the docs stored in disk
} DataStorage;


DataStorage* initializeDataStorage(int maxCache);

void destroyDataInMemory (DataStorage* data);

void getIndex (DataStorage* data);

Document* lookupDoc (DataStorage* data, int id);

Document* getDocFromStorage(DataStorage* data, int id);

void addDocToCache (DataStorage* data, Document* doc);

void removeDocIndexing (DataStorage* data, int id);

GPtrArray* getAllDocuments(DataStorage* data);

#endif