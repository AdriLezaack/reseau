#include "packet_interface.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <zlib.h>
#include <arpa/inet.h>

/* Extra #includes */
/* Ne pas utiliser de threads!!!*/

int main(int argc, char *argv[]) {
	
	//~ char *data = (char *) malloc(27);
	//~ *(data) = 0x5c;
	//~ *(data+1) = 0x7b;
	//~ *(data+2) = 0x00;
	//~ *(data+3) = 0x0b;
	//~ *(data+4) = 0x17;
	//~ *(data+5) = 0x00;
	//~ *(data+6) = 0x00;
	//~ *(data+7) = 0x00;
	//~ *(data+8) = 0x33;
	//~ *(data+9) = 0xda;
	//~ *(data+10) = 0xe3;
	//~ *(data+11) = 0x06;
	//~ //*(data+8) = 0xb7;
	//~ //*(data+9) = 0x90;
	//~ //*(data+10) = 0xed;
	//~ //*(data+11) = 0xfc;
	//~ //payload
	//~ *(data+23) = 0x68;
	//~ *(data+24) = 0x65;
	//~ *(data+25) = 0x6c;
	//~ *(data+26) = 0x6c;
	
	pkt_t *pkt2 = pkt_new();
	pkt_set_type(pkt2, 1);
	pkt_set_tr(pkt2, 0);
	pkt_set_window(pkt2, 28);
	pkt_set_seqnum(pkt2, 123);
	pkt_set_length(pkt2, 11);
	pkt_set_timestamp(pkt2, 9999);
	char * payload = "hello world"; //(char *) malloc(11);
	pkt_set_payload(pkt2, payload, 11);
	char *buf = (char *) malloc(27);
	size_t size2 = 27;
	pkt_status_code code2 = pkt_encode(pkt2, buf, &size2);
	printf("Code2 obtenu:%d\n", code2);
	
	//~ uint32_t *buf2 = (uint32_t *) buf;
	//~ printf("Buffer obtenu:%x", *buf2);
	//~ printf("%x", *(buf2+1));
	//~ printf("%x\n", *(buf2+2));
	
	pkt_t *pkt = pkt_new();
	const size_t size = 27;
	pkt_status_code code = pkt_decode(buf, size, pkt);
	printf("Type:%d\n", pkt_get_type(pkt));
	printf("Tr:%d\n", pkt_get_tr(pkt));
	printf("Window:%d\n", pkt_get_window(pkt));
	printf("Seqnum:%d\n", pkt_get_seqnum(pkt));
	printf("Length:%d\n", pkt_get_length(pkt));
	printf("Timescamp:%d\n", pkt_get_timestamp(pkt));
	printf("CRC1:%d\n", pkt_get_crc1(pkt));
	printf("Code obtenu:%d\n", code);
	
	if (argc == 0) printf("%d\n", argc);
	if (argc == 0) printf("%s\n", argv[0]);
	return EXIT_SUCCESS;
}

struct __attribute__((__packed__)) pkt {
	uint8_t  window : 5;
	uint8_t tr : 1;
	uint8_t type : 2;
	uint8_t  seqnum;
	uint16_t length;
	uint32_t timestamp;
	uint32_t crc1;
	char* payload;
	uint32_t crc2;
};

/* Alloue et initialise une struct pkt
 * @return: NULL en cas d'erreur */
pkt_t* pkt_new() {
	pkt_t *result = (pkt_t *) malloc(sizeof(pkt_t));
	if (result != NULL){
		result->window = 0;
		result->tr = 0;
		result->type = 0;
		result->seqnum = 0;
		result->length = 0;
		result->timestamp = 0;
		result->crc1 = 0;
		result->payload = NULL;
		result->crc2 = 0;
	}
	return result;
}

/* Libere le pointeur vers la struct pkt, ainsi que toutes les
 * ressources associees
 */
