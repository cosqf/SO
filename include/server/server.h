#include <glib.h>
#include <persistence.h>

#ifndef SERVER_H
#define SERVER_H

/** @brief Creates a FIFO which the server will read for messages.  */
void createServerFifo ();

/**
 * @brief Notifies the server that the child process has completed its execution.
 *
 * This function constructs a CHILD_EXIT message containing the current process ID,
 * and sends it to the server through the designated FIFO. The server should then handle
 * cleanup and wait for the child process to exit.
 *
 * @note This should be called by a child process right before terminating (via _exit()).
 */
void notifyChildExit();

/**
 * @brief Processes client commands and performs the corresponding document operations.
 *
 * This function interprets and dispatches the received command to the appropriate document-related operation:
 * - "-f": Instructs the server to close.
 * - "-a": Adds a document using provided title, author, year, and file path.
 * - "-c": Consults a document by its ID.
 * - "-d": Deletes a document by its ID.
 * - "-l": Looks up a keyword within a document by ID.
 * - "-s": Searches for documents containing a keyword, optionally specifying the number of processes to use.
 *
 * @param commands     An array of parsed command strings.
 * @param noCommands   The number of command arguments received.
 * @param pathDocs     The base path to document files.
 * @param ds           Pointer to the data storage structure managing cache and index.
 * 
 * @return A dynamically allocated string containing the message to be sent to the Client.
 */
char* processCommands(char **commands, int noCommands, char* path, DataStorage*);

#endif