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