#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "registro_client.h"

#include "log.h"
#include "mappa.h"

#define HELLO (isTrain) ((isTrain) ? "TRAIN" : "SUPER")
#define OK ("OKE")
#define GET_TRENI ("TRENI")
#define GET_ITINERARIO ("ITINERARIO")

///TODO: replace with inline function?
#define TIMEOUT(ret)\
(({\
	LOGE("Time out!\n");\
	rc_fini();\
}), (ret))						//Evaluates to ret


static int _fd;
static bool _isTrain;

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
	struct sockaddr_un addr =
	{
		.sun_family = AF_UNIX,
		.sun_path = "./sock/registro"
	};

	struct timeval rdtimeout =
	{
		.tv_sec = 5
	};

	//Crea la socket
	_fd = socket(AF_UNIX, SOCK_STREAM, 0 /*default*/);
	if(_fd<0)
	{
		LOGE("Impossibile aprire la socket!\n");
		perror("registro_client");
		return false;
	}

	//Imposta un timeout
	if(setsockopt(_fd, SOL_SOCKET, SO_RCVTIMEO, /*(const char*)*/&rdtimeout, sizeof rdtimeout) < 0)
	{
		LOGE("Impossibile impostare la socket!\n");
		perror("registro_client");
		return false;
	}

	//Apre la socket
	if(connect(_fd, &addr, sizeof(addr)) < 0)
	{
		LOGE("Impossibile connettersi alla socket!\n");
		perror("registro_client");
		return false;
	}

	//Comunica al registro che l'interlocutore è un treno
	uint8_t len = sizeof(HELLO_MSG(_isTrain));
	write(_fd, &len, 1);
	write(_fd, HELLO_MSG(_isTrain), len);

	char buf[8] = {0};

	if(read(_fd, buf, sizeof(OK)) < sizeof(OK))
		return TIMEOUT(false);

	if(strcmp(buf, OK) != 0)
	{
		LOGE("Risposta inaspettata!\n");
		rc_fini();
		return false;
	}
}

int rc_getNumeroTreni(void)
{
	///TODO: Short?
	int numTreni;

	write(_fd, GET_TRENI, sizeof(GET_TRENI));

	if(read(_fd, &numTreni, sizeof(int)) < sizeof(int))
		return TIMEOUT(-1);

	return numTreni;
}

struct itinerario *rc_getItinerario(void)
{
	struct itinerario *itin = malloc(sizeof(struct itinerario));

	if(!itin)
	{
		LOGE("Impossibile allocare memoria!\n");
		return NULL;
	}

	write(_fd, GET_ITINERARIO, sizeof(GET_ITINERARIO));

	if(read(_fd, &itin->num_itinerario, sizeof(unsigned short)) < sizeof(unsigned short))
		return TIMEOUT(NULL);

	return 0xDEADBEEF;
}

void rc_freeItinerario(struct itinerario *itin)
{
	free(itin);
	///TODO: free fields
}

void rc_fini(void)
{
	close(_fd);
}