#include <glib.h>
#include <persistence.h>

#ifndef SERVER_H
#define SERVER_H

void createServerFifo ();

void notifyChildExit();

char* processCommands(char **commands, int noCommands, char* path, int cacheSize, DataStorage*);

#endif