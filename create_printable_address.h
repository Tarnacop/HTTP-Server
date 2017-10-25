/**
 * Created by Alexandru Blinda.
 */
#include <netinet/in.h>

char *create_printable_address(const struct sockaddr_in6 *const,
                                char *const,
                                const size_t);