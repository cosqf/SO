#include <glib.h>

void createServerFifo ();

char* processCommands(char **commands, char* path, int cacheSize, GHashTable*);