#include <stdlib.h> /* EXIT_X */
#include <stdio.h> /* fprintf */
#include <unistd.h> /* getopt */
#include <netinet/in.h> //MÈthode 1
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>

#define SIZE 1024

/* Block the caller until a message is received on sfd,
 * and connect the socket to the source addresse of the received message.
 * @sfd: a file descriptor to a bound socket but not yet connected
 * @return: 0 in case of success, -1 otherwise
 * @POST: This call is idempotent, it does not 'consume' the data of the message,
 * and could be repeated several times blocking only at the first call.
 */
int wait_for_client(int sfd){
    struct sockaddr_in6 addr;
    char buf[SIZE];
    socklen_t addrlen = sizeof(struct sockaddr_in6);
    
    memset(&addr, 0, addrlen);

    
    if(recvfrom(sfd, buf, SIZE, MSG_PEEK, (struct sockaddr *)&addr, &addrlen) == -1){
        fprintf(stderr, "Error in recvfrom() : %s\n", strerror(errno));
        return -1;
    }
    if(connect(sfd, (struct sockaddr *)&addr, addrlen)){
        fprintf(stderr, "Error in connect() : %s\n", strerror(errno));
        return -1;
    }
    
    return 0;
}
