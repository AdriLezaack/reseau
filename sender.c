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

int main(int argc, char *argv[]) {

	if (argc < 3){
		fprintf(stderr, "Erreur: trop peu d'arguments.\n");
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
					fprintf(stderr, "Erreur: trop peu d'arguments.\n");
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

	addres = argv[optind];
	errno = 0;
	char *endptr = NULL;
	port = strtold(argv[optind + 1], &endptr);
	if (errno != 0 || argv[optind+1] == endptr){
		fprintf(stderr, "Port invalide: %s\n", argv[optind+1]);
		return EXIT_FAILURE;
	}
	//~ printf("Port: %d\n", port);
	//~ printf("Addres: %s\n", addres);

	/* Resolve the hostname */
	struct sockaddr_in6 addr;
	const char *err = real_address(addres, &addr);
	if (err) {
		fprintf(stderr, "Could not resolve hostname %s: %s\n", addres, err);
		return EXIT_FAILURE;
	}

	// Initialisation sfd
	int sfd;
	sfd = create_socket(NULL, -1, &addr, port); /* Connected */

	if (sfd < 0) {
		fprintf(stderr, "Failed to create the socket!\n");
		return EXIT_FAILURE;
	}
	// fprinf(stdin, "Connection établie \n");

	FILE *f = NULL;
	if (file != NULL){
		f = fopen(file, "rb");
		if (f == NULL) {
			fprintf(stderr, "Erreur: impossible d'ouvrir le fichier de lecture.\n");
			return EXIT_FAILURE;
		}
	}
	
	uint8_t window = 1;
	uint8_t seqnum = 0;
	char data[512];
	char* buffer = (char *) malloc(1024);
	size_t len = 1024;
	//TO DO Si pas de buffer, setter window a 0.
	
	while(1){
		
		int rd = -1;
		if (f != NULL){
			rd = fread(data, 1, 512, f);
		} else {
			fd_set fd;
			FD_ZERO(&fd);
			FD_SET(STDIN_FILENO, &fd);
			
			if(select(sfd+1, &fd, NULL, NULL, NULL) == -1){
				fprintf(stderr, "sender(): erreur dans select\n "
						"L'erreur est %s\n", strerror(errno));
				return EXIT_FAILURE;
			}
			rd = read(STDIN_FILENO, data, 512);
		}
		
		if (rd == -1){
				printf("Erreur de lecture.\n");
				return EXIT_FAILURE;
		}

		pkt_t *pkt = pkt_new();
		pkt_set_type(pkt, 1);
		pkt_set_tr(pkt,0);
		pkt_set_window(pkt, window);
		pkt_set_seqnum(pkt, seqnum);
		//pkt_set_length //Fait implicitement dans ce qui suit.
		if (pkt_set_payload(pkt, data, rd) != PKT_OK){
			pkt_set_tr(pkt, 1); //Tronquer le paquet
		}

		pkt_encode(pkt, buffer, &len);

		if(write(sfd, buffer,len)<0){
			fprintf(stderr, "sender(): Impossible d'envoyer le packet\n%s\n", strerror(errno));
			return EXIT_FAILURE;
		}
		
		if(pkt_get_length(pkt) == 0 && pkt_get_tr(pkt) == 0){
			break; //Fin de la connexion.
		}
		
		seqnum ++;
		len = 1024;
	}
	
	if (f != NULL){
		if (fclose(f) == -1){
			fprintf(stderr, "Erreur fermeture du fichier de lecture.\n");
			return EXIT_FAILURE;
		}
	}

	return EXIT_SUCCESS;
}
