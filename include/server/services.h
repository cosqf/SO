#include <glib.h>
#include <protocol.h>
#include <persistence.h>

#ifndef SERVICES_H
#define SERVICES_H

/**
 * @brief Sends a shutdown request to the server.
 *
 * It's triggered by "dclient -f"
 *
 * @return A dynamically allocated string indicating the server has closed.
 */
char* closeServer ();

/**
 * @brief Adds a new document to the system.
 *
 * Constructs a Document using the provided metadata and file path, then sends an ADD request to the server.
 * The ID used is calculate by the time and pid of the processor. 
 * It's triggered by "dclient -a "title" "authors" "year" "path""
 *
 * @param title     The title of the document.
 * @param author    The author(s) of the document.
 * @param year      The publication year of the document.
 * @param fileName  The name of the file containing the document.
 * @param pathDocs  The directory path where documents are stored.
 * 
 * @return A dynamically allocated message indicating success or failure.
 */
char* addDoc (char* title, char* author, short year, char* fileName, char* pathDocs);

/**
 * @brief Retrieves and displays information for a document by ID.
 *
 * This function searches for a document using its ID. If found, it sends a LOOKUP
 * command to the server (to update LRU cache ordering), and returns a formatted string
 * with document metadata.
 * It's triggered by "dclient -c [key]".
 *
 * @param ds  Pointer to the DataStorage instance.
 * @param id  ID of the document to consult.
 * 
 * @return A dynamically allocated string with document information or an error message.
 */
char* consultDoc (DataStorage* ds, int id);

/**
 * @brief Deletes a document from the system by ID.
 *
 * Searches for a document using its ID. If found, it sends a DELETE command to the server,
 * removes the document from indexing, and returns a confirmation message.
 * It's triggered by "dclient -d [key].

 * @param ds  Pointer to the DataStorage instance.
 * @param id  ID of the document to delete.
 * 
 * @return A dynamically allocated status message.
 */
char* deleteDoc (DataStorage* ds, int id);

/**
 * @brief Looks up a keyword in a document by ID and returns a message with the number of times it appears.
 *
 * Retrieves a document from the cache or disk using its ID, sends a LOOKUP signal
 * to update cache access, and counts how many lines contain a specific keyword.
 * It's triggered by "dclient -l "key" "keyword"".
 *
 * @param ds      Pointer to the DataStorage instance.
 * @param id      The ID of the document to search in.
 * @param keyword The keyword to search for.
 *
 * @return A dynamically allocated message string with the result. 
 */
char* lookupKeyword (DataStorage* ds, int id, char* keyword);

/**
 * @brief Performs a concurrent search for documents containing the keyword.
 *
 * Launches multiple processes to scan the document set in parallel, then 
 * returns a list of document IDs that contain the keyword.
 * It's triggered by "dclient -s "keyword" [nr_processes]".
 *
 * @param ds           Pointer to DataStorage.
 * @param keyword      Keyword to search.
 * @param nrProcesses  Number of child processes to use.
 *
 * @return Dynamically allocated string listing matching document IDs.
 *         Must be freed by the caller.
 */
char* lookupDocsWithKeyword (DataStorage* ds, char* keyword, int nrProcesses);

#endif