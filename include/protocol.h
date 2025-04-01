#define SERVER_PATH "tmp/fifoServer"

typedef struct clientrequest {
    char fifoPath[256];  
    char command[8][1024];  
} ClientRequest;
