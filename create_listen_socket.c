/**
 * Created by Alexandru Blinda.
 */
#include<sys/socket.h>
#include<netinet/in.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

#include "create_listen_socket.h"

/**
 * Method to create a listening socket bound to the given port number
 */
int create_listen_socket(const int port_number) {

    printf("Binding to port %d\n", port_number);

    /**
     * Structure that holds all the address details for an IPv6 protocol implementation
     */
    struct sockaddr_in6 address;

    /**
     * We will initialise the address memory with the 0 byte
     */
    memset(&address, '\0', sizeof(address));

    /**
     * Family is always set to AF_INET6
     * The 128-bit IPv6 address; bind to any
     * The port number is the number passed as argument, converted from host order to network order
     */
    address.sin6_family = AF_INET6;
    address.sin6_addr = in6addr_any;
    address.sin6_port = htons(port_number);

    /**
     * Try and get a TCP IPv6 socket for listening
     */
    int tcp6_socket = socket(PF_INET6, SOCK_STREAM, 0);

    /**
     * If the value returned is negative, there is an error
     */
    if(tcp6_socket < 0) {

        perror("Socket error occurred");
        return -1;
    }

    /**
     * We are trying to bind the socket to our address
     * If it fails, an error occurred
     */
    if( bind(tcp6_socket, (struct sockaddr *) &address, sizeof(address)) != 0 ) {

        perror("Binding error occurred");
        return -1;
    }

    /**
     * We are trying to start listening to the newly created socket
     * If it fails, an error occurred
     */
    if( listen(tcp6_socket, 5) != 0 ) {

        perror("Listen error occurred");
        return -1;
    }

    return tcp6_socket;
} // END OF create_listen_socket FUNCTION