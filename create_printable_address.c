/**
 * Created by Alexandru Blinda.
 */

#include <stdlib.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <memory.h>

#include "create_printable_address.h"

char* create_printable_address(const struct sockaddr_in6 *const address,
                                char *const buffer,
                                const size_t buffer_size) {

    /**
     * Create a buffer to hold the printable address
     */
    char printable[INET6_ADDRSTRLEN];

    /**
     * inet_ntop is used to convert IPv4 and IPv6 from binary to text
     * the first argument passed is the family of the address
     * the second argument passed points to the buffer holding the address in network byte order
     * the third argument points to a buffer where the text will be stored
     * the fourth argument specifies the size of the buffer which must be big enough to hold the text
     * If inet_ntop is successfull, it will return the pointer to the buffer that holds the text (in our case printable)
     * Otherwise it returns NULL
     * localhost will be printed as ::1 because localhost is 0.0.0.1
     */
    if(inet_ntop(address->sin6_family, &(address->sin6_addr),
                 printable, sizeof(printable)) == printable) {

        snprintf(buffer, buffer_size,
                 "Client with address %s and port %d", printable, ntohs(address->sin6_port));
    } else {

        perror("Could not parse the address.");
        snprintf(buffer, buffer_size, "Address cannot be parsed");
    }

    return strdup(buffer);
} // END OF create_printable_address FUNCTION