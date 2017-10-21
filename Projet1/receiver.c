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

int main(int argc, char *argv[]){

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
				printf("%s\n", optarg);
				if (argc < 5){
					printf("Erreur: trop peu d'arguments.\n");
					return EXIT_FAILURE;
				}
				// printf("Option: %s\n", optarg);
				break;
			case '?':
				 printf("Pas d'option");
				break;
			default:
				 printf("Pas d'option 2");
				break;
		}
	}
	//~ file = "output";
	addres = argv[optind];
	errno = 0;
	char *endptr;
	port = strtold(argv[optind + 1], &endptr);
	if (errno != 0 || argv[optind] == endptr){
		printf("Port invalide: %s\n", argv[optind]);
		return EXIT_FAILURE;
	}

	//~ printf("Port: %d\n", port);
	//~ printf("Addres: %s\n", addres);

	struct sockaddr_in6 addr;
	const char *err = real_address(addres, &addr);
	if (err) {
		fprintf(stderr, "Could not resolve hostname %s: %s\n", addres, err);
		return EXIT_FAILURE;
	}
	/* Get a socket */
	int sfd;

	sfd = create_socket(&addr, port, NULL, -1); /* Bound */

	if (sfd < 0) {
		fprintf(stderr, "Failed to create the socket!\n");
		return EXIT_FAILURE;
	}


	FILE *f = NULL;
	if (file != NULL){
		f = fopen(file, "wb");
		if (f == NULL) {
			printf("Erreur: impossible d'ouvrir le fichier d'ecriture.\n");
			return EXIT_FAILURE;
		}
	}

	struct timeval tv ;
	tv.tv_sec = 20;
	tv.tv_usec = 500000;

	fd_set readfd, fd;
	char buffer[1024];

	/*on attend des données en entrée*/
	FD_ZERO(&fd);
	FD_SET(sfd,&fd);

	uint8_t window = 1;
	uint8_t seqnum = 0;
	size_t len = 1024;
//	int rd;

	while(1){
		size_t lenbuffer = 1024;
		readfd = fd;
		if(select(sfd+1, &readfd, NULL, NULL, &tv) == -1){
				fprintf(stderr, "read_write_loop.c: problème au niveau du select\n "
						"L'erreur est %s\n", strerror(errno));
				exit(-1);
		}

		len = read(sfd,buffer,len);

		pkt_t *pkt = pkt_new();
		if (pkt_decode(buffer, len, pkt) == PKT_OK){
			//Si le paquet est valide...
			if (pkt_get_type(pkt) == 1){
				//Si le type est 1...
				if (pkt_get_seqnum(pkt) == seqnum){
					//Si le packet a bien un seqnum attendu
					printf("Length: %d.\n", pkt_get_length(pkt));
					if (pkt_get_length(pkt) == 0){
						break; //Fin de la connexion.
					}
					pkt_t *ack = pkt_new();
					if (pkt_get_tr(pkt) == 0){
						//Si le paquet a un payload...
						int wr = -1;
						if (f != NULL) {
							wr = fwrite(pkt_get_payload(pkt), 1, pkt_get_length(pkt), f);
						} else {
							wr = write(STDOUT_FILENO, pkt_get_payload(pkt), pkt_get_length(pkt));
						}
						if(wr < 0) {
							printf("Erreur d'ecriture.\n");
							return EXIT_FAILURE;
						}
						//Envoit un ack
						pkt_set_type(ack, 2);
						pkt_set_tr(ack, 0);
						pkt_set_window(ack, window);
						pkt_set_seqnum(ack, pkt_get_seqnum(pkt) + 1);
					} else {
						//Sinon envoit un non-ack
						pkt_set_type(ack, 3);
						pkt_set_tr(ack, 0);
						pkt_set_window(ack, window);
						pkt_set_seqnum(ack, pkt_get_seqnum(pkt));
					}
					pkt_encode(ack, buffer, &lenbuffer); //E_Nomem
/*					if(write(sfd,buffer,lenbuffer) < 0){
						fprintf(stderr, "Erreur d'ecriture sur le sfd\n");
						return EXIT_FAILURE;
					}*/
				}
			}
		}
		//Sinon ignore-le.

		len = 1024;
		lenbuffer = 1024;
		seqnum = (seqnum+1)%256;
	}

	if (fclose(f) == -1){
		printf("Erreur fermeture du fichier d'ecriture.\n");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
