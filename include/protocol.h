#define SERVER_PATH "tmp/fifoServer"

#ifndef PROTOCOL_H
#define PROTOCOL_H

typedef struct doc {
    int id;
    char title[200];
    char authors[200];
    char path[64];
    short year;
} Document;

enum MessageType {
    CLIENT, 
    CHILD
};

enum ChildCommand {
    ADD,
    DELETE,
    LOOKUP,
    CHILD_EXIT,
    EXIT
};

typedef struct clientrequest {
    char fifoPath[256];  
    char command[5][64];
    int noCommand;
} ClientRequest;

typedef struct childrequest {
    enum ChildCommand cmd;
    Document doc;
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