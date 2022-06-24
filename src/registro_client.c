#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "registro_client.h"

#include "log.h"
#include "mappa.h"
#include "socket.h"
#include "inline_rw_utils.h"

#define HELLO(isTrain) ((isTrain) ? "TRAIN" : "SUPER")
#define OK ("OK")
#define GET_TRENI ("TRENI")
#define GET_ITINERARIO ("ITINERARIO")
#define GET_MAPPA ("MAPPA")

static int _fd;

/* Si presuppone che la socket sia in nella sottodirectory sock, a sua volta presente nella PWD
	
	. (PWD)
	L___sock
	|	L___registro
	|
	L___[processo chiamante]
	L___[processo registro]
*/

bool rc_init(bool isTrain)
{
	_fd = connettiSocketUnix("./sock/registro");
	if(_fd < 0)
		return false;

	//Comunica al registro che l'interlocutore è un treno
	uint8_t len = strlen(HELLO(isTrain));
	writeWL(_fd, HELLO(isTrain), len);

	char buf[8] = {0};

	readWL(_fd, buf, sizeof(OK));

	if(strcmp(buf, OK) != 0)
	{
		LOGE("Risposta inaspettata!\n");
		rc_fini();
		return false;
	}

	return true;
}

int rc_getNumeroTreni(void)
{
	///TODO: Short?
	int numTreni;

	writeWL(_fd, GET_TRENI, sizeof(GET_TRENI));

	readWL(_fd, (char *)&numTreni, sizeof(int));

	return numTreni;
}

struct itinerario *rc_getItinerario(void)
{
	struct itinerario *itin = malloc(sizeof(struct itinerario));

	if(!itin)
		return NULL;

	writeWL(_fd, GET_ITINERARIO, sizeof(GET_ITINERARIO));

	readWL(_fd, (char *)&itin->num_itinerario, sizeof(uint8_t));
	readWL(_fd, (char *)&itin->num_tappe, sizeof(uint8_t));

	LOGD("I am Train number %d\n", itin->num_itinerario);
	LOGD("I have a total of %d stops\n", itin->num_tappe);

	itin->tappe = malloc(itin->num_tappe * sizeof(char *));

	///TODO: (timeout and) error checks
	for(int i=0; i<itin->num_tappe; i++)
	{
		uint8_t tappaLen;

		read(_fd, &tappaLen, sizeof(uint8_t));
		itin->tappe[i] = malloc(tappaLen);
		read(_fd, itin->tappe[i], tappaLen);

		LOGD("Stop #%d is %s\n", i, itin->tappe[i]);
	}

	return itin;
}

struct mappa *rc_getMappa(void)
{
	struct mappa *mappa = malloc(sizeof(struct mappa));

	if(!mappa)
		return NULL;

	writeWL(_fd, GET_MAPPA, sizeof(GET_MAPPA));

	readWL(_fd, (char *)&mappa->treni, sizeof(uint8_t));

	mappa->itinerari = calloc(mappa->treni, sizeof(struct itinerario));

	for(int i=0; i<mappa->treni; i++)
	{
		struct itinerario *itin = &mappa->itinerari[i];

		readWL(_fd, (char *)&itin->num_itinerario, sizeof(uint8_t));
		readWL(_fd, (char *)&itin->num_tappe, sizeof(uint8_t));

		LOGD("Itinerario %d ha %d tappe\n", itin->num_itinerario, itin->num_tappe);

		itin->tappe = malloc(itin->num_tappe * sizeof(char *));

		///TODO: error checks
		for(int j=0; j<itin->num_tappe; j++)
		{
			uint8_t tappaLen;

			read(_fd, &tappaLen, sizeof(uint8_t));
			
			itin->tappe[j] = malloc(tappaLen);
			if(!itin->tappe[j])
				return NULL;

			read(_fd, itin->tappe[j], tappaLen);

			LOGD("Stop #%d is %s\n", j, itin->tappe[j]);
		}
	}

	return mappa;

}

void rc_freeItinerario(struct itinerario *itin)
{
	///TODO: free fields
	free(itin);
}

void rc_freeMappa(struct mappa *mappa)
{
	///TODO: free fields
	free(mappa);
}

void rc_fini(void)
{
	close(_fd);
}
