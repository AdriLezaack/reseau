#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "packet_interface.h"

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
		f = fopen(file, "rb");
		if (f == NULL) {
			printf("Erreur: impossible d'ouvrir le fichier de lecture.\n");
			return EXIT_FAILURE;
		}
	} else {
		//f = STDIN_FILENO;
	}
	
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
		pkt_set_payload(pkt, data, rd);
		free(data);
	
		pkt_encode(pkt, buffer, &len);
		//write(sfd, buffer, len);
	}
	
	if (fclose(f) == -1){
		printf("Erreur fermeture du fichier de lecture.\n");
		return EXIT_FAILURE;
	}
	
	return EXIT_SUCCESS;
}
