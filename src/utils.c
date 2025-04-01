#include <stdio.h>
#include <stdlib.h>

int convertToNumber (char* string) {
    for (int i = 0; string[i]; i++) {
        if (string[i] == '\n') break;
        if (string[i] < '0' || string[i] > '9') {
            perror ("Cache size must be numbers");
            return -1;
        }
    }

    int number = atoi(string);
    if (number < 0) {
        perror ("Cache size must be positive");
        return -1;
    }
	return number;
}