#include <stdio.h>
#include <stdlib.h>
#include <utils.h>

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

void* gettingValuesOfHashTable (GHashTable* table) { 
    int tableSize = g_hash_table_size (table); 
    GList* docsList = g_hash_table_get_values (table); 
    void** docs = malloc(tableSize * sizeof(void*));
    GList* node = docsList;
    for (int i = 0; i < tableSize && node; i++, node = node->next) docs[i] = node->data;
    g_list_free(docsList);

    return docs;
}