void pkt_del(pkt_t* pkt) {
    if (pkt != NULL){
		if (pkt->payload != NULL) free(pkt->payload);
		free(pkt);
	}
}

/*
 * Decode des donnees recues et cree une nouvelle structure pkt.
 * Le paquet recu est en network byte-order.
 * La fonction verifie que:
 * - Le CRC32 du header recu est le même que celui decode a la fin
 *   du header (en considerant le champ TR a 0)
 * - S'il est present, le CRC32 du payload recu est le meme que celui
 *   decode a la fin du payload 
 * - Le type du paquet est valide
 * - La longueur du paquet et le champ TR sont valides et coherents
 *   avec le nombre d'octets recus.
 *
 * @data: L'ensemble d'octets constituant le paquet recu
 * @len: Le nombre de bytes recus
 * @pkt: Une struct pkt valide
 * @post: pkt est la representation du paquet recu
 *
 * @return: Un code indiquant si l'operation a reussi ou representant
 *         l'erreur rencontree.
 */
pkt_status_code pkt_decode(const char *data, const size_t len, pkt_t *pkt) {
	pkt_status_code error = PKT_OK;
	if (len < 12){
		error = E_NOHEADER; //Paquet sans header
		return error;
	}
	
	//header
	uint8_t header = *(data);
	memcpy(pkt, &header, sizeof(uint8_t));
	if (pkt->type < 1 || pkt->type > 3){
		return E_TYPE;
	}
	if (pkt->tr != 0 && pkt->tr != 1){
		return E_TR;
	}
	if (pkt->window > MAX_WINDOW_SIZE){
		return E_WINDOW;
	}
	
	//seqnum
	error = pkt_set_seqnum(pkt, *(data+1));
	if (error != PKT_OK) return error;
	
	//Length
	const uint16_t *data2 = (uint16_t *) data;
	error = pkt_set_length(pkt, ntohs(*(data2+1)));
	if (error != PKT_OK) return error;
	if (pkt->length == 0 && len != 12){
		error = E_UNCONSISTENT; //Longueur du paquet fantaisiste
		return error;
	}
	unsigned int ui = 16; //sinon comparaison signed/unsigned
	if (pkt->length != 0 && len != pkt->length + ui){
		error = E_UNCONSISTENT; //Longueur du paquet fantaisiste
		return error;
	}
	
	//Timestamp
	const uint32_t* data3 = (uint32_t *) data;
	error = pkt_set_timestamp(pkt, *(data3+1));
	if (error != PKT_OK) return error;
	
	//CRC1
	uint32_t crc1 = ntohl(*(data3+2));
	char bytefield[8];
	memcpy(bytefield, data, 8);
	bytefield[0] = bytefield[0] & 0xdf; //Force tr a 0.
	uint32_t crc = crc32(0, (Bytef *) bytefield, 8);
	//~ fprintf(stderr, "CRC attendu: %x\n", crc1);
	//~ fprintf(stderr, "CRC obtenu: %x\n", crc);
	if(crc1 != crc){
		error = E_CRC; //Mauvais CRC
		return error;
	}
	error = pkt_set_crc1(pkt, crc1);
	if (error != PKT_OK) return error;
	
	
	//Payload
	if (len == 12){ //Si paquet tronque, pas de payload a lire
		return error;
	}
	error = pkt_set_payload(pkt, data+12, pkt->length);
	if (error != PKT_OK) return error;
	//~ fprintf(stderr, "Payload1: %s\n", pkt_get_payload(pkt));
	
	//CRC2
	//~ const uint32_t* data4 = ;
	//~ fprintf(stderr, "CRC2: %x\n", *(data4));
	//~ fprintf(stderr, "Buffer: %x\n", *((uint32_t *) data+12+(pkt->length)));
	uint32_t crc2 = ntohl(*(uint32_t *) (data+12+(pkt->length)));
	char bytefield2[pkt->length];
	memcpy(bytefield2, data+12, pkt->length);
	//~ fprintf(stderr, "Bytestream: %s\n", bytefield2);
	fprintf(stderr, "CRC: %x\n", (uint32_t) crc32(0, (Bytef *) bytefield2, pkt->length));
	uint32_t crc_ = crc32(0, (Bytef *) bytefield2, pkt->length);
	fprintf(stderr, "CRC attendu: %x\n", crc2);
	fprintf(stderr, "CRC obtenu: %x\n", crc_);
	if(crc2 != crc_){
		error = E_CRC; //Mauvais CRC
		return error;
	}
	
	return error;
}

