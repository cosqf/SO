#include <stdio.h>
#include <protocol.h>
#include <stdlib.h>
#include <string.h>

char** decodeInfo(ClientRequest cr) {
    int argc = 0;
    
    for (int i = 0; i < 8 && cr.command[i][0] != '\0'; i++) {
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
