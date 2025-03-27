typedef struct clientinfo ClientInfo;

void createClientFifo (char *path, int number);

ClientInfo* convertInfo (int argc, char** args, char path[]);

void freeClientInfo(ClientInfo* ci);