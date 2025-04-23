#define SERVER_PATH "tmp/fifoServer"

#ifndef PROTOCOL_H
#define PROTOCOL_H

/**
 * @brief Enum for identifying message origin. */
enum MessageType {
    CLIENT, 
    CHILD
};
/**
 * @brief Enum for different commands that a child process can send. */
enum ChildCommand {
    ADD,
    DELETE,
    LOOKUP,
    CHILD_EXIT,
    EXIT
};

/**
 * @brief Document structure representing metadata about a document. */
typedef struct doc {
    int id;
    char title[200];
    char authors[200];
    char path[64];
    short year;
} Document;

/**
 * @brief Structure used to represent a client's request. */
typedef struct clientrequest {
    char fifoPath[256];      ///< FIFO path for writing back the response.
    char command[5][64];     ///< Up to 5 command parts (command + args).
    int noCommand;           ///< Actual number of command parts.
} ClientRequest;

/**
 * @brief Structure used for communication between child processes and the server. */
typedef struct childrequest {
    enum ChildCommand cmd;
    Document doc;
} ChildRequest;

/**
 * @brief Union-based message structure that can carry a ClientRequest or ChildRequest. 
 * It is the format the server reads from the fifo. */
typedef struct message {
    enum MessageType type;
    union {
        ClientRequest clientReq;
        ChildRequest childReq;  
    } data;
} Message;

/** 
 * @brief Decodes a ClintRequest into an array of commands. */
char** decodeClientInfo(ClientRequest cr, int* argcOut);

/** 
 * @brief Wraps a ClientRequest into a Message to the Server */
Message* clientToMessage (ClientRequest* cr);

/** 
 * @brief Wraps a ChildRequest into a Message to the Server */
Message* childToMessage (ChildRequest* cr);

#endif