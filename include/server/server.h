#include <glib.h>

#ifndef SERVER_H
#define SERVER_H

void createServerFifo ();

char* processCommands(char **commands, char* path, int cacheSize, GHashTable*);

#endif