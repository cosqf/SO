#include <stdlib.h>
/*
O programa servidor é responsável por registar meta-informação sobre cada documento (p.ex, identificador único,
título, ano, autor, localização), permitindo também um conjunto de interrogações relativamente a esta meta-informação e ao
conteúdo dos documentos
*/

void createServerFifo () {
    if (mkfifo("tmp/fifoServer", 0666) == -1) {
    perror("mkfifo failed");
    exit(EXIT_FAILURE);
    }
}


int main() {
    createServerFifo ();
    return 0;
}