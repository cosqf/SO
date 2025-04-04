#include <protocol.h>
#define CLIENT_FIFO "tmp/fifoClient"

#ifndef CLIENT_H
#define CLIENT_H

void createClientFifo (char path[], int number);

ClientRequest* convertInfo (int argc, char** args, char path[]);

void freeClientRequest(ClientRequest* ci);

#endif