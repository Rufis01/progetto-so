#include <stdlib.h>
#include <stdint.h>

#include <unistd.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>

#include "mappa.h"
#include "log.h"
#include "inline_rw_utils.h"

/*static char *tappe[2] =
{
	{"S1","S2","S3","S4","S5","S6","S7","S8"},
	{"MA1","MA2","MA3","MA4","MA5","MA6","MA7","MA8","MA9","MA10","MA11","MA12","MA13","MA14","MA15","MA16"}
};*/

#define TRENI_MAPPA1 4
#define TRENI_MAPPA2 5

static char **nomi_mappa1[TRENI_MAPPA1] =
{
	{"S1","MA1","MA2","MA3","MA8","S6"},
	{"S2","MA5","MA6","MA7","MA3","MA8","S6"},
	{"S7","MA13","MA12","MA11","MA10","MA9","S3"},
	{"S4","MA14","MA15","MA16","MA12","S8"},
};

static char **nomi_mappa2[TRENI_MAPPA2] =
{
	{"S2","MA5","MA6","MA7","MA3","MA8","S6"},
	{"S3","MA9","MA10","MA11","MA12","S8"},
	{"S4","MA14","MA15","MA16","MA12","S8"},
	{"S6","MA8","MA3","MA2","MA1","S1"},
	{"S5","MA4","MA3","MA2","MA1","S1"},
};

static int creaSocket(void);
static void servizio(void);

/* processo_registro richiede come parametro la mappa */
int main(int argc, char **argv)
{
	if(argc != 2)
	{
		exit(EXIT_FAILURE);
	}

	mappa_id map = m_getMappaId(argv[2]);

	if(map == MAPPA_NON_VALIDA)
	{
		exit(EXIT_FAILURE);
	}

	int sfd = creaSocket();
	accettaConnessioni(sfd, map);

	//crea socket AF_UNIX (unica)
	//mettiti in ascolto
	//rispondi n volte con l'itinerario i
	///TODO: muori?

	return 0;

}

static void accettaConnessioni(int sfd, mappa_id map)
{
	struct sockaddr_un clientAddr;
	int clientLen;
	int treni = 0;

	char buf[64 + 1] = {0};
	for(;;)
	{
		int clientFd = accept (sfd, &clientAddr, &clientLen);

		if(readWL(clientFd, buf, sizeof(buf) - 1) == 0)
		{
			LOGW("A client tried to connect, but no command has been recieved within 5 seconds\n");
			LOGD("Closing the connection\n");
			close(clientFd);
			continue;
		}

		if(strcmp(buf, "TRENO"))
		{
			treni++;
			int args[] = {clientFd, map, treni};
			writeWL(clientFd, "OK", 3);
			tryFork(trenoLoop, args);
		}
		else if(strcmp(buf, "SUPER"))
		{
			int args[] = {clientFd, map};
			writeWL(clientFd, "OK", 3);
			tryFork(superLoop, &args);
		}
		else
		{
			LOGW("A client issued an unknown command: %s\n", buf);
			writeWL(clientFd, "418 TEAPOT", 10);
		}
	}
}

static void tryFork(void(*fun)(void *), void *args)
{
	int fk = fork();
	if(fk == 0)
	{
		(*fun)(args);
		exit(EXIT_SUCCESS);
	}
	else if(fk < 0)
	{
		perror("fork");
		LOGE("fork() failed\n");
		exit(EXIT_FAILURE);
	}
}

static void trenoLoop(void *args)
{
	///TODO: refactor this abomination

	int fd = ((int *)args)[0];
	mappa_id map_id = (mappa_id)((int *)args)[1];
	int id = ((int *)args)[2];

	char **mappa[] = map_id == MAPPA1 ? nomi_mappa1 : nomi_mappa2;

	struct timeval rdtimeout = {0};
	if(setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, /*(const char*)*/&rdtimeout, sizeof rdtimeout) < 0)
	{
		LOGE("Impossibile impostare la socket!\n");
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}

	char buf[16 + 1] = {0};

	for(;;)
	{
		readWL(fd, buf, sizeof(buf) - 1);
		
		if(strcmp(buf, "TRENI"))
		{
			int numTreni = getNumTreni(map_id);
			writeWL(fd, &numTreni, sizeof(int));
		}
		else if(strcmp(buf, "ITINERARIO"))
		{
			int numTappe = getNumTappe(mappa[id]);
			writeWL(fd, &numTappe, sizeof(int));
			for(int i=0; i<numTappe; i++)
			{
				writeWL(fd, mappa[id], strlen(mappa[id]));
			}
		}
	}
}

static void superLoop(void *args)
{
	(void) args;
}

static int creaSocket(void)
{
	int sfd;
	struct sockaddr_un serverAddr =
	{
		.sun_family = AF_UNIX,
		.sun_path = "./sock/registro"
	};

	struct timeval rdtimeout =
	{
		.tv_sec = 5
	};

	mkdir("./sock", 0777);

	sfd = socket(AF_UNIX, SOCK_STREAM, 0);

	//Imposta un timeout
	if(setsockopt(sfd, SOL_SOCKET, SO_RCVTIMEO, /*(const char*)*/&rdtimeout, sizeof rdtimeout) < 0)
	{
		LOGE("Impossibile impostare la socket!\n");
		perror("registro_client");
		exit(EXIT_FAILURE);
	}
	
	bind(sfd, &serverAddr, sizeof(struct sockaddr_un));
	listen(sfd, 5);

	return sfd;
}

static inline uint8_t getNumTreni(mappa_id map)
{
	switch(map)
	{
		case MAPPA1:
			return TRENI_MAPPA1;
		case MAPPA2:
			return TRENI_MAPPA2;
		default:
			return 0;
	}
}

static int getNumTappe(char **itinerario)
{
	int stazioni = 0;

	do
	{
		if(ISSTAZIONE(*itinerario))
			stazioni++;

		itinerario++;
	} while (stazioni < 2);
}
