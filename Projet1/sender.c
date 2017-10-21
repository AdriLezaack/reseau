#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "packet_interface.h"
#include "real_address.h"
#include "create_socket.h"
#include "wait_for_client.h"


#include <sys/time.h>
#include <sys/types.h>

#define STDIN 0  // file descriptor for standard input

int main2(void) {
    struct timeval tv;
    fd_set readfds;

    tv.tv_sec = 2;
    tv.tv_usec = 500000;

    FD_ZERO(&readfds);
    FD_SET(STDIN, &readfds);

    // don't care about writefds and exceptfds:
    select(STDIN+1, &readfds, NULL, NULL, &tv);

    if (FD_ISSET(STDIN, &readfds))
        printf("A key was pressed!\n");
    else
        printf("Timed out.\n");

    return 0;
} 


int main(int argc, char *argv[]) {
	
	if (argc < 3){
		printf("Erreur: trop peu d'arguments.\n");
		return EXIT_FAILURE;
	}
	
	unsigned int port;
	char *addres;
	char *file = NULL;
	int opt;
	
	while ((opt = getopt(argc, argv, "f:")) != -1) {
		switch (opt) {
			case 'f':
				file = optarg;
				if (argc < 5){
					printf("Erreur: trop peu d'arguments.\n");
					return EXIT_FAILURE;
				}
				//~ printf("Option: %s\n", optarg);
				break;
			case '?':
				//~ printf("Pas d'option");
				break;
			default:
				//~ printf("Pas d'option");
				break;
		}
	}
	
	errno = 0;
	char *endptr;
	port = strtold(argv[optind], &endptr);
	if (errno != 0 || argv[optind] == endptr){
		printf("Port invalide: %s\n", argv[optind]);
		return EXIT_FAILURE;
	}
	addres = argv[optind + 1];
	printf("Port: %d\n", port);
	printf("Addres: %s\n", addres);
	
	
	FILE *f = NULL;
	if (file != NULL){
		f = fopen(file, "rb");
		if (f == NULL) {
			printf("Erreur: impossible d'ouvrir le fichier de lecture.\n");
			return EXIT_FAILURE;
		}
	} else {
		//f = STDIN_FILENO;
	}
	
	uint8_t window = 1;
	uint8_t seqnum = 0;
	//TO DO Si pas de buffer, setter window a 0.
	while(1){
		char *buffer = "";
		size_t len = 0;
		char *data = malloc(512);
		int rd = -1;
		if (f != NULL){
			//lit sur un fichier
			rd = fread(data, 1, 512, f);
		} else {
			rd = read(STDIN_FILENO, data, 512);
		}
		
		if (rd == -1){
				printf("Erreur de lecture.\n");
				return EXIT_FAILURE;
		}			
		pkt_t *pkt = pkt_new();
		pkt_set_type(pkt, 1);
		pkt_get_tr(0);
		pkt_set_window(pkt, window);
		pkt_set_seqnum(pkt, seqnum);
		//pkt_set_length //Fait implicitement dans ce qui suit.
		if (pkt_set_payload(pkt, data, rd) != PKT_OK){
			pkt_set_tr(pkt, 1); //Tronquer le paquet
		}	
		free(data);
	
		pkt_encode(pkt, buffer, &len);
		//write(sfd, buffer, len);
		
		seqnum ++;
	}
	
	if (fclose(f) == -1){
		printf("Erreur fermeture du fichier de lecture.\n");
		return EXIT_FAILURE;
	}
	
	return EXIT_SUCCESS;
}
