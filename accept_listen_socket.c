/**
 * Created by Alexandru Blinda.
 */
#include<netinet/in.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>
#include <memory.h>

#include "accept_listen_socket.h"
#include "create_printable_address.h"

int accept_listen_socket(const int tcp6_socket) {

    /**
     * Declaring variables
     * client holds the file descriptor of the accepted socket or -1 in case of error
     * client_address holds the client address
     * client_address_size holds the size of the client address
     * buffer is passed to another method to create a printable address
     * the extra 32 bytes to the buffer are needed for extra words
     * printable will hold the actual printable string
     */
    int client;

    struct sockaddr_in6 client_address;
    memset(&client_address, '\0', sizeof(client_address));

    socklen_t client_address_size = sizeof(client_address);

    char buffer[INET6_ADDRSTRLEN + 64];
    char *printable;

    /**
     * As long as we do not have an error, accept connections
     */
    while(client = accept(tcp6_socket,(struct sockaddr *) &client_address, &client_address_size) >= 0) {

        /**
         * Create printable address from the client address
         */
        printable = create_printable_address(&client_address, buffer, sizeof(buffer));

        printf("%s connected\n", printable);
        close(tcp6_socket);
        free(printable);
    }

    /**
     * If we have an error
     */
    if (client < 0) {

        perror("Accepting client error occurred");
        return -1;
    }

    return 0;
} // END OF accept_listen_socket FUNCTION
