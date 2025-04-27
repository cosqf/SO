#include <glib.h>
#include <protocol.h>

#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#define DISKFILE_PATH "tmp/data.bin"

/**
 * @brief Struct for cache related things.
 * The cache works with a LRU policy, which when full chooses to save on disk and kick from the cache the Document that was used least recently.
 * The document to be kicked (LRU) is stored at the head of the queue
 */
typedef struct {
    GHashTable* table; ///< Hashtable which holds the Documents, the IDs as keys and Document* as values 
    GQueue* LRUList; ///< Queue that holds the IDs of the docs stored in cache in order by how recent they were used. 
    int cacheSize; ///< The max number of Documents the cache holds. 
} Cache;


/**
 * @brief Struct for the two kinds of data storage, the cache and a set for the documents in disk.
 */
typedef struct dataStorage {
    Cache* cache;
    GHashTable* indexSet; ///< Set (hashtable that only stores keys) that stores the Ids of Documents stored in disk 
} DataStorage;


/**
 * @brief Initializes the data storage system, including cache and index.
 *
 * Allocates memory for the Cache and DataStorage structures, sets up the in-memory cache
 * and least-recently-used (LRU) queue, and populates the index from documents stored on disk.
 */
DataStorage* initializeDataStorage(int maxCache);


/**
 * @brief Retrieves a document by ID from the cache or disk.
 *
 * This function attempts to retrieve a document from the cache. If the document is found,
 * it updates its position in the LRU (Least Recently Used) queue to mark it as recently used.
 * If the document is not in the cache but exists on disk (as determined by the index set),
 * it is read from disk and inserted into the cache. If the document does not exist at all,
 * NULL is returned.
 */
Document* lookupDoc (DataStorage* data, int id);


/**
 * @brief Adds a document to the in-memory cache, managing LRU eviction.
 *
 * If the document is already in the cache, it updates its position in the LRU queue.
 * If the cache is at capacity, the least recently used document is evicted.
 * Evicted documents are written to disk only if they haven't been previously saved.
 */
void addDocToCache (DataStorage* data, Document* doc);


/**
 * @brief Removes a doc from storage, be it cache or the index set, if it exists. 
 * @note It does not remove the document from the file in disk and it must be instead cleaned later.
 */
void removeDocIndexing (DataStorage* data, int id);


/**
 * @brief Retrieves all documents currently known to the system.
 *
 * This function returns all documents, both from the cache and from disk.
 * It dynamically allocates memory for documents read from disk, which will
 * be automatically freed when the returned GPtrArray is freed.
 *
 * @param data Pointer to the DataStorage structure.
 * @return A GPtrArray of Document* elements. Each pointer is owned by the array and will be freed.
 */
GPtrArray* getAllDocuments(DataStorage* data);


/**
 * @brief Cleans all the (volatile) data related to Document storage */
void destroyDataInMemory (DataStorage* data);

#endif