/*
 * Encode une struct pkt dans un buffer, prêt a être envoye sur le reseau
 * (c-a-d en network byte-order), incluant le CRC32 du header et
 * eventuellement le CRC32 du payload si celui-ci est non nul.
 *
 * @pkt: La structure a encoder
 * @buf: Le buffer dans lequel la structure sera encodee
 * @len: La taille disponible dans le buffer
 * @len-POST: Le nombre de d'octets ecrit dans le buffer
 * @return: Un code indiquant si l'operation a reussi ou E_NOMEM si
 *         le buffer est trop petit.
 */
pkt_status_code pkt_encode(const pkt_t* pkt, char *buf, size_t *len){
	pkt_status_code error = PKT_OK;
	
	if (*len < 12){
		*len = 0;
		error = E_NOMEM; //Buffer trop petit
		return error;
	}
	if(pkt->length > 0){
		unsigned int ui = 16;
		if(*len < pkt->length + ui){
			*len = 0;
			error = E_NOMEM; //Buffer trop petit
			return error;
		}
	}
	
	memcpy(buf, pkt, sizeof(uint8_t));
	buf++;
	uint8_t seqnum = pkt_get_seqnum(pkt);
	memcpy(buf, &seqnum, sizeof(uint8_t));
	buf++;
	uint16_t length = htons(pkt->length);
	memcpy(buf, &length, sizeof(uint16_t));
	buf += 2;
	uint32_t timestamp = pkt_get_timestamp(pkt);
	memcpy(buf, &timestamp, sizeof(uint32_t));
	buf += 4;
	
	//CRC1
	buf -= 8;
	char bytefield[8];
	memcpy(bytefield, (char *) buf, 8);
	bytefield[0] = bytefield[0] & 0xdf; //Force tr a 0.
	uint32_t crc1 = htonl(crc32(0, (Bytef *) bytefield, 8));
	buf += 8;
	memcpy(buf, &crc1, sizeof(uint32_t));
	
	if(pkt->length == 0){
		*len = 12;
		return error; //Paquet sans payload
	}
	
	buf += 4;
	memcpy(buf, pkt_get_payload(pkt), pkt->length);
	//~ fprintf(stderr, "Payload1: %s\n", pkt_get_payload(pkt));
	
	//CRC2
	char bytefield2[pkt->length];
	memcpy(bytefield2, buf, pkt->length);
	//~ fprintf(stderr, "Bytestream: %s\n", bytefield2);
	//~ uint32_t tt = crc32(0, (Bytef *) bytefield2, pkt->length);
	//~ fprintf(stderr, "CRC: %x\n", htonl(tt));
	uint32_t crc2 = htonl((uint32_t) crc32(0, (Bytef *) pkt->payload, pkt->length));
	//~ fprintf(stderr, "CRC2: %x\n", crc2);
	buf += pkt->length;
	memcpy(buf, &crc2, sizeof(uint32_t));
	
	*len = 16 + pkt->length;
	fprintf(stderr, "Buffer: %x\n", ntohl(*((uint32_t *) buf)));
	return error;
}

/* Accesseurs pour les champs toujours presents du paquet.
 * Les valeurs renvoyees sont toutes dans l'endianness native
 * de la machine!
 */
ptypes_t pkt_get_type  (const pkt_t* pkt){
	return pkt->type;
}

uint8_t  pkt_get_tr(const pkt_t* pkt){
	return pkt->tr;
}

