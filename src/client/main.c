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
    int fifoWrite = open ("tmp/fifoServer", O_WRONLY);
    if (fifoWrite == -1) {
        perror ("Server didn't open");
		return 1;
    }

    // create client FIFO (read only)
    pid_t pid = getpid();
    char pathFifo[20];
    createClientFifo (pathFifo, pid);

    // convert args to clientInfo
    ClientInfo* cInfo = convertInfo (argc, argv, pathFifo);
    if (!cInfo){
        close(fifoWrite);
        return 1;
    }
    
    // send client info to server
    int bytesWritten = write (fifoWrite, cInfo, sizeof (cInfo));
    if (bytesWritten < 0) {
        perror ("No bytes written to server");
        freeClientInfo(cInfo);
        close(fifoWrite);
        unlink(pathFifo);
        return 1;
    }
    
    // open pipe to get server's response
    int fifoRead = open (pathFifo, O_RDONLY);
    if (fifoRead == -1) {
        perror ("Client didn't open");
        freeClientInfo(cInfo);
        close(fifoWrite);
        unlink(pathFifo);
        return 1;
    }
    
    // read server's resonse
    char buf[BUFSIZ];
    int bytesRead = read (fifoRead, buf, BUFSIZ);
    if (bytesRead<0) perror ("No bytes read from server");

    // write to user
    write (STDOUT_FILENO, buf, bytesRead);

    // cleanup
    free(buf);
    freeClientInfo(cInfo);
    close(fifoRead);
    close(fifoWrite);
    unlink(pathFifo);

    return 0;
}

/* TO DO: 
    melhorar modularidade nos paths, principalmente entre cliente-servidor
    definir struct da resposta do servidor - atenção à modularidade
*/