#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#include <server/server.h>
#include <server/services.h>

void createServerFifo () {
    if (mkfifo(SERVER_PATH, 0666) == -1) {
    perror("mkfifo failed");
    exit(EXIT_FAILURE);
    }
}

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

char* processCommands(char **commands, char* pathDocs, int cacheSize) {
    if (!commands || !commands[0]) return "Invalid command";
    if (strcmp(commands[0], "-f") == 0) return NULL;
    else if (strcmp(commands[0], "-t") == 0) { // for debugging
        static char result[256]; 
        snprintf(result, sizeof(result), "test %s\n", commands[1]); 
        return result;
        }
    else if (strcmp(commands[0], "-a") == 0) {
        int year = convertToNumber (commands[3]);
        return addDoc (commands[1], commands[2], year, commands[4], pathDocs); // confirmar dps a ordem
    }
    else if (strcmp(commands[0], "-c") == 0){
        int id = convertToNumber (commands[1]);
        return consultDoc (id);
    }
    else if (strcmp(commands[0], "-d") == 0){
        int id = convertToNumber (commands[1]);
        return deleteDoc (id);
    } 
    
    else return "Yippee\n"; 
}
