#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "packet_interface.h"
#include "real_address.h"
#include "create_socket.h"
#include "wait_for_client.h"

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
		f = fopen(file, "wb");
		if (f == NULL) {
			printf("Erreur: impossible d'ouvrir le fichier d'ecriture.\n");
			return EXIT_FAILURE;
		}
	}
	
	uint8_t window = 1;
	uint8_t seqnum = 0;
	while(1){
		char *buffer = "";
		const size_t len = 0;
		size_t lenbuffer = 0;
		pkt_t *pkt = pkt_new();
		if (pkt_decode(buffer, len, pkt) == PKT_OK){
			//Si le paquet est valide...
			if (pkt_get_type(pkt) == 1){
				//Si le type est 1...
				if (pkt_get_seqnum(pkt) == seqnum){
					//Si le packet a bien un seqnum attendu
					pkt_t *ack = pkt_new();
					if (pkt_get_tr(pkt) == 0){
						//Si le paquet a un payload...
						int wr = -1;
						if (f != NULL) {
							wr = fwrite(pkt_get_payload(pkt), 1, pkt_get_length(pkt), f);
						
						} else {
							wr = write(STDOUT_FILENO, pkt_get_payload(pkt), pkt_get_length(pkt));
						}
						if(wr != 1) {
							printf("Erreur d'ecriture.\n");
							return EXIT_FAILURE;
						}
						//Envoit un ack
						pkt_set_type(ack, 1);
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
					pkt_encode(ack, buffer, &lenbuffer);
				}
			}
		}
		//Sinon ignore-le.
	}
	
	if (fclose(f) == -1){
		printf("Erreur fermeture du fichier d'ecriture.\n");
		return EXIT_FAILURE;
	}
	
	return EXIT_SUCCESS;
}
