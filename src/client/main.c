#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include "client/client.h"
/*
Os utilizadores devem utilizar um programa cliente para interagir com o serviço (i.e., com o programa servidor). Esta interação
permitirá que os utilizadores adicionem ou removam a indexação de um documento no serviço, e que efetuem pesquisas (interro-
gações) sobre os documentos indexados. De notar que o programa cliente apenas executa uma operação por invocação, ou seja,
não é um programa interativo (i.e., que vai lendo várias operações a partir do stdin)
*/

int main(int argc, char** argv) {
    // open server FIFO (write only)
    int fifoWrite = open (SERVER_PATH, O_WRONLY);
    if (fifoWrite == -1) {
        perror ("Server didn't open");
		return 1;
    }

    // create client FIFO (read only)
    pid_t pidNo = getpid();
    char pathFifo[20];

    createClientFifo (pathFifo, pidNo);

    // convert args to ClientRequest
    ClientRequest* cInfo = convertInfo (argc, argv, pathFifo);
    if (!cInfo){
        close(fifoWrite);
        return 1;
    }

    printf ("talking to server\n");

    // send client info to server
    int bytesWritten = write (fifoWrite, cInfo, sizeof (*cInfo));
    if (bytesWritten < 0) {
        perror ("No bytes written to server");
        freeClientRequest(cInfo);
        close(fifoWrite);
        unlink(pathFifo);
        return 1;
    }
    freeClientRequest(cInfo);

    printf ("opening client pipe\n");

    // open pipe to get server's response
    int fifoRead = open (pathFifo, O_RDONLY);
    if (fifoRead == -1) {
        perror ("Client didn't open");
        close(fifoWrite);
        unlink(pathFifo);
        return 1;
    }
    

    printf ("waiting for servers response\n");
 
    // read server's resonse
    char buf[MAX_RESPONSE_SIZE];
    int bytesRead = read (fifoRead, buf, MAX_RESPONSE_SIZE);
    if (bytesRead<0) perror ("No bytes read from server");

    // write to user
    write (STDOUT_FILENO, buf, bytesRead);

    // cleanup
    close(fifoRead);
    close(fifoWrite);
    unlink(pathFifo);

    return 0;
}
