/**
 * Created by Alexandru Blinda.
 */
#include <netinet/in.h>

char *create_printable_address(const struct sockaddr_in6 *const address,
                                char *const buffer,
                                const size_t buffer_size);