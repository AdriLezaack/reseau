#include <stdlib.h> /* EXIT_X */
#include <stdio.h> /* fprintf */
#include <unistd.h> /* getopt */
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>

/* Creates a socket and initialize it
 * @source_addr: if !NULL, the source address that should be bound to this socket
 * @src_port: if >0, the port on which the socket is listening
 * @dest_addr: if !NULL, the destination address to which the socket should send data
 * @dst_port: if >0, the destination port to which the socket should be connected
 * @return: a file descriptor number representing the socket,
 *         or -1 in case of error (explanation will be printed on stderr)
 */
int create_socket(struct sockaddr_in6 *source_addr, int src_port, struct sockaddr_in6 *dest_addr, int dst_port){
    int sockfd;
    
    if((source_addr == NULL) == (dest_addr == NULL))
        return -1;
    
    sockfd = socket(PF_INET6, SOCK_DGRAM, 17);
    
    if(sockfd == -1) {
        perror("Error in socket()");
        return -1;
    }
    
    if(source_addr != NULL) {
        source_addr->sin6_port = htons(src_port);
        if(bind(sockfd, (struct sockaddr*) source_addr, sizeof(struct sockaddr_in6)) == -1) {
            perror("Error in bind()");
            return -1;
        }
    }
    
    if(dest_addr != NULL) {
        dest_addr->sin6_port = htons(dst_port);
        if(connect(sockfd, (struct sockaddr*) dest_addr, sizeof(struct sockaddr_in6)) == -1) {
            perror("Error in connect()");
            return -1;
        }
    }
    return sockfd;
}
