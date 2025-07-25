#include <glib.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include <memoryManager.h>
#include <protocol.h>

/**
 * @brief Populates the index set with document IDs from the disk file.
 *
 * Opens the file storing the documents (creating it if it doesn't exist), 
 * reads all stored documents, and populates the indexSet with their IDs.
 *
 * @param data Pointer to the DataStorage structure where the indexSet is stored.
 */
void getIndex (DataStorage* data, int cacheSize) {
    GHashTable* indexSet = data->indexSet;
    int fd = open (DISKFILE_PATH, O_RDONLY | O_CREAT, 0644);
    if (fd == -1) {
        perror ("Failed to open disk data file");
        return;
    }
    int i = 0;
    Document docRead;
    while (read(fd, &docRead, sizeof(Document)) == sizeof(Document)) {
        g_hash_table_add(indexSet, GINT_TO_POINTER(docRead.id));
        if (i++ < cacheSize) {
            Document* doc = malloc (sizeof (Document));
            *doc = docRead;
            addDocToCache (data, doc);
        }
    }
    close (fd);
}

DataStorage* initializeDataStorage (int maxCache) {
    Cache* cache = malloc (sizeof (Cache));
    if (!cache) {
        perror("Malloc error");
        return NULL;
    }
    DataStorage* ds = malloc(sizeof(DataStorage));
    if (!ds) {
        perror ("Malloc error");
        free (cache);
        return NULL;
     }

    cache->table = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, free);
    cache->LRUList = g_queue_new();
    cache->cacheSize = maxCache;

    GHashTable* index = g_hash_table_new(g_direct_hash, g_direct_equal);

    ds->cache = cache;
    ds->indexSet = index;

    getIndex(ds, maxCache);
    return ds;
}


/**
 * @brief Reads a Document from disk by its ID.
 * 
 * @param id The document ID to search for.
 * @return A pointer to a dynamically allocated Document with the matching ID, or NULL if not found or on error.
 */
Document* readDocFromFile (int id) {
    int fd = open (DISKFILE_PATH, O_RDONLY);
    if (fd == -1) {
        perror ("Failed to open disk data file");
        return NULL;
    }
    Document docRead;
    while (read(fd, &docRead, sizeof(Document)) == sizeof(Document)) {
        if (docRead.id == id) {
            Document* doc = malloc (sizeof (Document));
            if (!doc) {
                perror("Malloc error");
                close (fd);
                return NULL;
            }
            *doc = docRead;
            close (fd);
            return doc;
        }
    }
    close (fd); 
    return NULL;
}

const Document* lookupDoc (DataStorage* data, int id) {
    gpointer idp = GUINT_TO_POINTER (id);
    Document* doc = g_hash_table_lookup (data->cache->table, idp); 
    if (doc) {  // doc is in cache
        write (STDOUT_FILENO, "Cache hit\n", sizeof ("Cache hit\n"));
    }
    else {
        write (STDOUT_FILENO, "Cache miss\n", sizeof ("Cache miss\n"));
        if (!g_hash_table_contains (data->indexSet, idp)) return NULL; // doc isnt stored

        doc = readDocFromFile (id);
        if (!doc) return NULL;
    }
    return doc;
}


void addDocToCache (DataStorage* data, Document* doc) {
    gpointer idp = GUINT_TO_POINTER(doc->id);
    Cache* cache = data->cache;
    
    if (cache->cacheSize == 0) { // write directly to disk
        if (!g_hash_table_contains(data->indexSet, idp)) {
            int fd = open(DISKFILE_PATH, O_CREAT | O_WRONLY | O_APPEND, 0644);
            if (fd == -1) perror("Failed to open disk data file");
            else {
                write(fd, doc, sizeof(Document));
                g_hash_table_add(data->indexSet, idp);
                close(fd);
            }
        }
        free (doc);
        return;
    }

    if (g_hash_table_lookup(cache->table, idp)) { // refresh LRU
        g_queue_remove(cache->LRUList, idp);
        g_queue_push_tail(cache->LRUList, idp);
        return;
    }

    if (g_queue_get_length(cache->LRUList) >= cache->cacheSize) { // if its over capacity, evict the LRU doc to disk
        gpointer lruId = g_queue_pop_head(cache->LRUList);
        Document* evictedDoc = g_hash_table_lookup(cache->table, lruId);
        
        if (!g_hash_table_contains(data->indexSet, lruId)) { // write only if not already stored
            int fd = open(DISKFILE_PATH, O_CREAT | O_WRONLY | O_APPEND, 0644);
            if (fd == -1) perror("Failed to open disk data file");
            else {
                write(fd, evictedDoc, sizeof(Document));
                g_hash_table_add(data->indexSet, lruId);
                close(fd);
            }
        }

        g_hash_table_remove(cache->table, lruId);
    }

    // insert new doc
    g_hash_table_insert(cache->table, idp, doc);
    g_queue_push_tail(cache->LRUList, idp);
}

