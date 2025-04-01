#include <server/server.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>

/*
O programa servidor é responsável por registar meta-informação sobre cada documento (p.ex, identificador único,
título, ano, autor, localização), permitindo também um conjunto de interrogações relativamente a esta meta-informação e ao
conteúdo dos documentos
*/

    // $ ./dserver document_folder cache_size
int main(int argc, char **argv) {
    if (argc != 3) {
        perror ("Wrong number of arguments");
        exit (EXIT_FAILURE);
    }

    printf ("creating server fifo\n");
    // create server FIFO (read only)
    createServerFifo ();
    printf ("waiting for client\n");
    int fifoRead = open (SERVER_PATH, O_RDONLY);
    if (fifoRead == -1) {
        perror ("Server didn't open");
		return 1;
    }

    ClientRequest buf;
    int status;
    while (1) {
        int bytesRead = read (fifoRead, &buf, sizeof (ClientRequest));
        if (bytesRead <=0) continue;

        printf ("client read\n");
        pid_t pid = fork();
        if (pid == 0) {
            int fifoWrite = open (buf.fifoPath, O_WRONLY);
            if (fifoWrite == -1) {
                perror ("Failed to open client FIFO");
            }

            char** commands = decodeInfo(buf);
            char* reply = processCommands(commands);

            if (reply) write (fifoWrite, reply, strlen(reply) + 1);

            close (fifoWrite);
            for (int i = 0; i < 8 && commands[i][0] != '\0'; i++) {
                free(commands[i]);
            }
            free(commands);
            if (!reply) _exit (1);
            else _exit(0);
        }
        wait(&status);
        if (WEXITSTATUS(status) == 1) {  
            printf("Shutting down server...\n");
            break;
        }
    }
    close (fifoRead);
    unlink (SERVER_PATH);
    return 0;
}


// TODO: fix exiting the server