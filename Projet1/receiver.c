#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "packet_interface.h"
#include "real_address.h"
#include "create_socket.h"
#include "wait_for_client.h"

#define BUFFER_SIZE 1024
#define BUFFER_2_SIZE 50

uint8_t INITIAL_WINDOW = 3; //Le nombre de paquet que l'on peut accepter


#include <sys/time.h>
#include <sys/types.h>

int main(int argc, char *argv[]){

	if (argc < 3){
		fprintf(stderr,"Erreur: trop peu d'arguments.\n");
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
				break;
			case '?':
				break;
			default:
				break;
		}
	}
	
	addres = argv[optind];
	errno = 0;
	char *endptr;
	port = strtold(argv[optind +1], &endptr);
	if (errno != 0 || argv[optind+1] == endptr){
		fprintf(stderr,"Port invalide: %s\n", argv[optind+1]);
		return EXIT_FAILURE;
	}

	struct sockaddr_in6 addr;
	const char *err = real_address(addres, &addr);
	if (err) {
		fprintf(stderr, "Could not resolve hostname %s: %s\n", addres, err);
		return EXIT_FAILURE;
	}
	/* Get a socket */
	int sfd;

	sfd = create_socket(&addr, port, NULL, -1); /* Bound */
	if (sfd > 0 && wait_for_client(sfd) < 0) { /* Connected */
		fprintf(stderr, "Could not connect the socket after the first message.\n");
		close(sfd);
		return EXIT_FAILURE;
	}

	if (sfd < 0) {
		fprintf(stderr, "Failed to create the socket!\n");
		return EXIT_FAILURE;
	}


	FILE *f = NULL;
	if (file != NULL){
		f = fopen(file, "wb");
		if (f == NULL) {
			fprintf(stderr,"Erreur: impossible d'ouvrir le fichier d'ecriture.\n");
			return EXIT_FAILURE;
		}
	}

	struct timeval tv ;
	tv.tv_sec = 30;
	tv.tv_usec = 0;

	fd_set readfd, fd;

	/*on attend des données en entrée*/
	FD_ZERO(&fd);
	FD_SET(sfd,&fd);

	uint8_t window = INITIAL_WINDOW;
	uint8_t seqnum = 0; //Le seqnum attendu. Toujours a 0 au début.
	
	size_t len = BUFFER_SIZE;
	char buffer[BUFFER_SIZE];
	
	size_t len_2 = BUFFER_2_SIZE;
	char buffer_2[BUFFER_2_SIZE];
	
	pkt_t *packets[MAX_WINDOW_SIZE];
	int i;
	for(i=0; i < MAX_WINDOW_SIZE ; i++){
		packets[i] = NULL;
	}

	while(1){
		tv.tv_sec = 30;//
		tv.tv_usec = 0;//
		readfd = fd;
		int sel = select(sfd+1, &readfd, NULL, NULL, &tv);
		if(sel == -1){
				fprintf(stderr, "read_write_loop.c: problème au niveau du select\n "
						"L'erreur est %s\n", strerror(errno));
				return EXIT_FAILURE;
		} else if (sel == 0){
			//Deconnexion brutale (pas d'envoi pendant 30s).
			break;
		}
		
		len = read(sfd, buffer, len);

		pkt_t *pkt = pkt_new();
		if (pkt_decode(buffer, len, pkt) == PKT_OK && pkt_get_type(pkt) == 1){
			//Si le paquet est valide et son type est 1...
			uint8_t p_seqnum = pkt_get_seqnum(pkt);
			int make_ack = -1;
			if (p_seqnum == seqnum){
				//Si le packet a bien le seqnum attendu
				if (pkt_get_length(pkt) == 0){
					pkt_del(pkt);
					break; //Fin de la connexion.
				}
				int j;
				int k = 0;
				if (window == 0){ //Debloque-toi!
					int wri = -1;
					if (f != NULL) {
						wri = fwrite(pkt_get_payload(pkt), 1, pkt_get_length(pkt), f);
					} else {
						wri = write(STDOUT_FILENO, pkt_get_payload(pkt), pkt_get_length(pkt));
					}
					if(wri < 0) {
						fprintf(stderr,"Erreur d'ecriture.\n");
						return EXIT_FAILURE;
					}
					pkt_del(pkt);
					seqnum = (seqnum+1)%256;
					make_ack = seqnum;
				} else {
					packets[0] = pkt;
				}
				for (j=0; j<window; j++){
					if (packets[j] != NULL && k == 0){
						//Le paquet est bon, lis-le.
						int wr = -1;
						if (f != NULL) {
							wr = fwrite(pkt_get_payload(packets[j]), 1, pkt_get_length(packets[j]), f);
						} else {
							wr = write(STDOUT_FILENO, pkt_get_payload(packets[j]), pkt_get_length(packets[j]));
						}
						if(wr < 0) {
							fprintf(stderr,"Erreur d'ecriture.\n");
							return EXIT_FAILURE;
						}
						pkt_del(packets[j]);
						//packets[j] = NULL; //Fait par le else et la boucle en l.
						seqnum = (seqnum+1)%256;
						make_ack = seqnum;
					} else {
						//Bouge-toi.
						packets[k] = packets[j];
						k++;
					}
				}
				int l;
				for(l = k; l<window; l++){
					packets[l] = NULL;
				}
				
			} else if ((p_seqnum > seqnum && p_seqnum < seqnum + window) || (p_seqnum < seqnum && p_seqnum < (seqnum+window)%256)){
				//Paquet acceptable.
				int offset = 0;
				if (p_seqnum > seqnum) offset = p_seqnum - seqnum;
				if (p_seqnum < seqnum) offset = 256 + p_seqnum - seqnum;
				if (packets[offset] != NULL){
					//S'il n'a pas deja ete recu.
					packets[offset] = pkt;
				}
				make_ack = seqnum;
			} else {
				//Hors sequence. A ignorer.
				pkt_del(pkt);
				make_ack = -1;
			}
			if(make_ack >= 0){
				uint8_t newWindow = pkt_get_window(pkt);
				if(newWindow < window){
					int n;
					for (n=newWindow; n<window; n++){
						if (packets[n] != NULL){
							pkt_del(packets[n]);
						}
					}
					window = newWindow;
				}
				if(window < newWindow && window < INITIAL_WINDOW){
					if (INITIAL_WINDOW < newWindow){
						newWindow = INITIAL_WINDOW;
					}
					window = newWindow;
				}
				pkt_t *ack = pkt_new();
				if (pkt_get_tr(pkt) == 0){
					//Si le paquet a un payload...
					//Envoit un ack
					pkt_set_type(ack, 2);
					pkt_set_tr(ack, 0);
					pkt_set_window(ack, INITIAL_WINDOW);
					pkt_set_seqnum(ack, make_ack);
					pkt_set_timestamp(ack, pkt_get_timestamp(pkt));
				} else {
					//Sinon envoit un non-ack
					pkt_set_type(ack, 3);
					pkt_set_tr(ack, 0);
					pkt_set_window(ack, INITIAL_WINDOW);
					pkt_set_seqnum(ack, p_seqnum);
					pkt_set_timestamp(ack, pkt_get_timestamp(pkt));
				}
				pkt_encode(ack, buffer_2, &len_2); //TO DO: E_Nomem
				
				if(write(sfd, buffer_2, len_2) < 0){
					fprintf(stderr, "Erreur lors de l'envoi du ack.\n");
					fprintf(stderr, "L'erreur est %s\n", strerror(errno));
					return EXIT_FAILURE;
				}
				if(INITIAL_WINDOW == 0){ //Si INITIAL_WINDOW nulle...
					len_2 = BUFFER_SIZE;
					while(INITIAL_WINDOW == 0){
						//Attendre qu'un autre thread met INITIAL_WINDOW > 0
					}
					pkt_set_window(ack, INITIAL_WINDOW);
					pkt_encode(ack, buffer_2, &len_2); //TO DO: E_Nomem
					if(write(sfd, buffer_2, len_2) < 0){
						fprintf(stderr, "Erreur lors de l'envoi du ack.\n");
						fprintf(stderr, "L'erreur est %s\n", strerror(errno));
						return EXIT_FAILURE;
					}
				}
				pkt_del(ack);
				len_2 = BUFFER_SIZE;
			}//if ack
		} else {
			//Si non valide, ignore-le.
			pkt_del(pkt);
		}
		len = BUFFER_SIZE;
	}
	
	int m;
	for(m=0; m < MAX_WINDOW_SIZE; m++){
		if (packets[m] != NULL){
			pkt_del(packets[m]);
		}
	}
	
	if (f != NULL){
		if (fclose(f) == -1){
			fprintf(stderr,"Erreur fermeture du fichier d'ecriture.\n");
			return EXIT_FAILURE;
		}
	}

	return EXIT_SUCCESS;
}