uint8_t  pkt_get_window(const pkt_t* pkt){
	return pkt->window;
}

uint8_t  pkt_get_seqnum(const pkt_t* pkt){
	return pkt->seqnum;
}

uint16_t pkt_get_length(const pkt_t* pkt){
	return pkt->length;
}

uint32_t pkt_get_timestamp   (const pkt_t* pkt){
	return pkt->timestamp;
}

uint32_t pkt_get_crc1   (const pkt_t* pkt){
	return pkt->crc1;
}

uint32_t pkt_get_crc2   (const pkt_t* pkt){
	return pkt->crc2;
}

/* Renvoie un pointeur vers le payload du paquet, ou NULL s'il n'y
 * en a pas.
 */
const char* pkt_get_payload(const pkt_t* pkt){
	return pkt->payload;
}

/* Setters pour les champs obligatoires du paquet. Si les valeurs
 * fournies ne sont pas dans les limites acceptables, les fonctions
 * doivent renvoyer un code d'erreur adapte.
 * Les valeurs fournies sont dans l'endianness native de la machine!
 */
pkt_status_code pkt_set_type(pkt_t *pkt, const ptypes_t type){
	pkt_status_code error = PKT_OK;
	if (type < 1 || type > 3){
		error = E_TYPE;
	} else {
		pkt->type = type;
	}
	return error;
}

pkt_status_code pkt_set_tr(pkt_t *pkt, const uint8_t tr){
	pkt_status_code error = PKT_OK;
	if (pkt->type != 0 && pkt->type != 1){
		error = E_TR;
	} else {
		pkt->tr = tr;
	}
	return error;
}

pkt_status_code pkt_set_window(pkt_t *pkt, const uint8_t window){
	pkt_status_code error = PKT_OK;
	if (window > MAX_WINDOW_SIZE){
		error = E_WINDOW;
	} else {
		pkt->window = window;
	}
	return error;
}

pkt_status_code pkt_set_seqnum(pkt_t *pkt, const uint8_t seqnum){
	pkt_status_code error = PKT_OK;
	//E_SEQNUM si fenetre trop petite
	pkt->seqnum = seqnum;
	return error;
}
pkt_status_code pkt_set_length(pkt_t *pkt, const uint16_t length){
	pkt_status_code error = PKT_OK;
	if (length > MAX_PAYLOAD_SIZE){
		error = E_LENGTH;
	} else {
		pkt->length = length;
	}
	return error;
}

pkt_status_code pkt_set_timestamp(pkt_t *pkt, const uint32_t timestamp){
	pkt_status_code error = PKT_OK;
	pkt->timestamp = timestamp;
	return error;
}

pkt_status_code pkt_set_crc1(pkt_t *pkt, const uint32_t crc1){
	pkt_status_code error = PKT_OK;
	pkt->crc1 = crc1;
	return error;
}

pkt_status_code pkt_set_crc2(pkt_t *pkt, const uint32_t crc2){
	pkt_status_code error = PKT_OK;
	pkt->crc2 = crc2;
	return error;
}

/* Defini la valeur du champs payload du paquet.
 * @data: Une succession d'octets representants le payload
 * @length: Le nombre d'octets composant le payload
 * @POST: pkt_get_length(pkt) == length */
pkt_status_code pkt_set_payload(pkt_t *pkt, const char *data, const uint16_t length){
	pkt_status_code error = PKT_OK;
	if (length > MAX_PAYLOAD_SIZE){
		error = E_LENGTH;
		return error;
	}
	if (pkt->payload != NULL) free(pkt->payload);
	if (data == NULL || length == 0){
		pkt->payload = NULL;
		return error;
	}
	char *result = (char *) malloc(length * sizeof(char));
	if (result == NULL){
		error = E_NOMEM; //Echec de malloc
		return error;
	 }
	pkt->payload = result;
	memcpy(result, data, length);
	pkt->length = length;
	return error;
}
