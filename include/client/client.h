#include <protocol.h>

void createClientFifo (char path[], int number);

ClientRequest* convertInfo (int argc, char** args, char path[]);

void freeClientRequest(ClientRequest* ci);

#define CLIENT_FIFO "tmp/fifoClient"