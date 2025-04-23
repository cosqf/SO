#include <protocol.h>
#define CLIENT_FIFO "tmp/fifoClient"

#ifndef CLIENT_H
#define CLIENT_H

/**
 * @brief Creates a Client FIFO, which the client will use to read the server's response.
 * 
 * @param path A char array where the full FIFO path will be written.
 * @param number The pid of the client processor, used to generate a distinct FIFO filename, 
 */
void createClientFifo(char path[], int number);

/**
 * @brief Converts the arguments from the client request into a ClientRequest struct. */
ClientRequest* convertInfo (int argc, char** args, char path[]);

/** 
 * @brief Frees a ClientRequest struct. */
void freeClientRequest(ClientRequest* ci);

#endif