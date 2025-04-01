#include "protocol.h"

void createServerFifo ();

char** decodeInfo (ClientRequest cr);

char* processCommands(char **commands);