/**
 * Created by Alexandru Blinda.
 */
#include <stdlib.h>
#include <sys/socket.h>
#include <stdio.h>
#include <pthread.h>

#include "accept_listen_socket.h"
#include "handle_client_socket.h"

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

static void *client_thread(void *data) {

    thread_control_block_t *tcb_pointer = data;

    (void) handle_client_socket(tcb_pointer->client, &lock);

    free(data);
    pthread_exit(EXIT_SUCCESS);
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
