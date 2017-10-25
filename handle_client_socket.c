/**
 * Created by Alexandru Blinda.
 */
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "handle_client_socket.h"
#include "parse_http_request.h"

int handle_client_socket(const int client_socket, const char *const client_identification){

    /**
     * Declaring variables
     * the buffer is used
     */
    const int BUFFER_SIZE = 1024;
    char buffer[BUFFER_SIZE];
    memset(buffer, '\0', BUFFER_SIZE);
    size_t bytes;

    printf("%s connected.\n", client_identification);

    /**
     * Read the specified amount of bytes as long as there is no error (bytes > 0)
     * If there is an error, bytes will be negative and the loop will exit
     * We keep 1 space free to add a terminating null byte \0
     */

    bytes = read(client_socket, buffer, BUFFER_SIZE - 1);
    if(bytes < 0) {

        perror("Error reading from socket");
        close(client_socket);
        return -1;
    }

    buffer[bytes] = '\0';

    printf("Here is the message read: %s\n", buffer);

    parse_http_request(buffer, bytes, NULL);

    bytes = write(client_socket, buffer, bytes);

    if(bytes < 0) {

        perror("Error writing to socket");
        close(client_socket);
        return -1;
    }

    close(client_socket);

    printf("%s disconnected.\n", client_identification);


    return 0;
}