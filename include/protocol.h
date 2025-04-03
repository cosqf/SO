#define SERVER_PATH "tmp/fifoServer"
#define MAX_RESPONSE_SIZE 1000

#ifndef PROTOCOL_H
#define PROTOCOL_H

typedef struct doc Document;

enum MessageType {
    CLIENT, 
    CHILD
};

enum ChildCommand {
    ADD,
    DELETE,
};

typedef struct clientrequest {
    char fifoPath[256];  
    char command[8][1024];  
} ClientRequest;

typedef struct childrequest {
    enum ChildCommand cmd;
    Document* doc;
} ChildRequest;

typedef struct message {
    enum MessageType type;
    union {
        ClientRequest clientReq;
        ChildRequest childReq;  
    } data;
} Message;

char** decodeClientInfo (ClientRequest cr);
Message* clientToMessage (ClientRequest* cr);
Message* childToMessage (ChildRequest* cr);

#endif