void removeDocIndexing (DataStorage* data, int id) {
    gpointer idp = GUINT_TO_POINTER (id);
    g_hash_table_remove(data->cache->table, idp);
    g_queue_remove(data->cache->LRUList, idp);
    
    g_hash_table_remove(data->indexSet, idp);
}


GPtrArray* getAllDocuments(DataStorage* data) {
    GPtrArray* documents = g_ptr_array_new_with_free_func(free); 
    int fd = open (DISKFILE_PATH, O_RDONLY);
    if (fd == -1) {
        perror ("Failed to open disk data file");
        g_ptr_array_free(documents, TRUE);
        return NULL;
    } 
    Document doc;
    while (read (fd, &doc, sizeof(Document)) == sizeof (Document)) { // first add all in disk
        if (g_hash_table_contains (data->indexSet, GUINT_TO_POINTER (doc.id))) {
            Document* copy = malloc(sizeof(Document));
            if (!copy) {
                perror("Malloc error");
                continue;
            }
            *copy = doc;
            g_ptr_array_add(documents, copy);
        }
    }
    close (fd);

    GHashTableIter iter;
    gpointer idp;

    g_hash_table_iter_init(&iter, data->cache->table); // iterate cache and add only those not saved in disk
    while (g_hash_table_iter_next(&iter, &idp, NULL)) {
        Document* docCache = g_hash_table_lookup(data->cache->table, idp);
        if (g_hash_table_contains (data->indexSet, idp)) continue;

        Document* copy = malloc(sizeof(Document));
        if (!copy) {
            perror("Malloc error");
            continue;
        }
        *copy = *docCache; // deep copy
        g_ptr_array_add(documents, copy);
    }
    if (documents->len == 0) {
        g_ptr_array_free(documents, TRUE);
        return NULL;
    }
    return documents;
}

// CLEANING FUNCTIONS

/**
 * Will clean the disk file, removing the files that were deleted during the program */
void rebuildDiskFile (DataStorage* data) {
    int oldFD = open(DISKFILE_PATH, O_RDONLY);
    if (oldFD == -1) {
        perror("Failed to open old disk file");
        return;
    }

    int newFD = open("tmp/data_tmp.bin", O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (newFD == -1) {
        perror("Failed to open temp disk file");
        close(oldFD);
        return;
    }

    Document doc;
    while (read (oldFD, &doc, sizeof (Document)) == sizeof(Document)) {
        gpointer idp = GUINT_TO_POINTER(doc.id);
        if (g_hash_table_contains(data->indexSet, idp)) write(newFD, &doc, sizeof(Document));
    }

    close(oldFD);
    close(newFD);

    rename("tmp/data_tmp.bin", DISKFILE_PATH);
}

/**
 * Save all cache data to disk */
void saveCacheData (GHashTable* table, GHashTable* index) {
    int fd = open (DISKFILE_PATH, O_CREAT | O_WRONLY | O_APPEND, 0644);
    if (fd == -1) {
        perror ("Failed to open disk data file");
        return;
    }
    GHashTableIter iter;
    gpointer key, value;

    g_hash_table_iter_init (&iter, table);
    while (g_hash_table_iter_next (&iter, &key, &value)){
        if (g_hash_table_contains (index, key)) continue; // verify if doc is already on disk
        Document* doc = value;
        write (fd, doc, sizeof (Document));
    }
    close (fd);
}

void destroyDataInMemory (DataStorage* data) {
    if (!data) return;

    rebuildDiskFile (data);

    if (data->cache) {
        if (data->cache->table) {
                saveCacheData (data->cache->table, data->indexSet);
                g_hash_table_destroy (data->cache->table);
        }
        if (data->cache->LRUList) g_queue_free (data->cache->LRUList);
        free(data->cache);
    }
    if (data->indexSet) g_hash_table_destroy (data->indexSet);
    
    free (data);
}
