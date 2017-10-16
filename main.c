/**
 * Created by Alexandru Blinda.
 */
#include <stdio.h>
#include <stdlib.h>

#include "create_listen_socket.h"
#include "accept_listen_socket.h"

const char *server_name = "SERVER";

int main(int argc, char **argv) {

    int port_number, tcp6_socket;
    char *end_port_number_string;

    /**
     * The port number to bind to must be passed as an argument when running the server
     */
    if(argc != 2) {

        fprintf(stderr, "%s: You must pass the port as an argument.\n", server_name);
        exit(1);
    }

    /**
     * Convert the port number from the passed string to int
     * The first argument is the string
     * The second argument is a pointer that points to the first character after the number (used for checks)
     * The third argument is the base (in our case base 10)
     */
    port_number = strtol(*(argv + 1), &end_port_number_string, 10);

    /**
     * If the value that we read is not a number, return an error
     */
    if(*end_port_number_string != '\0') {

        fprintf(stderr, "%s: %s is not a number!\n", server_name, *argv);
        exit(1);
    }

    /**
     * Port number cannot be less than 1024 because you need root access
     * Port number cannot be bigger than 65535 because it is not valid
     */
    if(port_number < 1024 || port_number > 65535) {

        fprintf(stderr, "%s: %d is not a valid port. A valid port must be between 1024 and 65535.\n", server_name, port_number);
        exit(1);
    }

    /**
     * Try and create a listening socket using the port_number given
     * If result is negative, an error occurred
     */
    tcp6_socket = create_listen_socket(port_number);
    if(tcp6_socket < 0) {

        fprintf(stderr, "%s: Could not create a listening socket.\n", server_name);
        exit(1);
    }

    if(accept_listen_socket(tcp6_socket) < 0) {

        fprintf(stderr, "%s: Cannot process listen socket.\n", server_name);
        exit(1);
    }


    exit(0);
} // END OF main FUNCTION