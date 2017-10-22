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

#include <time.h>

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
	port = strtold(argv[optind + 1], &endptr);
	if (errno != 0 || argv[optind+1] == endptr){
		fprintf(stderr, "Port invalide: %s\n", argv[optind+1]);
		return EXIT_FAILURE;
	}
	
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
	char buffer[1024];
	pkt_t** buffers = malloc(sizeof(struct pkt_t*)*256);

	int i;
	for(i=0;i<256;i++)
		buffers[i] = NULL;

	size_t len = 1024;

	int disposfd=window;
	int dispobuf=3;
	int lastack = -1;
	int finbuf = 0;

	while(1){
		if((dispobuf>0)&&(!finbuf)){
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
					fprintf(stderr, "Erreur de lecture.\n");
					return EXIT_FAILURE;
			}
			pkt_t *pkt = pkt_new();
			pkt_set_type(pkt, 1);
			pkt_set_tr(pkt,0);
			pkt_set_window(pkt, 31);
			pkt_set_seqnum(pkt, seqnum);

			if (pkt_set_payload(pkt, data, rd) != PKT_OK){
				pkt_set_tr(pkt, 1); //Tronquer le paquet
			}

			if(buffers[seqnum]!=NULL)
				pkt_del(buffers[seqnum]);

			buffers[seqnum]=pkt;
			dispobuf--;
			if(pkt_get_length(pkt)==0){
				finbuf=1;
				pkt_set_window(pkt,0);
			}
			seqnum = (seqnum+1)%256;

		}

		if((disposfd>0)&&(dispobuf>0)){
			int j;
			for(j=0; (j!=disposfd)&&(j!=(seqnum-lastack-1+256)%256);j++){
				pkt_encode(buffers[(lastack+1+j)%256], buffer, &len); //-1
				if(write(sfd, buffer,len)<0){
					fprintf(stderr, "sender(): Impossible d'envoyer le packet\n%s\n", strerror(errno));
					return EXIT_FAILURE;
				}
				pkt_set_timestamp(buffers[(lastack+1+j)%256],(uint32_t)(clock()/CLOCKS_PER_SEC));
			}
			disposfd-=j;
			dispobuf+=j;
		}

		// On renvoie les segments non acquittés
		for(i=(lastack+1)%256;i!=(seqnum-finbuf+256)%256;i=(i+1)%256){
			fd_set fd;
			FD_ZERO(&fd);
			FD_SET(sfd, &fd);
			struct timeval tv;
			tv.tv_sec=0;
			tv.tv_usec=(clock()/CLOCKS_PER_SEC-pkt_get_timestamp(buffers[i]))*1000;
			if(select(sfd+1, &fd, NULL, NULL, &tv) == 1){
				pkt_encode(buffers[i], buffer, &len);
				if(write(sfd, buffer,len)<0){
					fprintf(stderr, "sender(): Impossible d'envoyer le packet\n%s\n", strerror(errno));
					return EXIT_FAILURE;
				}
				pkt_set_timestamp(buffers[i],(uint32_t)clock()/CLOCKS_PER_SEC);
			}
			else{
				char ack[1024];
				len = read(sfd,ack,1024);
				// Traite l'ack
				pkt_t *pkt = pkt_new();
				if (pkt_decode(ack, len, pkt) == PKT_OK)
					if(pkt_get_type(pkt) == 2){
					 	lastack = (pkt_get_seqnum(pkt)-1+256)%256;
					 	window = pkt_get_window(pkt);
		        disposfd = window - ((seqnum-1 - lastack)+256)%256;
					}
					if(pkt_get_type(pkt) == 3){
						int tronc = (pkt_get_seqnum(pkt)-1+256)%256;
						if (((seqnum < lastack) && ((tronc < lastack) == (tronc < seqnum))) ||
								((seqnum > lastack) && ((tronc > lastack) && (tronc < seqnum)))){
									window /= 2;
									if(window == 0)
											window = 1;
									disposfd = window - ((seqnum-1 - lastack)+256)%256;
								}

					}
			}
		}

		if(finbuf && dispobuf)
			break;
		len = 1024;
	}


	for(i=0;i<256;i++)
		if(buffers[i]!=NULL)
			pkt_del(buffers[i]);
	free(buffers);

	if (f != NULL){
		if (fclose(f) == -1){
			fprintf(stderr, "Erreur fermeture du fichier de lecture.\n");
			return EXIT_FAILURE;
		}
	}

	return EXIT_SUCCESS;
}
