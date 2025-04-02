#include <protocol.h>
#define CLIENT_FIFO "tmp/fifoClient"

void createClientFifo (char path[], int number);

ClientRequest* convertInfo (int argc, char** args, char path[]);

void freeClientRequest(ClientRequest* ci);

