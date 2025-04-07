#include <stdio.h>
#include <protocol.h>
#include <stdlib.h>
#include <string.h>
#include <protocol.h>

int getDocumentId (Document* doc) {
    return doc->id;
}

char** decodeClientInfo(ClientRequest cr) {
    int argc = 0;
    
    for (int i = 0; i < 5 && cr.command[i][0] != '\0'; i++) {
        argc++;
    }

    char** commands = malloc((argc + 1) * sizeof(char*));
    if (!commands) {
        perror("Malloc error");
        return NULL;
    }

    for (int i = 0; i < argc; i++) {
        commands[i] = strdup(cr.command[i]);  
        
        if (!commands[i]) {
            perror("Malloc error");
            for (int j = 0; j < i; j++) {
                free(commands[j]);
            }
            free(commands);
            return NULL;
        }
    }
    commands[argc] = NULL; 
    return commands;
}


Message* clientToMessage (ClientRequest* cr) {
    Message* msg = malloc (sizeof (Message));
    if (!msg) {
        perror ("Malloc error");
        return NULL;
    }
    memset(msg, 0, sizeof(Message)); // avoid using uninitialized memory
    msg->type = CLIENT;
    msg->data.clientReq = *cr;
    
    return msg;
}

Message* childToMessage (ChildRequest* cr) {
    Message* msg = malloc (sizeof (Message));
    if (!msg) {
        perror ("Malloc error");
        return NULL;
    }
    memset(msg, 0, sizeof(Message)); // avoid using uninitialized memory
    msg->type = CHILD;
    msg->data.childReq = *cr;
    
    return msg;
}
