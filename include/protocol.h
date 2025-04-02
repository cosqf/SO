#define SERVER_PATH "tmp/fifoServer"
#define MAX_RESPONSE_SIZE 1000

typedef struct clientrequest {
    char fifoPath[256];  
    char command[8][1024];  
} ClientRequest;

char** decodeInfo (ClientRequest cr);