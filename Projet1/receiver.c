#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
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
	
	port = atoi(argv[optind]);
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
	
	while(1){
		char *buffer = "";
		const size_t len = 0;
		pkt_t *pkt = pkt_new();
		if (pkt_decode(buffer, len, pkt) == PKT_OK){
			//Si le paquet est valide...
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
		} else {
			//Sinon ignore-le.
		}
	}
	
	if (fclose(f) == -1){
		printf("Erreur fermeture du fichier d'ecriture.\n");
		return EXIT_FAILURE;
	}
	
	return EXIT_SUCCESS;
}
