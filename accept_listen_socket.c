/**
 * Created by Alexandru Blinda.
 */
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <stdio.h>
#include <memory.h>
#include <pthread.h>

#include "accept_listen_socket.h"
#include "create_printable_address.h"
#include "handle_client_socket.h"

/**
 * Thread_control_block struct will hold the data that is thread related
 */
typedef struct thread_control_block {

    /**
    * Declaring variables
    * client holds the file descriptor of the accepted socket or -1 in case of error
    * client_address holds the client address
    * client_address_size holds the size of the client address
    */
    int client;
    struct sockaddr_in6 client_address;
    socklen_t client_address_size;

} thread_control_block_t;

static void *client_thread(void *data) {

    thread_control_block_t *tcb_pointer = (thread_control_block_t *) data;

    /**
    * buffer is passed to another method to create a printable address
    * the extra 32 bytes to the buffer are needed for extra words
    * printable will hold the actual printable string
    */
    char buffer[INET6_ADDRSTRLEN + 64];
    char *printable;

    /**
     * Create printable address from the client address
     */
    printable = create_printable_address( &(tcb_pointer->client_address), buffer, sizeof(buffer));

    (void) handle_client_socket(tcb_pointer->client, printable);

    free(printable);

    free(data);

    pthread_exit(0);
}


int accept_listen_socket(const int tcp6_socket) {

    /**
     * As long as we do not have an error, accept connections
     */
    while(1) {

        /**
         * Allocate space for a thread control block
         * Because it is malloc'd, it is thread safe
         */
        thread_control_block_t *tcb_pointer = malloc (sizeof (thread_control_block_t));

        if(tcb_pointer == NULL) {

            perror("Could not allocate memory");
            exit(1);
        }

        tcb_pointer->client_address_size = sizeof (tcb_pointer->client_address);

        tcb_pointer->client = accept(tcp6_socket,
                                     (struct sockaddr *) &(tcb_pointer->client_address),
                                     &(tcb_pointer->client_address_size));

        /**
         * If we have an error
         */
        if (tcb_pointer->client < 0) {

            perror("Accepting client error occurred");
            free(tcb_pointer);
            return -1;
        }
        else {

            pthread_t thread;
            pthread_attr_t pthread_attr; /** attributes for newly create thread */

            if (pthread_attr_init(&pthread_attr) !=0) {

                perror("Error creating initial thread attributes");
                free(tcb_pointer);
                return -1;
            }

            if(pthread_attr_setdetachstate(&pthread_attr, PTHREAD_CREATE_DETACHED) != 0) {

                perror("Setting thread attributes failed");
                free(tcb_pointer);
                return -1;
            }

            if(pthread_create(&thread, &pthread_attr, &client_thread, (void *)tcb_pointer)!= 0){

                perror("Thread creation failed");
                free(tcb_pointer);
                return -1;
            }
        }

    }

} // END OF accept_listen_socket FUNCTION
