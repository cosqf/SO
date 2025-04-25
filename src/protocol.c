#include <stdio.h>
#include <protocol.h>
#include <stdlib.h>
#include <string.h>


char** decodeClientInfo(ClientRequest cr, int* argcOut) {
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
    *argcOut = argc;
    commands[argc] = NULL; 
    return commands;
}


Message* clientToMessage (ClientRequest* cr) {
    Message* msg = calloc (1, sizeof (Message));
    if (!msg) {
        perror ("Malloc error");
        return NULL;
    }
    msg->type = CLIENT;
    msg->data.clientReq = *cr;
    
    return msg;
}

Message* childToMessage (ChildRequest* cr) {
    Message* msg = calloc (1, sizeof (Message));
    if (!msg) {
        perror ("Malloc error");
        return NULL;
    }
    msg->type = CHILD;
    msg->data.childReq = *cr;
    
    return msg;
}

int convertToNumber (char* string) {
    for (int i = 0; string[i]; i++) {
        if (string[i] == '\n') break;
        if (string[i] < '0' || string[i] > '9') {
            perror ("Must be numbers");
            return -1;
        }
    }

    int number = atoi(string);
    if (number < 0) {
        perror ("Number must be positive");
        return -1;
    }
	return number;
}