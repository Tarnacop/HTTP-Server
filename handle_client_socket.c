/**
 * Created by Andreea Gheorghe on 16/10/2017.
 */
#include <stdio.h>
#include <unistd.h>
#include <memory.h>

#include "handle_client_socket.h"

#define BUFFER_SIZE 1024

int handle_client_socket(const int tcp6_socket, const char *const client_identification){

    /**
     * Declaring variables
     * the buffer is used
     */
    char buffer[BUFFER_SIZE];
    size_t bytes;

    printf("%s connected.\n", client_identification);

    /**
     * Read the specified amount of bytes as long as there is no error (bytes > 0)
     * If there is an error, bytes will be negative and the loop will exit
     * We keep 1 space free to add a terminating null byte \0
     */
    while( (bytes = read(tcp6_socket, buffer, BUFFER_SIZE - 1)) > 0) {

        if (write(tcp6_socket, buffer, bytes) != bytes) {

            perror("Error writing");
            return -1;
        }

        buffer[bytes] = '\0';

        if (bytes >= 1 && buffer[bytes - 1] == '\n') {

            if (bytes >= 2 && buffer[bytes - 2] == '\r') {

                strcpy(buffer + bytes - 2, "..");
            } else {

                strcpy(buffer + bytes - 1, ".");
            }
        }

#if (__SIZE_WIDTH__ == 64 || __SIZEOF_POINTER__ == 8)
        printf("echoed %ld bytes back to %s, \"%s\"\n", bytes, client_identification, buffer);
#else
        printf ("echoed %d bytes back to %s, \"%s\"\n", bytes, tag, buffer);
#endif
    }

    /* bytes == 0: orderly close; bytes < 0: something went wrong */
    if (bytes != 0) {

        perror("read");
        return -1;
    }

    printf("%s disconnected.\n", client_identification);

    close(tcp6_socket);

    return 0;
}
