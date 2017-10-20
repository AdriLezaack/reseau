#include <stdlib.h> /* EXIT_X */
#include <stdio.h> /* fprintf */
#include <unistd.h> /* getopt */
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>



/* Resolve the resource name to an usable IPv6 address
 * @address: The name to resolve
 * @rval: Where the resulting IPv6 address descriptor should be stored
 * @return: NULL if it succeeded, or a pointer towards
 *          a string describing the error if any.
 *          (const char* means the caller cannot modify or free the return value,
 *           so do not use malloc!)
 */
const char * real_address(const char *address, struct sockaddr_in6 *rval){
    int errcode;
    struct addrinfo hints, *res;
    
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_flags = AI_PASSIVE | AI_V4MAPPED;
    hints.ai_family = AF_INET6; // Adresse IPV6
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = 17; // lower byte UDP
    
    if((errcode = getaddrinfo(address, NULL, &hints, &res)))
        return gai_strerror(errcode);
    
    memcpy(rval, res->ai_addr, res->ai_addrlen);
    
    freeaddrinfo(res);
    return NULL;
}
