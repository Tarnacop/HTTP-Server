/**
 * Created by Alexandru Blinda.
 */
#include <netinet/in.h>
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

static void *client_thread(void*);

int accept_listen_socket(const int);
